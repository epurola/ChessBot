#include "Board.h"


// 8/8/8/8/8/8/8/KR6 b - - 0 1
// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
// 7r/8/8/k7/r7/8/8/pK6 b - - 0 1
//4r1k1/1q1b1pb1/1n1p1npp/2pPp3/2P1P3/2BB1N1P/2QN1PP1/R5K1 b - - 3 25

Board::Board()
{
    
    setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    enPassantTarget = 0;
    lastMoveEnPassant = false;
    WhiteCanCastleK = true;
    WhiteCanCastleQ = true;
    blackCanCastleK = true;
    blackCanCastleQ = true;
    whiteToMove = true;
    attackTable.initialize();
    initializeZobrist();
    zobristHash = computeZobristHash();
}
Board::Board(const std::shared_ptr<Board> &other1)
{
    const Board &other = *other1;
    // Copy basic properties
    this->whitePawns = other.whitePawns;
    this->blackPawns = other.blackPawns;
    this->whiteKnights = other.whiteKnights;
    this->blackKnights = other.blackKnights;
    this->whiteBishops = other.whiteBishops;
    this->blackBishops = other.blackBishops;
    this->whiteRooks = other.whiteRooks;
    this->blackRooks = other.blackRooks;
    this->whiteQueens = other.whiteQueens;
    this->blackQueens = other.blackQueens;
    this->whiteKing = other.whiteKing;
    this->blackKing = other.blackKing;
    this->enPassantTarget = other.enPassantTarget;
    this->lastMoveEnPassant = other.lastMoveEnPassant;
    this->enpassantCapturedSquare = other.enpassantCapturedSquare;
    this->enpassantCapturedPiece = other.enpassantCapturedPiece;
    this->blackCanCastleQ = other.blackCanCastleQ;
    this->blackCanCastleK = other.blackCanCastleK;
    this->WhiteCanCastleQ = other.WhiteCanCastleQ;
    this->WhiteCanCastleK = other.WhiteCanCastleK;
    this->whiteToMove = other.whiteToMove;
    this->zobristHash = other.zobristHash;

    // Copy any dynamic data structures like move history and transposition table
    this->moveHistory = other.moveHistory;
    std::copy(std::begin(other.kingMovesTable), std::end(other.kingMovesTable), std::begin(this->kingMovesTable));
    std::copy(std::begin(other.pinMasks), std::end(other.pinMasks), std::begin(this->pinMasks));

    // Initialize any other members (like AttackTable) if necessary
    this->attackTable.initialize();

    // Reinitialize Zobrist if required, since we have just copied the data
    this->initializeZobrist();
}

void Board::resetBoard()
{
    setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    enPassantTarget = 0;
    lastMoveEnPassant = false;
    WhiteCanCastleK = true;
    WhiteCanCastleQ = true;
    blackCanCastleK = true;
    blackCanCastleQ = true;
    whiteToMove = true;
    attackTable.initialize();
    initializeZobrist();
    zobristHash = computeZobristHash();
    gameFensHistory.clear();
    moveCount = 0;
}

bool Board::isThreefoldRepetition() {
    computeZobristHash();

    uint64_t hash = getZobristHash();
    if (gameFensHistory[hash] >= 2) {
        return true;
    }
    return false;
}

void Board::initializeZobrist()
{
    std::mt19937_64 rng(123456789);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    for (int piece = 0; piece < PIECES; piece++)
    {
        for (int square = 0; square < SQUARES; square++)
        {
            zobristTable[piece][square] = dist(rng);
        }
    }

    for (int i = 0; i < CASTLING_RIGHTS; i++)
    {
        castlingTable[i] = dist(rng);
    }

    for (int i = 0; i < EN_PASSANT_FILES; i++)
    {
        enPassantTable[i] = dist(rng);
    }
    sideToMoveHash = dist(rng);
}

int Board::pieceToIndex(char piece)
{
    switch (piece)
    {
    case 'P':
        return 0; // White Pawn
    case 'N':
        return 1; // White Knight
    case 'B':
        return 2; // White Bishop
    case 'R':
        return 3; // White Rook
    case 'Q':
        return 4; // White Queen
    case 'K':
        return 5; // White King
    case 'p':
        return 6; // Black Pawn
    case 'n':
        return 7;
    case 'b':
        return 8;
    case 'r':
        return 9;
    case 'q':
        return 10;
    case 'k':
        return 11;
    default:
        return -1;
    }
}

uint64_t Board::computeZobristHash()
{
    uint64_t hash = 0;

    for (int square = 0; square < 64; square++)
    {
        char piece = getPieceAtSquare(square);
        int pieceIndex = pieceToIndex(piece);
        if (pieceIndex != -1)
        {
            hash ^= zobristTable[pieceIndex][square];
        }
    }

    if (!whiteToMove)
    {
        hash ^= sideToMoveHash;
    }

    if (WhiteCanCastleK)
        hash ^= castlingTable[0];
    if (WhiteCanCastleQ)
        hash ^= castlingTable[1];
    if (blackCanCastleK)
        hash ^= castlingTable[2];
    if (blackCanCastleQ)
        hash ^= castlingTable[3];

    if (enPassantTarget)
    {
        unsigned long index;
        if (_BitScanForward64(&index, enPassantTarget))
        {
            int file = index % 8;
            hash ^= enPassantTable[file];
        }
    }

    zobristHash = hash;

    return hash;
}

bool Board::probeTranspositionTable(uint64_t hash, int depth, int alpha, int beta, TTEntry &entry)
{
    TTEntry &result = transpositionTable[hash % TABLE_SIZE];

    if (result.hash == hash) // Ensure we are checking the correct position
    {
        if (result.depth >= depth) // Only use if stored depth is sufficient
        {
            if (result.flag == TTFlag::EXACT)
            {
                entry = result;
                return true;
            }
            else if (result.flag == TTFlag::LOWERBOUND && result.evaluation >= beta)
            {
                entry = result;
                return true;
            }
            else if (result.flag == TTFlag::UPPERBOUND && result.evaluation <= alpha)
            {
                entry = result;
                return true;
            }
        }
    }

    return false;
}

// Store position in transposition table
void Board::storeTransposition(uint64_t hash, int depth, int eval, int alpha, int beta, int from, int to)
{
    TTEntry &entry = transpositionTable[hash % TABLE_SIZE];
    entry.hash = hash;
    entry.evaluation = eval;
    entry.depth = depth;
    entry.bestFrom = from;
    entry.bestTo = to;
    if (eval <= alpha)
        entry.flag = TTFlag::UPPERBOUND; // Eval is worse than or equal to beta, we can't improve
    else if (eval >= beta)
        entry.flag = TTFlag::LOWERBOUND; // Eval is better than or equal to alpha, we can't worsen
    else
        entry.flag = TTFlag::EXACT;
}

void Board::storeMove(int from, int to, char capturedPiece, uint64_t enpSquare,
                      bool wasEnPassant, int enPassantCapturedSquare,
                      char enPassantCapturedPiece, bool wasPromotion,
                      char originalPawn, bool WhiteCastleKBefore,
                      bool WhiteCastleQBefore, bool BlackCastleKBefore, bool BlackCastleQBefore, uint64_t hash, bool whiteTurn)
{
    if (moveCount >= MAX_MOVES) {
        // You can choose to throw an exception or handle overflow as needed.
        throw std::runtime_error("Move history is full!");
    }

    // Create and store the move
    moveHistory[moveCount++] = Move{
        from, to, capturedPiece, enpSquare, wasEnPassant, 
        enPassantCapturedSquare,           
        enPassantCapturedPiece, 
        wasPromotion, 
        originalPawn,
        WhiteCastleKBefore, WhiteCastleQBefore,
        BlackCastleKBefore, BlackCastleQBefore,
        hash, 
        whiteTurn
    };
}

Move Board::getLastMove() {
    if (moveCount == 0) {
        // Return a default move or handle as you wish.
        return Move{};
    }
    return moveHistory[moveCount - 1];
}

bool Board::gameOver(bool maximizingPlayer)
{
    
    int kingSquare = maximizingPlayer
                         ? bitScanForward(whiteKing.bitboard)
                         : bitScanForward(blackKing.bitboard);

    char king = getPieceAtSquare(kingSquare);


    uint64_t moves = generateKingMoves(kingSquare, king);

    if (moves == 0)
    {
        constexpr int MAX_MOVES = 218;
        std::pair<int, int> legalMoves[MAX_MOVES];
        int moveCount = getAllLegalMovesAsArray(legalMoves, maximizingPlayer).first;

        return (moveCount == 0);
    }

    return false;
}

