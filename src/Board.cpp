#include "Board.h"
#include <iostream>
#include <bitset>
#include <array>
#include <sstream>
#include <unordered_map>

// 8/8/8/8/8/8/8/KR6 b - - 0 1
// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
// 7r/8/8/k7/r7/8/8/pK6 b - - 0 1

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
    initTranspositionTable();
}
Board::Board(const std::shared_ptr<Board>& other1)
{
    const Board& other = *other1;
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
    this->transpositionTable = other.transpositionTable;
    std::copy(std::begin(other.kingMovesTable), std::end(other.kingMovesTable), std::begin(this->kingMovesTable));
    std::copy(std::begin(other.pinMasks), std::end(other.pinMasks), std::begin(this->pinMasks));

    // Initialize any other members (like AttackTable) if necessary
    this->attackTable.initialize();
    
    // Reinitialize Zobrist if required, since we have just copied the data
    this->initializeZobrist();
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

void Board::initTranspositionTable()
{
    transpositionTable.resize(TABLE_SIZE);
}

bool Board::probeTranspositionTable(uint64_t hash, int depth, int alpha, int beta, std::pair<int, std::pair<int, int>> &result)
{
    TranspositionEntry &entry = transpositionTable[hash % TABLE_SIZE];

    if (entry.zobristKey == hash && entry.depth >= depth)
    {
        if (entry.flag == 1)
        {
            result = {entry.evaluation, {entry.bestFrom, entry.bestTo}};
            return true;
        }
        if (entry.flag == 0 && entry.evaluation <= alpha)
        {
            result = {entry.evaluation, {entry.bestFrom, entry.bestTo}};
            return true;
        }
        if (entry.flag == -1 && entry.evaluation >= beta)
        {
            result = {entry.evaluation, {entry.bestFrom, entry.bestTo}};
            return true;
        }
    }
    return false;
}

// Store position in transposition table
void Board::storeTransposition(uint64_t hash, int depth, int eval, int alpha, int beta, int from, int to)
{
    TranspositionEntry &entry = transpositionTable[hash % TABLE_SIZE];

    if (entry.depth < depth || entry.zobristKey == 0)
    {
        entry.zobristKey = hash;
        entry.evaluation = eval;
        entry.depth = depth;
        entry.bestFrom = from;
        entry.bestTo = to;
        entry.flag = (eval <= alpha) ? -1 : (eval >= beta) ? 1
                                                           : 0;
    }
}

void Board::storeMove(int from, int to, char capturedPiece, uint64_t enpSquare,
                      bool wasEnPassant, int enPassantCapturedSquare,
                      char enPassantCapturedPiece, bool wasPromotion,
                      char originalPawn)
{
    moveHistory.push_back({from, to, capturedPiece, enpSquare,
                           wasEnPassant, enPassantCapturedSquare, enPassantCapturedPiece, wasPromotion, originalPawn});
}

Move Board::getLastMove()
{
    if (moveHistory.empty())
    {
        return Move{-1, -1, ' ', false, false, -1, false};
    }
    Move lastMove = moveHistory.back();
    moveHistory.pop_back();
    return lastMove;
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
        int moveCount = getAllLegalMovesAsArray(legalMoves, maximizingPlayer);

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
}

void Board::undoMove(int from, int to, char capturedPiece, uint64_t enpSquare,
                     bool lastMoveEnPassant1, int enpassantCapturedSquare1,
                     char enpassantCapturedPiece1, bool wasPromotion,
                     char originalPawn)
{
    char movedPiece = getPieceAtSquare(to);
    if (std::tolower(movedPiece) == 'k')
    {
        if (std::isupper(movedPiece))
        {
            WhiteCanCastleK = true;
            WhiteCanCastleQ = true;
        }
        else
        {
            blackCanCastleK = true;
            blackCanCastleQ = true;
        }
    }
    else if (std::tolower(movedPiece) == 'r')
    {
        if (from == 56)
            WhiteCanCastleQ = true;
        if (from == 63)
            WhiteCanCastleK = true;
        if (from == 0)
            blackCanCastleQ = true;
        if (from == 7)
            blackCanCastleK = true;
    }

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
        else
            return;

        updateBitboards(movedPiece, to, from);

        updateBitboards(getPieceAtSquare(rookFrom), rookFrom, rookTo);

        if (!std::isupper(movedPiece))
        {
            if (from == 4 && to == 6)
                WhiteCanCastleK = true;
            if (from == 4 && to == 2)
                WhiteCanCastleQ = true;
        }
        else
        {
            if (from == 60 && to == 62)
                blackCanCastleK = true;
            if (from == 60 && to == 58)
                blackCanCastleQ = true;
        }
        return;
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

    this->enPassantTarget = enpSquare;
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

    char piece = getPieceAtSquare(from);
    char destPiece = getPieceAtSquare(to);

    /*if (piece == ' ')
    {
        return false;
    }*/

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
              enpassantCapturedPiece, wasPromotion, piece);

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

    if (enPassantTarget)
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

int Board::getAllLegalMovesAsArray(std::pair<int, int> movesList[], bool maximizingPlayer)
{
    int moveCount = 0;
    int captureCount = 0; 

    std::pair<int, int> captureMoves[218];    
    std::pair<int, int> nonCaptureMoves[218]; 

    uint64_t AllPieces = maximizingPlayer ? getWhitePieces() : getBlackPieces();
    
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
                captureMoves[captureCount++] = {fromSquare, toSquare};  // It's a capture
            else
                nonCaptureMoves[moveCount++] = {fromSquare, toSquare};  // It's a non-capture

            moves &= moves - 1; // Clears the least significant bit (processed move)
        }

        AllPieces &= AllPieces - 1;  // Clears the least significant bit (processed piece)
    }

    // Prioritize certain moves, especially those related to check or forced moves
    auto prioritizeMove = [&](std::pair<int, int> move) -> bool
    {
        int fromSquare = move.first;
        char piece = getPieceAtSquare(fromSquare);

        // Prioritize knight moves that are attacking near the edges of the board
        if (piece == 'n' || piece == 'N')
        {
            if ((maximizingPlayer && (fromSquare == 1 || fromSquare == 6)) ||
                (!maximizingPlayer && (fromSquare == 57 || fromSquare == 62)))
            {
                return true; 
            }
        }

        // Prioritize king castling moves
        if (piece == 'k' || piece == 'K')
        {
            if (fromSquare - move.second == 2 || fromSquare - move.second == -2)  // Castling move
            {
                return true; 
            }
        }

        return false;  // Default case: no special prioritization
   
    };

    // First, add capture moves (prioritize them as they change the board state)
    for (int i = 0; i < captureCount; i++)
    {
        movesList[i] = captureMoves[i];
    }

    int nonCaptureIndex = captureCount;

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

    return captureCount + moveCount;  // Return the total number of moves
}


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

    
    constexpr std::array<std::pair<int, int>, 8> KING_DIRECTIONS = {{
        {1, 0},  
        {-1, 0}, 
        {0, 1},  
        {0, -1}, 
        {1, 1},  
        {1, -1}, 
        {-1, 1}, 
        {-1, -1} 
    }};

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

    constexpr std::array<std::pair<int, int>, 8> ENEMY_KING_DIRECTIONS = {{
        {1, 0},  
        {-1, 0}, 
        {0, 1},  
        {0, -1}, 
        {1, 1},  
        {1, -1}, 
        {-1, 1}, 
        {-1, -1} 
    }};
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