bool Board::isKingInCheck(bool maximizingPlayer)
{
    int kingSquare = maximizingPlayer
                         ? bitScanForward(whiteKing.bitboard)
                         : bitScanForward(blackKing.bitboard);
    uint64_t opponentAttacks = getOpponentAttacks(maximizingPlayer ? 'B' : 'w');
    return (opponentAttacks & (1ULL << kingSquare)) != 0;
}

void Board::setFen(const std::string &fen)
{

    whitePawns.bitboard= 0;
    blackPawns.bitboard = 0;
    whiteKnights.bitboard = 0;
    blackKnights.bitboard = 0;
    whiteBishops.bitboard = 0;
    blackBishops.bitboard = 0;
    whiteRooks.bitboard = 0;
    blackRooks.bitboard = 0;
    whiteQueens.bitboard = 0;
    blackQueens.bitboard = 0;
    whiteKing.bitboard = 0;
    blackKing.bitboard = 0;

    std::istringstream fenStream(fen);
    std::string boardPart, turnPart, castlingPart, enPassantPart;
    int halfMoveClock, fullMoveNumber;

    fenStream >> boardPart >> turnPart >> castlingPart >> enPassantPart >> halfMoveClock >> fullMoveNumber;

    std::unordered_map<char, Bitboard *> pieceMap = {
        {'P', &whitePawns}, {'p', &blackPawns}, {'N', &whiteKnights}, {'n', &blackKnights}, {'B', &whiteBishops}, {'b', &blackBishops}, {'R', &whiteRooks}, {'r', &blackRooks}, {'Q', &whiteQueens}, {'q', &blackQueens}, {'K', &whiteKing}, {'k', &blackKing}};

    int rankIndex = 0, fileIndex = 0;
    for (char c : boardPart)
    {
        if (c == '/')
        {
            rankIndex++;
            fileIndex = 0;
        }
        else if (isdigit(c))
        {
            fileIndex += c - '0';
        }
        else
        {
            if (pieceMap.count(c))
            {
                int square = rankIndex * 8 + fileIndex;
                pieceMap[c]->bitboard |= (1ULL << square);
            }
            fileIndex++;
        }
    }
    enPassantTarget = (enPassantPart != "-") ? (1ULL << ((enPassantPart[0] - 'a') + 8 * (enPassantPart[1] - '1'))) : 0;

    WhiteCanCastleK = false;
    WhiteCanCastleQ = false;
    blackCanCastleK = false;
    blackCanCastleQ = false;

    for (char c : castlingPart)
    {
        if (c == 'K')
        {
            WhiteCanCastleK = true;
        }
        else if (c == 'Q')
        {
            WhiteCanCastleQ = true;
        }
        else if (c == 'k')
        {
            blackCanCastleK = true;
        }
        else if (c == 'q')
        {
            blackCanCastleQ = true;
        }
    }

    if (turnPart == "w")
        whiteToMove = true;
    else
        whiteToMove = false;


}

std::string Board::getFen()
{
    std::string fen = "";
    for (int rank = 0; rank < 8; rank++) {
        int emptySquares = 0;
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            char piece = getPieceAtSquare(square);
    
            if (piece == ' ') {
                emptySquares++;
            } else {
                if (emptySquares > 0) {
                    fen += std::to_string(emptySquares);  // Append empty squares
                    emptySquares = 0;  // Reset empty squares counter
                }
                fen += piece;  // Append the piece to the FEN string
            }
        }
    
        // Handle trailing empty squares at the end of the rank
        if (emptySquares > 0) {
            fen += std::to_string(emptySquares);
        }
    
        // Add the rank separator, unless it's the last rank
        if (rank < 7) {
            fen += "/";
        }
    }


    fen += whiteToMove ? " w " : " b ";
    fen += WhiteCanCastleK ? "K" : "";
    fen += WhiteCanCastleQ ? "Q" : "";
    fen += blackCanCastleK ? "k" : "";
    fen += blackCanCastleQ ? "q" : "";
    if(!WhiteCanCastleK && !WhiteCanCastleQ && !blackCanCastleK && !blackCanCastleQ){
        fen += " -";
    }
    int squareIndex = bitScanForward(enPassantTarget);

    char file = 'a' + (squareIndex % 8); 
    char rank = '1' + (squareIndex / 8); 

    fen += enPassantTarget ? " " + std::string(1, file) + std::string(1, rank) : " -";

    fen += " 0 1";

    return fen;
}

void Board::undoMove(int from, int to, char capturedPiece, uint64_t enpSquare,
                     bool lastMoveEnPassant1, int enpassantCapturedSquare1,
                     char enpassantCapturedPiece1, bool wasPromotion,
                     char originalPawn, bool WhiteCastleKBefore,
                     bool WhiteCastleQBefore, bool BlackCastleKBefore, bool BlackCastleQBefore, uint64_t hash, bool whiteTurn)
{
   
    if (gameFensHistory.find(hash) != gameFensHistory.end()) {
        gameFensHistory[hash]--; 
        if (gameFensHistory[hash] == 0) {
            gameFensHistory.erase(hash); 
        }
    }
    
    whiteToMove = whiteTurn;
    this->enPassantTarget = enpSquare;
    
    char movedPiece = getPieceAtSquare(to);

    WhiteCanCastleK = WhiteCastleKBefore;
    WhiteCanCastleQ = WhiteCastleQBefore;
    blackCanCastleK = BlackCastleKBefore;
    blackCanCastleQ = BlackCastleQBefore;

    if (std::tolower(movedPiece) == 'k' && std::abs(from - to) == 2)
    {
        int rookFrom, rookTo;

        if (to == 6)
        {
            rookFrom = 5;
            rookTo = 7;
        }
        else if (to == 2)
        {
            rookFrom = 3;
            rookTo = 0;
        }
        else if (to == 62)
        {
            rookFrom = 61;
            rookTo = 63;
        }
        else if (to == 58)
        {
            rookFrom = 59;
            rookTo = 56;
        }
        
        updateBitboards(movedPiece, to, from);

        updateBitboards(getPieceAtSquare(rookFrom), rookFrom, rookTo);
      
    }

    if (wasPromotion)
    {

        if (std::isupper(originalPawn))
        {
            whiteQueens.clearSquare(to);
            whitePawns.setSquare(from);
        }
        else
        {
            blackQueens.clearSquare(to);
            blackPawns.setSquare(from);
        }
    }
    else
    {
        char movedPiece_ = getPieceAtSquare(to);
        updateBitboards(movedPiece_, to, from);
    }

    if (capturedPiece != ' ')
    {
        restoreCapturedPiece(to, capturedPiece);
    }

    if (lastMoveEnPassant1)
    {
        restoreCapturedPiece(enpassantCapturedSquare1, enpassantCapturedPiece1);
    }



}

void Board::restoreCapturedPiece(int square, char piece)
{
    if (piece == 'p')
        blackPawns.setSquare(square);
    if (piece == 'P')
        whitePawns.setSquare(square);
    if (piece == 'n')
        blackKnights.setSquare(square);
    if (piece == 'N')
        whiteKnights.setSquare(square);
    if (piece == 'b')
        blackBishops.setSquare(square);
    if (piece == 'B')
        whiteBishops.setSquare(square);
    if (piece == 'r')
        blackRooks.setSquare(square);
    if (piece == 'R')
        whiteRooks.setSquare(square);
    if (piece == 'q')
        blackQueens.setSquare(square);
    if (piece == 'Q')
        whiteQueens.setSquare(square);
    if (piece == 'k')
        blackKing.setSquare(square);
    if (piece == 'K')
        whiteKing.setSquare(square);
}

bool Board::movePiece(int from, int to)
{
    computeZobristHash();
    uint64_t hash = getZobristHash();
    gameFensHistory[hash]++; 

    char piece = getPieceAtSquare(from);
    char destPiece = getPieceAtSquare(to);

    /*if (piece == ' ')
    {
        return false;
    }*/
    bool WhiteCanCastleK1 = WhiteCanCastleK;
    bool WhiteCanCastleQ1 = WhiteCanCastleQ;
    bool blackCanCastleK1 = blackCanCastleK;
    bool blackCanCastleQ1 = blackCanCastleQ;
    
    
    if (std::tolower(piece) == 'k')
    {
        int diff = from - to;

        if (std::abs(diff) == 2)
        {
            int color = std::islower(piece) ? 0 : 1;

            if (color && diff < 0)
            {
                if (WhiteCanCastleK)
                {
                    updateBitboards('R', 63, 61);
                    WhiteCanCastleK = WhiteCanCastleQ = false;
                }
            }
            else if (color && diff > 0)
            {
                if (WhiteCanCastleQ)
                {
                    updateBitboards('R', 56, 59);
                    WhiteCanCastleK = WhiteCanCastleQ = false;
                }
            }
            else if (!color && diff < 0)
            {
                if (blackCanCastleK)
                {
                    updateBitboards('r', 7, 5);
                    blackCanCastleK = blackCanCastleQ = false;
                }
            }
            else if (!color && diff > 0)
            {
                if (blackCanCastleQ)
                {
                    updateBitboards('r', 0, 3);
                    blackCanCastleK = blackCanCastleQ = false;
                }
            }
        }
    }

    if (std::tolower(piece) == 'k')
    {
        if (std::isupper(piece))
        {
            WhiteCanCastleK = false;
            WhiteCanCastleQ = false;
        }
        else
        {
            blackCanCastleK = false;
            blackCanCastleQ = false;
        }
    }

    if (std::tolower(piece) == 'r')
    {
        if (from == 56)
            WhiteCanCastleQ = false;
        if (from == 63)
            WhiteCanCastleK = false;
        if (from == 0)
            blackCanCastleQ = false;
        if (from == 7)
            blackCanCastleK = false;
    }

    if (destPiece != ' ')
    {
        clearCapturedPiece(to, destPiece);
    }

    if (std::tolower(piece) == 'p' && (1ULL << to) == enPassantTarget)
    {
        int capturedPawnSquare = (std::islower(piece)) ? (to - 8) : (to + 8);
        char capturedPawn = getPieceAtSquare(capturedPawnSquare);
        if (std::tolower(capturedPawn) == 'p')
        {
            clearCapturedPiece(capturedPawnSquare, capturedPawn);
            lastMoveEnPassant = true;
        }
        enpassantCapturedSquare = capturedPawnSquare;
        enpassantCapturedPiece = capturedPawn;
    }

    bool result = updateBitboards(piece, from, to);
    bool wasPromotion = false;

    if (std::tolower(piece) == 'p' && (to >= 56 || to <= 7))
    {
        wasPromotion = true;

        if (std::isupper(piece))
        {
            whitePawns.clearSquare(to);
            whiteQueens.setSquare(to);
        }
        else
        {
            blackPawns.clearSquare(to);
            blackQueens.setSquare(to);
        }
    }
    storeMove(from, to, destPiece, enPassantTarget, lastMoveEnPassant,
              enpassantCapturedSquare,
              enpassantCapturedPiece, wasPromotion, piece,
              WhiteCanCastleK1, WhiteCanCastleQ1, blackCanCastleK1, blackCanCastleQ1, hash, whiteToMove);

    enpassantCapturedSquare = -1;
    enpassantCapturedPiece = ' ';
    lastMoveEnPassant = 0;
    wasPromotion = false;

    if (std::tolower(piece) == 'p' && std::abs(from - to) == 16)
    {
        enPassantTarget = 1ULL << ((from + to) / 2);
    }
    else
    {
        enPassantTarget = 0;
    }
    
    whiteToMove = !whiteToMove;
    return result;
}

void Board::clearCapturedPiece(int to, char destPiece)
{
    if (destPiece != ' ')
    {
        if ((1ULL << to) & enPassantTarget)
        {
            if (destPiece == 'p')
            {
                blackPawns.clearSquare(to - 8);
            }

            if (destPiece == 'P')
            {
                whitePawns.clearSquare(to + 8);
            }
        }

        if (destPiece == 'p')
            blackPawns.clearSquare(to);
        if (destPiece == 'P')
            whitePawns.clearSquare(to);
        if (destPiece == 'n')
            blackKnights.clearSquare(to);
        if (destPiece == 'N')
            whiteKnights.clearSquare(to);
        if (destPiece == 'b')
            blackBishops.clearSquare(to);
        if (destPiece == 'B')
            whiteBishops.clearSquare(to);
        if (destPiece == 'r')
            blackRooks.clearSquare(to);
        if (destPiece == 'R')
            whiteRooks.clearSquare(to);
        if (destPiece == 'q')
            blackQueens.clearSquare(to);
        if (destPiece == 'Q')
            whiteQueens.clearSquare(to);
        if (destPiece == 'k')
            blackKing.clearSquare(to);
        if (destPiece == 'K')
            whiteKing.clearSquare(to);
    }
}

char Board::getPieceAtSquare(int square)
{
    if (whitePawns.isSet(square))
        return 'P';
    if (blackPawns.isSet(square))
        return 'p';
    if (whiteKnights.isSet(square))
        return 'N';
    if (blackKnights.isSet(square))
        return 'n';
    if (whiteBishops.isSet(square))
        return 'B';
    if (blackBishops.isSet(square))
        return 'b';
    if (whiteRooks.isSet(square))
        return 'R';
    if (blackRooks.isSet(square))
        return 'r';
    if (whiteQueens.isSet(square))
        return 'Q';
    if (blackQueens.isSet(square))
        return 'q';
    if (whiteKing.isSet(square))
        return 'K';
    if (blackKing.isSet(square))
        return 'k';
    return ' ';
}

bool Board::isValidMove(int from, int to)
{
    char piece = getPieceAtSquare(from);
    char lowerCasePiece = static_cast<char>(std::tolower(piece));

    switch (lowerCasePiece)
    {
    case 'p':
        return legalPawnMove(from, to);
    case 'n':
        return legalKnightMove(from, to);
    case 'b':
        return legalBishopMove(from, to);
    case 'r':
        return legalRookMove(from, to);
    case 'q':
        return legalQueenMove(from, to);
    case 'k':
        return legalKingMove(from, to);
    default:
        return false;
    }
}

uint64_t Board::findCheckers(int squareOfKing, char king, uint64_t &checkMask)
{
    uint64_t checkers = 0;

    int color = std::islower(king) ? 1 : 0;

    uint64_t opponentPawns = color ? whitePawns.bitboard : blackPawns.bitboard;
    uint64_t opponentKnights = color ? whiteKnights.bitboard : blackKnights.bitboard;
    uint64_t opponentBishops = color ? whiteBishops.bitboard | whiteQueens.bitboard : blackBishops.bitboard | blackQueens.bitboard;
    uint64_t opponentRooks = color ? whiteRooks.bitboard | whiteQueens.bitboard : blackRooks.bitboard | blackQueens.bitboard;
    // uint64_t opponentKing = color ? whiteKing.bitboard : blackKing.bitboard;

    uint64_t attackingPawns = opponentPawns;
    while (attackingPawns)
    {
        int pawnSquare = bitScanForward(attackingPawns);
        char piece = getPieceAtSquare(pawnSquare);
        int colorPawn = std::islower(piece) ? 0 : 1;

        uint64_t pawnAttacks = generatePawnMovesForKing(pawnSquare, piece);

        if (pawnAttacks & (1ULL << squareOfKing))
        {
            checkers |= (1ULL << pawnSquare);

            if (enPassantTarget > 0)
            {
                int offset = (colorPawn == 0) ? -8 : 8;
                checkers |= (1ULL << (pawnSquare + offset));
                checkers &= ~(1ULL << pawnSquare);
            }
        }

        attackingPawns &= attackingPawns - 1; // Remove the current pawn and continue
    }

    uint64_t knights = opponentKnights;
    while (knights)
    {
        int square = bitScanForward(knights);
        if (attackTable.knightMovesTable[square] & (1ULL << squareOfKing))
        {
            checkers |= (1ULL << square);
        }
        knights &= knights - 1;
    }

    uint64_t rooks = opponentRooks;
    while (rooks)
    {
        int square = bitScanForward(rooks);
        uint64_t blockers = getOccupiedSquares();
        blockers &= attackTable.rookMask[square];

        uint64_t hash = (blockers * attackTable.rookMagics[square]) >> (64 - attackTable.rookIndex[square]);
        if (attackTable.rookTable[square][hash] & (1ULL << squareOfKing))
        {
            checkers |= (1ULL << square);
        }
        rooks &= rooks - 1;
    }

    uint64_t bishops = opponentBishops;
    while (bishops)
    {
        int square = bitScanForward(bishops);
        uint64_t blockers = getOccupiedSquares();
        blockers &= attackTable.bishopMask[square];

        uint64_t hash = (blockers * attackTable.bishopMagics[square]) >> (64 - attackTable.bishopIndex[square]);
        if (attackTable.bishopTable[square][hash] & (1ULL << squareOfKing))
        {
            checkers |= (1ULL << square);
        }
        bishops &= bishops - 1;
    }

    unsigned __int64 numCheckers = __popcnt64(checkers); // Count the number of checkers
    if (numCheckers == 1)
    {
        int checkerSquare = bitScanForward(checkers);
        checkMask = attackTable.betweenTable[squareOfKing][checkerSquare] | (1ULL << checkerSquare);
    }
    else if (numCheckers >= 2)
    {
        // Double check: The king must move, so checkMask should be 0
        checkMask = 0;
    }
    return checkers;
}