uint64_t Board::getEmptySquares()
{
    return ~getOccupiedSquares(); 
}


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

bool Board::legalPawnMove(int from, int to)
{
    uint64_t moves = generatePawnMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

bool Board::legalKnightMove(int from, int to)
{
    uint64_t moves = generateKnightMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

bool Board::legalBishopMove(int from, int to)
{
    uint64_t moves = generateBishopMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

bool Board::legalRookMove(int from, int to)
{
    uint64_t moves = generateRookMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

bool Board::legalQueenMove(int from, int to)
{
    uint64_t moves = generateQueenMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

bool Board::legalKingMove(int from, int to)
{
    uint64_t moves = generateKingMoves(from, getPieceAtSquare(from));
    return moves & (1ULL << to);
}

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

std::pair<int, int> Board::parseMove(const std::string &move)
{
    if (move.length() != 4)
        return {-1, -1};

    int from = getSquareIndex(move.substr(0, 2)); 
    int to = getSquareIndex(move.substr(2, 2));   

    return {from, to};
}

int Board::evaluatePosition()
{
    int materialScore = 0;

    return materialScore;
}

int Board::evaluateMaterial()
{
    int score = 0;


    score += evaluatePieceSet(whitePawns, 1); 
    score += evaluatePieceSet(blackPawns, -1);
    score += evaluatePieceSet(whiteKnights, 3); 
    score += evaluatePieceSet(blackKnights, -3);
    score += evaluatePieceSet(whiteBishops, 3); 
    score += evaluatePieceSet(blackBishops, -3);
    score += evaluatePieceSet(whiteRooks, 5); 
    score += evaluatePieceSet(blackRooks, -5);
    score += evaluatePieceSet(whiteQueens, 9); 
    score += evaluatePieceSet(blackQueens, -9);
    score += evaluatePieceSet(whiteKing, 0); 
    score += evaluatePieceSet(blackKing, 0);

    return score;
}

int Board::evaluatePieceSet(Bitboard &bitboard, int value)
{
    uint64_t bitMask = bitboard.bitboard;
    int count = 0;

    while (bitMask)
    {
        count++;
        bitMask &= bitMask - 1;
    }

    return count * value;
}