uint64_t Board::findPinnedPieces(int squareOfKing, char king)
{
    uint64_t pinnedPieces = 0;

    int color = std::islower(king) ? 1 : 0;

    uint64_t xRayKing = attackTable.rookMaskFull[squareOfKing] | attackTable.bishopMaskFull[squareOfKing];
    uint64_t xRayKingDiagonal = attackTable.bishopMaskFull[squareOfKing];
    uint64_t xRayKingRookAttackers = attackTable.rookMaskFull[squareOfKing];

    xRayKing = (1ULL << squareOfKing) | xRayKing;

    uint64_t diagonalAttackers = color ? whiteBishops.bitboard
                                       : blackBishops.bitboard;

    // uint64_t diagonalAttackersQueen = color ? whiteQueens.bitboard
    //                                         : blackQueens.bitboard;

    uint64_t rooks = color ? whiteRooks.bitboard : blackRooks.bitboard;
    uint64_t queens = color ? whiteQueens.bitboard : blackQueens.bitboard;

    uint64_t queensInLine = xRayKing & queens;

    while (queensInLine)
    {
        int queenSquare = bitScanForward(queensInLine);

        // uint64_t kingRookLine = attackTable.rookMaskFull[queenSquare];
        uint64_t lineOfSight = attackTable.betweenTable[squareOfKing][queenSquare] | (1ULL << squareOfKing);

        uint64_t blockers = lineOfSight & (getBlackPieces() | getWhitePieces());
        blockers &= ~(1ULL << squareOfKing);

        if (blockers)
        {
            int blockerSquare = bitScanForward(blockers);

            if (isPiecePinnedToKing(blockerSquare, squareOfKing, lineOfSight))
            {
                pinnedPieces |= blockers;
                pinMasks[blockerSquare] = lineOfSight | (1ULL << queenSquare);
            }
        }
        queensInLine &= queensInLine - 1;
    }

    uint64_t rooksInLine = xRayKingRookAttackers & rooks;

    while (rooksInLine)
    {
        int rookSquare = bitScanForward(rooksInLine);
        uint64_t lineOfSight = attackTable.betweenTable[squareOfKing][rookSquare] | (1ULL << squareOfKing);
        uint64_t blockers = lineOfSight & (getBlackPieces() | getWhitePieces());
        blockers &= ~(1ULL << squareOfKing);

        if (blockers)
        {
            int blockerSquare = bitScanForward(blockers);

            if (isPiecePinnedToKing(blockerSquare, squareOfKing, lineOfSight))
            {
                pinnedPieces |= blockers;
                pinMasks[blockerSquare] = lineOfSight | (1ULL << rookSquare);
            }
        }
        rooksInLine &= rooksInLine - 1;
    }

    uint64_t bishopsInLine = xRayKingDiagonal & diagonalAttackers;
    while (bishopsInLine)
    {
        int bishopSquare = bitScanForward(bishopsInLine);
        uint64_t lineOfSight = attackTable.betweenTable[squareOfKing][bishopSquare] | (1ULL << squareOfKing);

        uint64_t blockers = lineOfSight & (getBlackPieces() | getWhitePieces());
        blockers &= ~(1ULL << squareOfKing);
        if (blockers)
        {
            int blockerSquare = bitScanForward(blockers);

            if (isPiecePinnedToKing(blockerSquare, squareOfKing, lineOfSight))
            {
                pinnedPieces |= blockers;
                pinMasks[blockerSquare] = lineOfSight | (1ULL << bishopSquare);
            }
        }
        bishopsInLine &= bishopsInLine - 1;
    }

    return pinnedPieces;
}

bool Board::isPiecePinned(int pieceSquare, int squareOfKing, char king)
{
    uint64_t pinnedPieces = findPinnedPieces(squareOfKing, king);
    return (pinnedPieces >> pieceSquare) & 1;
}

bool Board::isPiecePinnedToKing(int pieceSquare, int squareOfKing, uint64_t lineOfSight)
{

    if ((lineOfSight >> pieceSquare) & 1)
    {

        if ((lineOfSight >> squareOfKing) & 1)
        {

            uint64_t blockersBetween = lineOfSight & (getWhitePieces() | getBlackPieces());

            blockersBetween &= ~(1ULL << pieceSquare);
            blockersBetween &= ~(1ULL << squareOfKing);
            if (blockersBetween == 0)
            {
                return true;
            }
        }
    }
    return false;
}

uint64_t Board::generatePawnMoves(int square, char piece)
{
    uint64_t moves = 0;

    bool isWhite = std::isupper(piece);
    int direction = isWhite ? -8 : 8;

    uint64_t kingBoard = std::islower(piece) ? blackKing.bitboard : whiteKing.bitboard;
    int kingSquare = bitScanForward(kingBoard);
    char king = getPieceAtSquare(kingSquare);
    uint64_t checkMask;
    uint64_t checkers = findCheckers(kingSquare, king, checkMask);

    uint64_t pinnedPieces = findPinnedPieces(kingSquare, king);

    uint64_t occupied = getOccupiedSquares();
    uint64_t opponentsPieces = isWhite ? getBlackPieces() : getWhitePieces();

    uint64_t forwardMove = 1ULL << (square + direction);

    if (!(occupied & forwardMove))
    {
        moves |= forwardMove;
    }

    if (!(occupied & forwardMove))
    {
        moves |= forwardMove;

        if (!isWhite && (square >= 8 && square < 16))
        {
            uint64_t doubleMove = 1ULL << (square + 2 * direction);
            uint64_t intermediateSquare = 1ULL << (square + direction);

            if (!(occupied & doubleMove) && !(occupied & intermediateSquare))
            {
                moves |= doubleMove;
            }
        }
        else if (isWhite && (square >= 48 && square < 56))
        {
            uint64_t doubleMove = 1ULL << (square + 2 * direction);
            uint64_t intermediateSquare = 1ULL << (square + direction);

            if (!(occupied & doubleMove) && !(occupied & intermediateSquare))
            {
                moves |= doubleMove;
            }
        }
    }
    if (square % 8 != 0)
    {
        uint64_t leftCapture = (1ULL << (square + direction - 1));
        if (opponentsPieces & leftCapture)
        {
            moves |= leftCapture;
        }
    }

    if (square % 8 != 7)
    {
        uint64_t rightCapture = (1ULL << (square + direction + 1));
        if (opponentsPieces & rightCapture)
        {
            moves |= rightCapture;
        }
    }

    if (enPassantTarget  )
    {
        if (square % 8 != 0)
        {
            uint64_t leftEnPassant = (1ULL << (square + direction - 1));
            if (leftEnPassant & enPassantTarget)
            {
                moves |= leftEnPassant;
            }
        }
        if (square % 8 != 7)
        {
            uint64_t rightEnPassant = (1ULL << (square + direction + 1));
            if (rightEnPassant & enPassantTarget)
            {
                moves |= rightEnPassant;
            }
        }
    }

    if (pinnedPieces & (1ULL << square))
    {
        moves &= pinMasks[square];
    }
    if (checkers)
        moves &= checkMask;

    return moves;
}

uint64_t Board::generatePawnMovesForKing(int square, char piece)
{
    uint64_t moves = 0;
    bool isWhite = std::isupper(piece);
    int direction = isWhite ? -8 : 8;

    if (square % 8 != 0)
        moves |= (1ULL << (square + direction - 1));

    if (square % 8 != 7)
        moves |= (1ULL << (square + direction + 1));

    return moves;
}

uint64_t Board::generateKnightMoves(int square, char piece)
{

    uint64_t kingBoard = std::islower(piece) ? blackKing.bitboard : whiteKing.bitboard;
    int kingSquare = bitScanForward(kingBoard);
    char king = getPieceAtSquare(kingSquare);

    uint64_t pinnedPieces = findPinnedPieces(kingSquare, king);
    uint64_t moves = attackTable.knightMovesTable[square];
    uint64_t checkMask;
    uint64_t checkers = findCheckers(kingSquare, king, checkMask);

    uint64_t friendlyPieces = std::islower(piece) ? getBlackPieces() : getWhitePieces();

    if (pinnedPieces & (1ULL << square))
    {
        moves &= pinMasks[square];
    }

    if (checkers)
        moves &= checkMask;

    return moves & ~friendlyPieces;
}

uint64_t Board::generateKnightMovesWithProtection(int square, char piece)
{

    uint64_t kingBoard = std::islower(piece) ? blackKing.bitboard : whiteKing.bitboard;
    int kingSquare = bitScanForward(kingBoard);
    char king = getPieceAtSquare(kingSquare);

    uint64_t moves = attackTable.knightMovesTable[square];
    uint64_t checkMask;
    uint64_t checkers = findCheckers(kingSquare, king, checkMask);

    if (checkers)
        moves &= checkMask;

    return moves;
}

uint64_t Board::generateBishopMoves(int square, char piece)
{
    uint64_t moves;
    uint64_t kingBoard = std::islower(piece) ? blackKing.bitboard : whiteKing.bitboard;
    int kingSquare = bitScanForward(kingBoard);
    char king = getPieceAtSquare(kingSquare);

    uint64_t pinnedPieces = findPinnedPieces(kingSquare, king);
    uint64_t checkMask;
    uint64_t checkers = findCheckers(kingSquare, king, checkMask);

    uint64_t blockers = getOccupiedSquares();
    blockers &= attackTable.bishopMask[square];

    uint64_t hash = (blockers * attackTable.bishopMagics[square]) >> (64 - attackTable.bishopIndex[square]);

    uint64_t friendly;

    std::islower(piece) ? friendly = getBlackPieces() : friendly = getWhitePieces();
    moves = attackTable.bishopTable[square][hash];

    if (pinnedPieces & (1ULL << square))
    {
        moves &= pinMasks[square];
    }
    if (checkers)
        moves &= checkMask;
    moves &= ~friendly;
    return moves;
}
uint64_t Board::generateBishopMovesWithProtection(int square, char piece)
{
    uint64_t moves;
    uint64_t kingBoard = std::islower(piece) ? blackKing.bitboard : whiteKing.bitboard;
    int kingSquare = bitScanForward(kingBoard);
    char king = getPieceAtSquare(kingSquare);

    uint64_t checkMask;
    uint64_t checkers = findCheckers(kingSquare, king, checkMask);

    uint64_t blockers = getOccupiedSquares();
    uint64_t opponentKingBoard = std::islower(piece) ? whiteKing.bitboard : blackKing.bitboard;
    blockers &= ~opponentKingBoard;
    blockers &= attackTable.bishopMask[square];

    uint64_t hash = (blockers * attackTable.bishopMagics[square]) >> (64 - attackTable.bishopIndex[square]);

    uint64_t friendly;

    std::islower(piece) ? friendly = getBlackPieces() : friendly = getWhitePieces();
    moves = attackTable.bishopTable[square][hash];

    if (checkers)
        moves &= checkMask;

    return moves;
}

uint64_t Board::generateRookMoves(int square, char piece)
{

    uint64_t moves;
    uint64_t kingBoard = std::islower(piece) ? blackKing.bitboard : whiteKing.bitboard;
    int kingSquare = bitScanForward(kingBoard);
    char king = getPieceAtSquare(kingSquare);

    uint64_t pinnedPieces = findPinnedPieces(kingSquare, king);
    uint64_t checkMask;
    uint64_t checkers = findCheckers(kingSquare, king, checkMask);

    uint64_t blockers = getOccupiedSquares();
    blockers &= attackTable.rookMask[square];

    uint64_t hash = (blockers * attackTable.rookMagics[square]) >> (64 - attackTable.rookIndex[square]);

    uint64_t friendly;

    std::islower(piece) ? friendly = getBlackPieces() : friendly = getWhitePieces();
    moves = attackTable.rookTable[square][hash];

    if (pinnedPieces & (1ULL << square))
    {
        moves &= pinMasks[square];
    }
    if (checkers)
        moves &= checkMask;
    moves &= ~friendly;
    return moves;
}

uint64_t Board::generateRookMovesWithProtection(int square, char piece)
{

    uint64_t moves;

    uint64_t kingBoard = std::islower(piece) ? blackKing.bitboard : whiteKing.bitboard;
    int kingSquare = bitScanForward(kingBoard);
    char king = getPieceAtSquare(kingSquare);

    uint64_t checkMask;
    uint64_t checkers = findCheckers(kingSquare, king, checkMask);

    uint64_t blockers = getOccupiedSquares();
    uint64_t opponentKingBoard = std::islower(piece) ? whiteKing.bitboard : blackKing.bitboard;
    blockers &= ~opponentKingBoard;
    blockers &= attackTable.rookMask[square];

    uint64_t hash = (blockers * attackTable.rookMagics[square]) >> (64 - attackTable.rookIndex[square]);

    moves = attackTable.rookTable[square][hash];

    if (checkers)
        moves &= checkMask;

    return moves;
}

/**
 * @brief Generates all legal queen moves for a given square, considering the current board state.
 * 
 * This function generates all possible moves for a queen, combining rook-like (straight) and bishop-like (diagonal) moves.
 * It further applies additional constraints:
 * - **Pins**: If the queen is pinned relative to the king, its moves are limited to the set of allowed moves according to the pin.
 * - **Checks**: If the king is in check, only moves that help resolve the check (such as blocking or capturing a checking piece) are considered.
 * 
 * The function combines the legal straight and diagonal moves for the queen and applies these constraints.
 * 
 * @param square The starting square of the queen for which the legal moves are being generated.
 * @param piece The character representing the queen's piece ('Q' for white queen, 'q' for black queen).
 * 
 * @return A 64-bit integer representing the legal moves for the queen, with each bit corresponding to a square on the board.
 *         The set of moves is restricted by the current board state, considering pins, checks, and attacks.
 */
uint64_t Board::generateQueenMoves(int square, char piece)
{
    uint64_t moves;

    uint64_t kingBoard = std::islower(piece) ? blackKing.bitboard : whiteKing.bitboard;
    int kingSquare = bitScanForward(kingBoard);
    char king = getPieceAtSquare(kingSquare);

    uint64_t pinnedPieces = findPinnedPieces(kingSquare, king);
    uint64_t straightMoves = generateRookMoves(square, piece);
    uint64_t diagonalMoves = generateBishopMoves(square, piece);
    uint64_t checkMask;
    uint64_t checkers = findCheckers(kingSquare, king, checkMask);

    moves = straightMoves | diagonalMoves;

    if (pinnedPieces & (1ULL << square))
    {
        moves &= pinMasks[square];
    }

    if (checkers)
        moves &= checkMask;

    return moves;
}

/**
 * @brief Generates all legal queen moves including moving on to own pieces to simulate protection.
 * 
 * This function generates all possible queen moves.
 * 
 * - The function first calculates both the straight (rook-like) and diagonal (bishop-like) moves of the queen.
 * - It checks if the queen is pinned by any pieces relative to the king and adjusts its available moves accordingly.
 * - It also accounts for the presence of checkers, limiting the queen’s available moves to only those that do not leave the king in check.
 * 
 * The final set of legal moves is returned after considering all constraints. 
 * 
 * @param square The starting square of the queen for which legal moves are being generated.
 * @param piece The character representing the piece (e.g., 'Q' for white queen, 'q' for black queen).
 * 
 * @return A 64-bit integer representing the legal moves for the queen, with each bit corresponding to a square on the board.
 *         The set of moves is restricted by the current board state, taking into account pins, checks, and attacks.
 */
uint64_t Board::generateQueenMovesWithProtection(int square, char piece)
{
    uint64_t moves;

    uint64_t kingBoard = std::islower(piece) ? blackKing.bitboard : whiteKing.bitboard;
    int kingSquare = bitScanForward(kingBoard);
    char king = getPieceAtSquare(kingSquare);

    uint64_t pinnedPieces = findPinnedPieces(kingSquare, king);

    uint64_t straightMoves = generateRookMovesWithProtection(square, piece);
    uint64_t diagonalMoves = generateBishopMovesWithProtection(square, piece);

    uint64_t checkMask;
    uint64_t checkers = findCheckers(kingSquare, king, checkMask);

    moves = straightMoves | diagonalMoves;

    if (pinnedPieces & (1ULL << square))
    {
        moves &= pinMasks[square];
    }

    if (checkers)
        moves &= checkMask;
    return moves;
}

/**
 * @brief Generates all legal moves for the current player and organizes them into an array.
 * 
 * This function generates all legal moves for the given player (maximizing or minimizing) and categorizes 
 * them into three types: capture moves, non-capture moves, and pieces that are under attack.
 * The moves are sorted in a priority order, with captures and certain important moves, like castling, 
 * being prioritized first.
 * 
 * The generated moves are returned as an array of pairs of integers, where each pair represents a move 
 * from one square to another on the chessboard.
 * 
 * @param movesList A pre-allocated array of pairs that will hold the generated moves.
 * @param maximizingPlayer A boolean flag indicating whether the current player is maximizing or minimizing 
 *                         in the search tree (i.e., the current player’s side).
 * 
 * @return A pair containing two values:
 *         - The first value is the total number of legal moves (including captures, non-captures, and moves 
 *           where the piece is under attack).
 *         - The second value is the number of capture moves to indicate the end of capture moves.
 * 
 * The function prioritizes certain types of moves, such as capture moves and king castling moves.
 */

std::pair<int, int> Board::getAllLegalMovesAsArray(std::pair<int, int> movesList[], bool maximizingPlayer)
{
    int moveCount = 0;
    int captureCount = 0;
    int attackedCount = 0;

    std::pair<int, int> captureMoves[218];
    std::pair<int, int> nonCaptureMoves[218];
    std::pair<int, int> pieceUnderAttack[218];

    uint64_t AllPieces = maximizingPlayer ? getWhitePieces() : getBlackPieces();
    uint64_t opponentAttacks = getOpponentAttacks(maximizingPlayer ? 'P' : 'p');

    // Iterate over all pieces
    while (AllPieces)
    {
        int fromSquare = bitScanForward(AllPieces);
        char piece = getPieceAtSquare(fromSquare);
        uint64_t moves = 0;

        // Generate moves for each piece type
        switch (std::tolower(piece))
        {
        case 'p':
            moves = generatePawnMoves(fromSquare, piece);
            break;
        case 'b':
            moves = generateBishopMoves(fromSquare, piece);
            break;
        case 'r':
            moves = generateRookMoves(fromSquare, piece);
            break;
        case 'q':
            moves = generateQueenMoves(fromSquare, piece);
            break;
        case 'k':
            moves = generateKingMoves(fromSquare, piece);
            break;
        case 'n':
            moves = generateKnightMoves(fromSquare, piece);
            break;
        }

        // Process all the generated moves
        while (moves)
        {
            int toSquare = bitScanForward(moves);
            if (getPieceAtSquare(toSquare) != ' ')
                captureMoves[captureCount++] = {fromSquare, toSquare}; // It's a capture
            else if (opponentAttacks & (1ULL << fromSquare))
                pieceUnderAttack[attackedCount++] = {fromSquare, toSquare}; // It's a non-capture but the piece is under attack
            else
                nonCaptureMoves[moveCount++] = {fromSquare, toSquare}; // It's a non-capture

            moves &= moves - 1; // Clears the least significant bit (processed move)
        }

        AllPieces &= AllPieces - 1; // Clears the least significant bit (processed piece)
    }

    // Prioritize certain moves, especially those related to check or forced moves
    auto prioritizeMove = [&](std::pair<int, int> move) -> bool
    {
        int fromSquare = move.first;
        int toSquare = move.second;
        char piece = getPieceAtSquare(fromSquare);

       

        // Prioritize king castling moves
        if (piece == 'k' || piece == 'K')
        {
            if (fromSquare - move.second == 2 || fromSquare - move.second == -2) // Castling move
            {
                return true;
            }
        }

        return false; // Default case: no special prioritization
    };

    // First, add capture moves (prioritize them as they change the board state)
    for (int i = 0; i < captureCount; i++)
    {
        movesList[i] = captureMoves[i];
    }

    int nonCaptureIndex = captureCount;

    for (int i = 0; i < attackedCount; i++)
    {
        movesList[nonCaptureIndex++] = pieceUnderAttack[i];
    }

    // Process non-capture moves, prioritizing based on their "importance"
    for (int i = 0; i < moveCount; i++)
    {
        if (prioritizeMove(nonCaptureMoves[i]))
        {
            movesList[nonCaptureIndex++] = nonCaptureMoves[i];
        }
    }

    // Add remaining non-capture moves that were not prioritized
    for (int i = 0; i < moveCount; i++)
    {
        if (!prioritizeMove(nonCaptureMoves[i]))
        {
            movesList[nonCaptureIndex++] = nonCaptureMoves[i];
        }
    }

    return {captureCount + moveCount + attackedCount, captureCount};
}

/**
 * @brief Converts a move from square to square into its algebraic notation.
 * 
 * This function takes two integer indices representing the starting and destination squares on the chessboard 
 * and converts them into their corresponding algebraic notation. The notation is formatted as a string where 
 * the starting square is followed by the destination square, such as "e2e4".
 * 
 * The squares are converted from their integer indices (0-63) into algebraic notation based on the chessboard's 
 * file (a-h) and rank (1-8).
 * 
 * @param fromSquare The index (0-63) of the starting square.
 * @param toSquare The index (0-63) of the destination square.
 * 
 * @return A string representing the move in algebraic notation (e.g., "e2e4").
 */
std::string Board::moveToString(int fromSquare, int toSquare)
{

    auto squareToAlgebraic = [](int square) -> std::string
    {
        char file = 'a' + (square % 8);
        char rank = static_cast<char>('8' - (square / 8));
        return std::string(1, file) + std::string(1, rank);
    };

    std::string from = squareToAlgebraic(fromSquare);
    std::string to = squareToAlgebraic(toSquare);

    return from + to;
}

/**
 * @brief Generates all the attacks on the king by the opponent's pieces, without considering protection.
 * 
 * This function calculates all the squares on the board that are attacked by the opponent's pieces, 
 * excluding any pieces of the same color (based on the case of the `piece` parameter). It considers all 
 * types of opponent pieces, including pawns, rooks, knights, bishops, and queens, and generates their 
 * corresponding attack patterns.
 * 
 * The function does not take into account moves that protect your piece since you can not move on them.
 * 
 * @param piece The character representing the piece for which we are calculating the opponent's attacks ('K' or 'k').
 * 
 * @return A 64-bit bitboard where bits are set to 1 for squares attacked by the opponent's pieces, 
 *         and 0 for all other squares.
 */
uint64_t Board::getOpponentAttacks(char piece)
{
    uint64_t attacks = 0;
    bool isWhite;
    std::islower(piece) ? isWhite = 1 : isWhite = 0;

    for (auto i = 0; i < 64; i++)
    {
        char currentPiece = getPieceAtSquare(i);
        if (currentPiece == ' ' || (std::islower(currentPiece) && isWhite) || ((!std::islower(currentPiece) && !isWhite)))
        {
            continue;
        }
        if (std::tolower(currentPiece) == 'p')
        {
            attacks |= generatePawnMovesForKing(i, currentPiece);
        }

        if (std::tolower(currentPiece) == 'r')
        {
            attacks |= generateRookMoves(i, currentPiece);
        }

        if (std::tolower(currentPiece) == 'n')
        {
            attacks |= generateKnightMoves(i, currentPiece);
        }

        if (std::tolower(currentPiece) == 'b')
        {
            attacks |= generateBishopMoves(i, currentPiece);
        }
        if (std::tolower(currentPiece) == 'q')
        {
            attacks |= generateRookMoves(i, currentPiece);
            attacks |= generateBishopMoves(i, currentPiece);
        }
    }

    return attacks;
}

/**
 * @brief Generates all the attacks on the king by the opponent's pieces, including protected attacks.
 * 
 * This function calculates all the squares on the board that are attacked by the opponent's pieces, 
 * taking into account whether the attacking pieces are protected by other pieces. It considers all 
 * types of opponent pieces, including pawns, rooks, knights, bishops, and queens. For each piece, 
 * it generates its corresponding attack pattern and adds the protected piece to the attacks.
 * 
 * The function only considers pieces of the opposite color (based on the case of the `piece` parameter) 
 * and avoids processing friendly pieces or empty squares.
 * 
 * @param piece The character representing the piece for which we are calculating the opponent's attacks ('K' or 'k').
 * 
 * @return A 64-bit bitboard where bits are set to 1 for squares attacked by the opponent's pieces, including those 
 *         protected by other pieces, and 0 for all other squares.
 */
uint64_t Board::getOpponentAttacksWithProtection(char piece)
{
    uint64_t attacks = 0;
    bool isWhite = std::isupper(piece);

    for (auto i = 0; i < 64; i++)
    {
        char currentPiece = getPieceAtSquare(i);
        if (currentPiece == ' ' || (std::islower(currentPiece) && !isWhite) || ((!std::islower(currentPiece) && isWhite)))
        {
            continue;
        }
        if (std::tolower(currentPiece) == 'p')
        {
            attacks |= generatePawnMovesForKing(i, currentPiece);
        }

        if (std::tolower(currentPiece) == 'r')
        {
            attacks |= generateRookMovesWithProtection(i, currentPiece);
        }

        if (std::tolower(currentPiece) == 'n')
        {
            attacks |= generateKnightMovesWithProtection(i, currentPiece);
        }

        if (std::tolower(currentPiece) == 'b')
        {
            attacks |= generateBishopMovesWithProtection(i, currentPiece);
        }

        if (std::tolower(currentPiece) == 'q')
        {
            attacks |= generateRookMovesWithProtection(i, currentPiece);
            attacks |= generateBishopMovesWithProtection(i, currentPiece);
        }
    }

    return attacks;
}

/**
 * @brief Generates all legal moves for a king.
 * 
 * This function generates a bitboard representing all possible legal moves for a king 
 * from a given square. It considers the standard king moves, excluding those that would 
 * result in moving into squares occupied by friendly pieces or squares under attack by 
 * the opponent. Additionally, the function handles castling moves for both white and black kings 
 * if applicable.
 * 
 * The function also ensures that the king does not move into a square where it would be in check 
 * or move adjacent to the opponent's king.
 * 
 * @param square The index of the square the king is currently occupying (0 to 63).
 * @param piece The character representing the type of the king ('K' for white, 'k' for black).
 * 
 * @return A 64-bit bitboard representing the legal moves for the king. Bits are set to 1 for 
 *         valid move squares and 0 for invalid ones.
 */
uint64_t Board::generateKingMoves(int square, char piece)
{
    char king1 = getPieceAtSquare(square);
    uint64_t enemyKing = !std::islower(king1) ? blackKing.bitboard : whiteKing.bitboard;
    int enemyKingSquare = bitScanForward(enemyKing);
    uint64_t moves = 0;
    int rank = square / 8;
    int file = square % 8;
    // uint64_t fileMask = 0x0101010101010101ULL;
    // uint64_t rankMask = 0xFFULL;

    constexpr std::array<std::pair<int, int>, 8> KING_DIRECTIONS = {{{1, 0},
                                                                     {-1, 0},
                                                                     {0, 1},
                                                                     {0, -1},
                                                                     {1, 1},
                                                                     {1, -1},
                                                                     {-1, 1},
                                                                     {-1, -1}}};

    for (const auto &[dRank, dFile] : KING_DIRECTIONS)
    {
        int newRank = rank + dRank;
        int newFile = file + dFile;
        if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8)
        {
            int newSquare = newRank * 8 + newFile;
            moves |= (1ULL << newSquare);
        }
    }

    constexpr std::array<std::pair<int, int>, 8> ENEMY_KING_DIRECTIONS = {{{1, 0},
                                                                           {-1, 0},
                                                                           {0, 1},
                                                                           {0, -1},
                                                                           {1, 1},
                                                                           {1, -1},
                                                                           {-1, 1},
                                                                           {-1, -1}}};
    uint64_t enemyKingMoves = 0;

    for (const auto &[dRank, dFile] : ENEMY_KING_DIRECTIONS)
    {
        int newRank = (enemyKingSquare / 8) + dRank;
        int newFile = (enemyKingSquare % 8) + dFile;
        if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8)
        {
            int newSquare = newRank * 8 + newFile;
            enemyKingMoves |= (1ULL << newSquare);
        }
    }

    uint64_t occupied = getOccupiedSquares();
    uint64_t friendly;
    char king = getPieceAtSquare(square);

    friendly = std::islower(king) ? getBlackPieces() : getWhitePieces();
    moves &= ~friendly;
    uint64_t opponentAttacks = getOpponentAttacksWithProtection(piece);
    moves &= ~opponentAttacks;
    moves &= ~enemyKingMoves;

    if (piece == 'k' && square == 4)
    { // White King on e1
        if (blackCanCastleK &&
            !(occupied & (1ULL << 5)) && !(occupied & (1ULL << 6)) &&
            !(opponentAttacks & (1ULL << 4)) &&
            !(opponentAttacks & (1ULL << 5)) &&
            !(opponentAttacks & (1ULL << 6)) &&
            getPieceAtSquare(7) == 'r')
        {
            moves |= (1ULL << 6);
        }
        if (blackCanCastleQ &&
            !(occupied & (1ULL << 1)) && !(occupied & (1ULL << 2)) && !(occupied & (1ULL << 3)) &&
            !(opponentAttacks & (1ULL << 4)) &&
            !(opponentAttacks & (1ULL << 3)) &&
            !(opponentAttacks & (1ULL << 2)) &&
            getPieceAtSquare(0) == 'r')
        {
            moves |= (1ULL << 2);
        }
    }
    else if (piece == 'K' && square == 60)
    {

        if (WhiteCanCastleK &&
            !(occupied & (1ULL << 61)) && !(occupied & (1ULL << 62)) &&
            !(opponentAttacks & (1ULL << 60)) &&
            !(opponentAttacks & (1ULL << 61)) &&
            !(opponentAttacks & (1ULL << 62)) &&
            getPieceAtSquare(63) == 'R')
        {
            moves |= (1ULL << 62);
        }
        if (WhiteCanCastleQ &&
            !(occupied & (1ULL << 57)) && !(occupied & (1ULL << 58)) && !(occupied & (1ULL << 59)) &&
            !(opponentAttacks & (1ULL << 60)) &&
            !(opponentAttacks & (1ULL << 58)) &&
            getPieceAtSquare(56) == 'R')
        {
            moves |= (1ULL << 58);
        }
    }

    return moves;
}

/**
 * @brief Gets the bitboard for all white pieces.
 * 
 * This function returns a bitboard representing all the white pieces on the chessboard. 
 * It combines the bitboards of white pawns, knights, bishops, rooks, queens, and the king 
 * to generate the bitboard of all white pieces.
 * 
 * @return A 64-bit bitboard with bits set to 1 for squares occupied by white pieces and 
 *         0 for empty squares.
 */
uint64_t Board::getWhitePieces()
{
    uint64_t whitePieces = 0;

    whitePieces |= whitePawns.bitboard;
    whitePieces |= whiteKnights.bitboard;
    whitePieces |= whiteBishops.bitboard;
    whitePieces |= whiteRooks.bitboard;
    whitePieces |= whiteQueens.bitboard;
    whitePieces |= whiteKing.bitboard;

    return whitePieces;
}

/**
 * @brief Gets the bitboard for all black pieces.
 * 
 * This function returns a bitboard representing all the black pieces on the chessboard. 
 * It combines the bitboards of black pawns, knights, bishops, rooks, queens, and the king 
 * to generate the bitboard of all black pieces.
 * 
 * @return A 64-bit bitboard with bits set to 1 for squares occupied by black pieces and 
 *         0 for empty squares.
 */
uint64_t Board::getBlackPieces()
{
    uint64_t blackPieces = 0;

    blackPieces |= blackPawns.bitboard;
    blackPieces |= blackKnights.bitboard;
    blackPieces |= blackBishops.bitboard;
    blackPieces |= blackRooks.bitboard;
    blackPieces |= blackQueens.bitboard;
    blackPieces |= blackKing.bitboard;

    return blackPieces;
}

/**
 * @brief Gets the occupied squares on the board.
 * 
 * This function returns a bitboard representing all the squares currently occupied by 
 * pieces on the chessboard. It combines the bitboards of all pieces (both white and black), 
 * including pawns, knights, bishops, rooks, queens, and kings, to calculate the bitboard 
 * of occupied squares.
 * 
 * @return A 64-bit bitboard with bits set to 1 for occupied squares and 0 for empty squares.
 */
uint64_t Board::getOccupiedSquares()
{
    uint64_t occupied = 0;

    occupied |= whitePawns.bitboard;
    occupied |= blackPawns.bitboard;
    occupied |= whiteKnights.bitboard;
    occupied |= blackKnights.bitboard;
    occupied |= whiteBishops.bitboard;
    occupied |= blackBishops.bitboard;
    occupied |= whiteRooks.bitboard;
    occupied |= blackRooks.bitboard;
    occupied |= whiteQueens.bitboard;
    occupied |= blackQueens.bitboard;
    occupied |= whiteKing.bitboard;
    occupied |= blackKing.bitboard;

    return occupied;
}

/**
 * @brief Gets the empty squares on the board.
 * 
 * This function returns a bitboard representing all the empty squares on the chessboard. 
 * It does so by taking the bitboard of all occupied squares and inverting it (using bitwise NOT) 
 * to yield the bitboard of empty squares.
 * 
 * @return A 64-bit bitboard with bits set to 1 for empty squares and 0 for occupied squares.
 */
uint64_t Board::getEmptySquares()
{
    return ~getOccupiedSquares();
}

/**
 * @brief Updates the bitboards for a piece movement.
 * 
 * This function updates the bitboards for a specific piece after it has moved from the 
 * 'from' square to the 'to' square. It clears the bit in the bitboard of the piece 
 * from the 'from' square and sets the bit in the bitboard of the piece at the 'to' square.
 * The function handles updates for both white and black pieces, including pawns, knights, 
 * bishops, rooks, queens, and kings.
 * 
 * @param piece The piece being moved ('P' for white pawn, 'p' for black pawn, 'N' for white knight, 
 *              'n' for black knight, 'B' for white bishop, 'b' for black bishop, 'R' for white rook, 
 *              'r' for black rook, 'Q' for white queen, 'q' for black queen, 'K' for white king, 
 *              'k' for black king).
 * @param from The index of the piece's current position (0 to 63).
 * @param to The index of the target square the piece is moving to (0 to 63).
 * 
 * @return true If the bitboard was successfully updated, false otherwise.
 */
bool Board::updateBitboards(char piece, int from, int to)
{
    if (piece == 'P' && whitePawns.isSet(from))
    {
        whitePawns.clearSquare(from);
        whitePawns.setSquare(to);
        return true;
    }
    else if (piece == 'p' && blackPawns.isSet(from))
    {
        blackPawns.clearSquare(from);
        blackPawns.setSquare(to);
        return true;
    }
    else if (piece == 'N' && whiteKnights.isSet(from))
    {
        whiteKnights.clearSquare(from);
        whiteKnights.setSquare(to);
        return true;
    }
    else if (piece == 'n' && blackKnights.isSet(from))
    {
        blackKnights.clearSquare(from);
        blackKnights.setSquare(to);
        return true;
    }
    else if (piece == 'B' && whiteBishops.isSet(from))
    {
        whiteBishops.clearSquare(from);
        whiteBishops.setSquare(to);
        return true;
    }
    else if (piece == 'b' && blackBishops.isSet(from))
    {
        blackBishops.clearSquare(from);
        blackBishops.setSquare(to);
        return true;
    }
    else if (piece == 'R' && whiteRooks.isSet(from))
    {
        whiteRooks.clearSquare(from);
        whiteRooks.setSquare(to);
        return true;
    }
    else if (piece == 'r' && blackRooks.isSet(from))
    {
        blackRooks.clearSquare(from);
        blackRooks.setSquare(to);
        return true;
    }
    else if (piece == 'Q' && whiteQueens.isSet(from))
    {
        whiteQueens.clearSquare(from);
        whiteQueens.setSquare(to);
        return true;
    }
    else if (piece == 'q' && blackQueens.isSet(from))
    {
        blackQueens.clearSquare(from);
        blackQueens.setSquare(to);
        return true;
    }
    else if (piece == 'K' && whiteKing.isSet(from))
    {
        whiteKing.clearSquare(from);
        whiteKing.setSquare(to);
        return true;
    }
    else if (piece == 'k' && blackKing.isSet(from))
    {
        blackKing.clearSquare(from);
        blackKing.setSquare(to);
        return true;
    }

    return false;
}

/**
 * @brief Checks if the pawn's move is legal.
 * 
 * This function checks if a move from the 'from' square to the 'to' square is a valid move 
 * for the pawn. It generates all legal pawn moves from the 'from' position and verifies 
 * whether the 'to' square is included in those valid moves.
 * 
 * @param from The index of the pawn's current position (0 to 63).
 * @param to The index of the target square the pawn is attempting to move to (0 to 63).
 * 
 * @return true If the move from 'from' to 'to' is legal for the pawn, false otherwise.
 */
bool Board::legalPawnMove(int from, int to)
{
    uint64_t moves = generatePawnMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

/**
 * @brief Checks if the knight's move is legal.
 * 
 * This function checks if a move from the 'from' square to the 'to' square is a valid move 
 * for the knight. It generates all legal knight moves from the 'from' position and verifies 
 * whether the 'to' square is included in those valid moves.
 * 
 * @param from The index of the knight's current position (0 to 63).
 * @param to The index of the target square the knight is attempting to move to (0 to 63).
 * 
 * @return true If the move from 'from' to 'to' is legal for the knight, false otherwise.
 */
bool Board::legalKnightMove(int from, int to)
{
    uint64_t moves = generateKnightMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

/**
 * @brief Checks if the bishop's move is legal.
 * 
 * This function checks if a move from the 'from' square to the 'to' square is a valid move 
 * for the bishop. It generates all legal bishop moves from the 'from' position and verifies 
 * whether the 'to' square is included in those valid moves.
 * 
 * @param from The index of the bishop's current position (0 to 63).
 * @param to The index of the target square the bishop is attempting to move to (0 to 63).
 * 
 * @return true If the move from 'from' to 'to' is legal for the bishop, false otherwise.
 */
bool Board::legalBishopMove(int from, int to)
{
    uint64_t moves = generateBishopMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

/**
 * @brief Checks if the rook's move is legal.
 * 
 * This function checks if a move from the 'from' square to the 'to' square is a valid move 
 * for the rook. It generates all legal rook moves from the 'from' position and verifies 
 * whether the 'to' square is included in those valid moves.
 * 
 * @param from The index of the rook's current position (0 to 63).
 * @param to The index of the target square the rook is attempting to move to (0 to 63).
 * 
 * @return true If the move from 'from' to 'to' is legal for the rook, false otherwise.
 */
bool Board::legalRookMove(int from, int to)
{
    uint64_t moves = generateRookMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

/**
 * @brief Checks if the queen's move is legal.
 * 
 * This function checks if a move from the 'from' square to the 'to' square is a valid move 
 * for the queen. It generates all legal queen moves from the 'from' position and verifies 
 * whether the 'to' square is included in those valid moves.
 * 
 * @param from The index of the queen's current position (0 to 63).
 * @param to The index of the target square the queen is attempting to move to (0 to 63).
 * 
 * @return true If the move from 'from' to 'to' is legal for the queen, false otherwise.
 */
bool Board::legalQueenMove(int from, int to)
{
    uint64_t moves = generateQueenMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

/**
 * @brief Checks if the king's move is legal.
 * 
 * This function checks if a move from the 'from' square to the 'to' square is a valid move 
 * for the king. It generates all legal king moves from the 'from' position and verifies 
 * whether the 'to' square is included in those valid moves.
 * 
 * @param from The index of the king's current position (0 to 63).
 * @param to The index of the target square the king is attempting to move to (0 to 63).
 * 
 * @return true If the move from 'from' to 'to' is legal for the king, false otherwise.
 */
bool Board::legalKingMove(int from, int to)
{
    uint64_t moves = generateKingMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

/**
 * @brief Prints a bitboard representation.
 * 
 * @param bitboard The 64-bit bitboard to print.
 * @param label A string label describing the bitboard.
 */
void Board::printBitboard(uint64_t bitboard, const std::string &label)
{
    std::cout << label << ":\n";
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;
            std::cout << ((bitboard >> square) & 1 ? "1 " : ". ");
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

/**
 * @brief Finds the index of the least significant set bit (LSB) in a 64-bit bitboard.
 * 
 * Uses hardware intrinsics for fast bit scanning.
 * 
 * @param bitboard The 64-bit integer representing the bitboard.
 * @return int Index of the least significant set bit (0-63), or -1 if bitboard is 0.
 */
int Board::bitScanForward(uint64_t bitboard)
{
    if (bitboard == 0)
    {
        return -1;
    }
    unsigned long index;
    if (_BitScanForward64(&index, bitboard))
    {
        return index;
    }
    return -1;
}

/**
 * @brief Converts a chess board square notation (e.g., "e2") into a 0-based index.
 * 
 * The function validates and converts a chess square (e.g., "a1", "h8") into 
 * a single integer index (0 to 63) representing its position on an 8x8 board.
 * 
 * @param square A 2-character string representing a chessboard square (e.g., "e2").
 * @return int The 0-based index (0-63) if valid, or -1 if the input is invalid.
 */
int Board::getSquareIndex(const std::string &square)
{
    if (square.length() != 2)
        return -1;

    char file = square[0];
    char rank = square[1];

    int fileIndex = file - 'a';
    int rankIndex = '8' - rank;

    if (fileIndex < 0 || fileIndex > 7 || rankIndex < 0 || rankIndex > 7)
        return -1;

    return rankIndex * 8 + fileIndex;
}

/**
 * @brief Parses a chess move string and converts it into board indices.
 * 
 * This function takes a move string in standard algebraic notation (e.g., "e2e4"),
 * extracts the starting and destination squares, and converts them into 
 * numerical indices using `getSquareIndex()`.
 * 
 * @param move A 4-character chess move string (e.g., "e2e4").
 * @return std::optional<std::pair<int, int>> 
 *         - A pair {from, to} if parsing is successful.
 *         - std::nullopt if the input is invalid (wrong length or invalid squares).
 */
std::pair<int, int> Board::parseMove(const std::string &move)
{
    if (move.length() != 4)
        return {-1, -1};

    int from = getSquareIndex(move.substr(0, 2));
    int to = getSquareIndex(move.substr(2, 2));

    return {from, to};
}





