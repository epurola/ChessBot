#include "Board.h"

/*DEBUG FEN*/
// 8/8/8/8/8/8/8/KR6 b - - 0 1
// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
// 7r/8/8/k7/r7/8/8/pK6 b - - 0 1
// 4r1k1/1q1b1pb1/1n1p1npp/2pPp3/2P1P3/2BB1N1P/2QN1PP1/R5K1 b - - 3 25

/**
 * @brief Default constructor for the Board class.
 *
 * Initializes the board to the standard starting position and resets relevant game states.
 */
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
    allPieces = getBlackPieces() | getWhitePieces();
}

/**
 * @brief Copy constructor for the Board class.
 *
 * Initializes a new board instance by copying all properties and states from the given Board object.
 */
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
    this->moveHistory = other.moveHistory;
    std::copy(std::begin(other.kingMovesTable), std::end(other.kingMovesTable), std::begin(this->kingMovesTable));
    std::copy(std::begin(other.pinMasks), std::end(other.pinMasks), std::begin(this->pinMasks));
    this->attackTable.initialize();
    this->initializeZobrist();
    allPieces = getBlackPieces() | getWhitePieces();
}

/**
 * @brief Resets the board to the initial chess position.
 *
 * This function sets the board to the standard starting position, resets castling rights,
 * en passant state, and clears move history. It also recalculates the Zobrist hash and
 * reinitializes related structures.
 */
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
    allPieces = getBlackPieces() | getWhitePieces();
}

/**
 * @brief Checks if the current position has occurred three times (threefold repetition).
 *
 * This function computes the Zobrist hash for the current board position and checks if it has appeared
 * in the `gameFensHistory` at least twice (indicating the position has been repeated three times,
 * including the current occurrence).
 *
 * @return `true` if the position has occurred three times, `false` otherwise.
 */
bool Board::isThreefoldRepetition()
{
    computeZobristHash();

    uint64_t hash = getZobristHash();
    if (gameFensHistory[hash] >= 2)
    {
        return true;
    }
    return false;
}

/**
 * @brief Initializes the Zobrist hashing table with random values.
 *
 * This function initializes the Zobrist hash table with random values used for efficient position hashing.
 * Zobrist hashing assigns a unique hash value to each possible combination of piece and square on the board,
 * as well as to special board states like castling rights, en passant, and the side to move. The precomputed
 * hash values are used in the search algorithm to quickly determine if a position has been encountered before,
 * improving performance by reducing redundant calculations (via transposition tables).
 *
 * The Zobrist table is populated with random 64-bit values using a pseudo-random number generator (`std::mt19937_64`)
 * seeded with a fixed value (123456789) for consistency across runs. The `std::uniform_int_distribution` is used
 * to generate random values in the full range of 64-bit unsigned integers (0 to UINT64_MAX).
 *
 * Additionally, the function initializes the following tables:
 * - `zobristTable`: A table containing random hash values for each piece on each square (pieces are indexed 0-11, squares 0-63).
 * - `castlingTable`: A table containing random hash values for the four possible castling rights.
 * - `enPassantTable`: A table containing random hash values for the en passant target files (0-7).
 * - `sideToMoveHash`: A random hash for the side to move, determining whether it's white or black's turn to move.
 *
 * This function is typically called once during the setup of the board to initialize the Zobrist hashing system.
 */
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

/**
 * @brief Converts a piece character to its corresponding index in the Zobrist hash table.
 *
 * This function maps a chess piece character (e.g., 'P' for white pawn, 'n' for black knight) to an index
 * used in the Zobrist hash table. The index corresponds to the piece type and color. The index is used to
 * retrieve the appropriate precomputed hash value for that piece on a given square in the `zobristTable`.
 *
 * The function uses the following mapping:
 * - 'P' -> 0 (White Pawn)
 * - 'N' -> 1 (White Knight)
 * - 'B' -> 2 (White Bishop)
 * - 'R' -> 3 (White Rook)
 * - 'Q' -> 4 (White Queen)
 * - 'K' -> 5 (White King)
 * - 'p' -> 6 (Black Pawn)
 * - 'n' -> 7 (Black Knight)
 * - 'b' -> 8 (Black Bishop)
 * - 'r' -> 9 (Black Rook)
 * - 'q' -> 10 (Black Queen)
 * - 'k' -> 11 (Black King)
 *
 * If the input piece is not a valid chess piece (e.g., an empty square or an invalid character), the function
 * returns -1, indicating an invalid piece.
 *
 * @param piece The piece character to be mapped to an index.
 * @return The index corresponding to the piece in the Zobrist hash table, or -1 if the piece is invalid.
 */
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

/**
 * @brief Computes the Zobrist hash for the current board position.
 *
 * This function generates a unique hash for the current board position using the Zobrist hashing technique.
 * The Zobrist hash is a bitwise XOR of the hash values associated with each piece on the board, the side to
 * move, castling rights, and the en passant target. This hash can be used for efficient transposition table
 * lookups in search algorithms, such as alpha-beta pruning, allowing previously encountered positions to be
 * recognized and reused, improving performance by avoiding redundant evaluations.
 *
 * The hash is calculated as follows:
 * - XOR the hash values for each piece on the board (each square and piece type).
 * - XOR the hash for the side to move (white or black).
 * - XOR the hash values for the castling rights of both players.
 * - XOR the hash for the en passant target square, if applicable.
 *
 * The final Zobrist hash value is stored in `zobristHash` and returned.
 *
 * @return The computed Zobrist hash for the current board position.
 *
 * @details
 * - `zobristTable` contains the precomputed hash values for each piece type on each square.
 * - `sideToMoveHash` is the hash for the side to move (white or black).
 * - `castlingTable` contains precomputed hashes for the castling rights of both players.
 * - `enPassantTable` contains precomputed hashes for each possible en passant square.
 */
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

/**
 * @brief Probes the transposition table for a stored entry that matches the current position.
 *
 * This function checks the transposition table for a previously stored evaluation of the current position
 * based on the Zobrist hash. If a valid entry is found that meets the required conditions (e.g., depth
 * sufficient and the evaluation being within alpha-beta bounds), the stored entry is returned, and the
 * function indicates a successful probe by returning `true`. Otherwise, it returns `false` to indicate
 * that no usable entry was found.
 *
 * The transposition table is a cache of previously evaluated positions that helps speed up the search
 * by avoiding redundant calculations for positions that have already been evaluated.
 *
 * @param hash The Zobrist hash of the current board position.
 * @param depth The current search depth at which the position is being evaluated.
 * @param alpha The alpha value from the alpha-beta pruning search, representing the best score the maximizer can guarantee.
 * @param beta The beta value from the alpha-beta pruning search, representing the best score the minimizer can guarantee.
 * @param entry The reference to a `TTEntry` structure that will be populated with the result of the probe if successful.
 *
 * @return `true` if a valid transposition entry is found in the table, otherwise `false`.
 *
 * @details
 * - The function first checks if the hash in the transposition table matches the current position's hash.
 * - It then checks if the depth of the stored entry is at least as large as the current depth, ensuring the entry is relevant.
 * - The function returns `true` if the entry's evaluation is exact or if the evaluation is within the alpha-beta bounds (lowerbound or upperbound).
 * - If a valid entry is found, the `entry` parameter is populated with the data from the transposition table.
 */
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

/**
 * @brief Stores a transposition entry in the transposition table.
 *
 * This function stores a transposition entry in the transposition table, which is used to cache previously
 * evaluated positions during the search. The transposition table helps to avoid recalculating evaluations
 * for previously visited positions, improving search efficiency in algorithms like alpha-beta pruning.
 *
 * The function records the hash of the position, the evaluation score, the depth at which the position was
 * evaluated, and the best move (from and to squares) for the current position. The entry is also tagged with
 * an appropriate flag (e.g., UPPERBOUND, LOWERBOUND, or EXACT) based on the evaluation and alpha-beta bounds.
 *
 * @param hash The Zobrist hash of the position.
 * @param depth The search depth at which the position was evaluated.
 * @param eval The evaluation score of the position.
 * @param alpha The alpha value in alpha-beta pruning, representing the lower bound of the search.
 * @param beta The beta value in alpha-beta pruning, representing the upper bound of the search.
 * @param from The starting square of the best move found.
 * @param to The destination square of the best move found.
 *
 * @details
 * The function stores the transposition table entry with the following flags:
 * - **UPPERBOUND**: The evaluation is worse than or equal to the beta value, meaning we cannot improve the result.
 * - **LOWERBOUND**: The evaluation is better than or equal to the alpha value, meaning we cannot worsen the result.
 * - **EXACT**: The evaluation is within the bounds, indicating that the exact evaluation is known.
 *
 * This cached information is later used during the search to prune the search tree, potentially reducing the
 * number of nodes explored and speeding up the search.
 */
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

/**
 * @brief Stores a move in the move history.
 *
 * This function stores a move in the move history array. It records the details of the move, including the
 * starting and ending squares, any captured piece, en passant information, castling rights, and other relevant
 * game state details at the time of the move. The function also handles the possibility of move history overflow
 * by throwing an exception when the maximum allowed number of moves is reached.
 *
 * @param from The starting square of the move (0-63).
 * @param to The destination square of the move (0-63).
 * @param capturedPiece The piece that was captured, if any (represented as a character, e.g., 'p' for pawn).
 * @param enpSquare The square where en passant is available, or 0 if not applicable.
 * @param wasEnPassant Flag indicating whether the move was an en passant capture.
 * @param enPassantCapturedSquare The square of the captured pawn in case of en passant, or -1 if not applicable.
 * @param enPassantCapturedPiece The piece captured by en passant, or ' ' if no capture occurred.
 * @param wasPromotion Flag indicating whether the move involved a pawn promotion.
 * @param originalPawn The original pawn that was promoted (if applicable).
 * @param WhiteCastleKBefore Whether White could castle kingside before the move.
 * @param WhiteCastleQBefore Whether White could castle queenside before the move.
 * @param BlackCastleKBefore Whether Black could castle kingside before the move.
 * @param BlackCastleQBefore Whether Black could castle queenside before the move.
 * @param hash The Zobrist hash of the board state before the move.
 * @param whiteTurn A boolean indicating whether it is White's turn to move after this move.
 *
 * @throws std::runtime_error if the move history is full and no more moves can be stored.
 *
 * The move history is stored in the `moveHistory` array, and `moveCount` keeps track of the number of moves
 * that have been made. The function will throw a runtime exception if `moveCount` exceeds the `MAX_MOVES` limit,
 * ensuring that the move history does not exceed its allocated size.
 */
void Board::storeMove(int from, int to, char capturedPiece, uint64_t enpSquare,
                      bool wasEnPassant, int enPassantCapturedSquare,
                      char enPassantCapturedPiece, bool wasPromotion,
                      char originalPawn, bool WhiteCastleKBefore,
                      bool WhiteCastleQBefore, bool BlackCastleKBefore,
                      bool BlackCastleQBefore, uint64_t hash,
                      bool whiteTurn)
{
    if (moveCount >= MAX_MOVES)
    {
        // You can choose to throw an exception or handle overflow as needed.
        throw std::runtime_error("Move history is full!");
    }

    // Create and store the move
    moveHistory[moveCount++] = LastMove{
        from, to, capturedPiece, enpSquare, wasEnPassant,
        enPassantCapturedSquare,
        enPassantCapturedPiece,
        wasPromotion,
        originalPawn,
        WhiteCastleKBefore, WhiteCastleQBefore,
        BlackCastleKBefore, BlackCastleQBefore,
        hash,
        whiteTurn};
}

/**
 * @brief Retrieves the last move made in the game.
 *
 * This function returns the most recent move from the move history, which is tracked during the game.
 * If no moves have been made yet (i.e., `moveCount` is 0), the function returns a default `Move` object,
 * indicating that there is no last move available.
 *
 * @return Returns a `Move` object representing the last move made. If no moves have been made, it returns
 *         a default `Move` object.
 *
 * The move history is stored in the `moveHistory` array, and `moveCount` keeps track of the number of moves
 * that have been made. If `moveCount` is greater than 0, the last move is simply retrieved from the array
 * at the index `moveCount - 1`. If no moves have been made, a default move (with empty or initial values)
 * is returned.
 * @note Used in minimax to undo the moves when backtracking.
 */
LastMove Board::getLastMove()
{
    if (moveCount == 0)
    {
        // Return a default move or handle as you wish.
        return LastMove{};
    }
    return moveHistory[moveCount - 1];
}

/**
 * @brief Determines if the game is over for the specified player.
 *
 * This function checks if the current player is in a "game-over" situation, meaning the player is in checkmate
 * or stalemate. The function first checks whether the player's king is in check, and if it is, whether there
 * are any legal moves left for the player to escape the check. If no legal moves are possible and the king
 * is still in check, the game is over. If no legal moves are available and the king is not in check, it indicates
 * a stalemate, and the game is also considered over.
 *
 * @param maximizingPlayer A boolean indicating whose turn it is. If true, it checks for White's turn,
 *                         and if false, it checks for Black's turn.
 *
 * @return Returns true if the game is over for the specified player (either due to checkmate or stalemate),
 *         otherwise returns false.
 *
 * The process works as follows:
 * 1. The function first determines the square of the king of the specified player.
 * 2. It then checks if the king has any legal moves. If no moves are available, it proceeds to check whether
 *    there are any other legal moves for the player.
 * 3. If there are no legal moves and the king is in check, the game is over (checkmate).
 * 4. If there are no legal moves but the king is not in check, the game is over (stalemate).
 */
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
        Move legalMoves[MAX_MOVES];
        int moveCount = getAllLegalMovesAsArray(legalMoves, maximizingPlayer).from;

        return (moveCount == 0);
    }

    return false;
}

/**
 * @brief Determines if the king of the specified player is in check.
 *
 * This function checks whether the king of the given player is currently in check by the opponent.
 * A player is said to be in check if their king is under attack by any of the opponent's pieces.
 * The function uses bitwise operations to check if any opponent piece is attacking the square occupied by the king.
 *
 * @param maximizingPlayer A boolean indicating which player's king to check. If true, it checks for White's king;
 *                         if false, it checks for Black's king.
 *
 * @return Returns true if the specified player's king is in check; otherwise, returns false.
 *
 * The process works as follows:
 * 1. The function determines the square of the king for the current player (either White or Black).
 * 2. It then calculates all the opponent's possible attacking squares using `getOpponentAttacks`.
 * 3. It checks if the king's square is included in the opponent's attacking squares.
 *
 * @note This function assumes the board is correctly initialized and the player being checked is valid.
 */
bool Board::isKingInCheck(bool maximizingPlayer)
{
    int kingSquare = maximizingPlayer
                         ? bitScanForward(whiteKing.bitboard)
                         : bitScanForward(blackKing.bitboard);
    uint64_t opponentAttacks = getOpponentAttacks(maximizingPlayer ? 'B' : 'w');
    return (opponentAttacks & (1ULL << kingSquare)) != 0;
}

/**
 * @brief Sets the board state from a given FEN (Forsyth-Edwards Notation) string.
 *
 * This function parses a FEN string to initialize the chessboard's piece positions, side to move,
 * castling rights, en passant target square, halfmove clock, and fullmove number. The function will reset
 * the board state and apply the FEN string's data to set up the game for the given position.
 *
 * @param fen A FEN string representing the desired board state. The FEN string should include:
 *            - Piece placement (ranks 8 to 1).
 *            - Side to move ('w' for White, 'b' for Black).
 *            - Castling rights for both players.
 *            - En passant target square (or '-' if none).
 *            - Halfmove clock (used for the fifty-move rule).
 *            - Fullmove number (the number of full moves in the game).
 *
 * The structure of the FEN string is as follows:
 * <Piece positions> <Side to move> <Castling rights> <En passant target> <Halfmove clock> <Fullmove number>
 *
 * Example FEN string: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e3 0 1"
 *
 * The function performs the following:
 * - Clears the existing piece positions on the board.
 * - Sets up each piece (P, p, N, n, B, b, R, r, Q, q, K, k) using the FEN string.
 * - Parses the turn to move (White or Black).
 * - Sets the castling rights for both players (White and Black).
 * - Sets the en passant target square.
 * - Initializes the halfmove clock and fullmove number.
 *
 * @note The FEN format must be valid for the function to correctly set up the board state. If any part of the
 *       FEN string is malformed or missing, the behavior IS UNDEFINED.
 */
void Board::setFen(const std::string &fen)
{
    whitePawns.bitboard = 0;
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

/**
 * @brief Generates the current state of the chessboard in Forsyth-Edwards Notation (FEN).
 *
 * This function constructs a string representation of the chessboard in FEN format, which is commonly used to
 * save and share chess game positions. The FEN string includes information about the piece positions, castling rights,
 * en passant target square, the turn to move, and the number of half-moves since the last pawn advance or capture.
 *
 * @return A string representing the current state of the chessboard in FEN format.
 *
 * The FEN string consists of:
 * - Piece positions from top to bottom (ranks 8 to 1), with each rank separated by a "/".
 * - Castling rights for both White and Black.
 * - The en passant target square, or "-" if none.
 * - The side to move ("w" for White, "b" for Black).
 * - The halfmove counter, which tracks the number of half-moves since the last pawn move or capture.
 *
 * The structure of the FEN string is as follows:
 * <Piece positions> <Side to move> <Castling rights> <En passant target> <Halfmove counter> <Fullmove number>
 *
 * Example FEN string: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e3 0 1"
 *
 * @note The castling rights, en passant target square, and other game state information are appended conditionally,
 *       based on the current game state. Empty squares on the board are represented by numbers, where a number
 *       indicates consecutive empty squares in a rank.
 */
std::string Board::getFen()
{
    std::string fen = "";
    for (int rank = 0; rank < 8; rank++)
    {
        int emptySquares = 0;
        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;
            char piece = getPieceAtSquare(square);

            if (piece == ' ')
            {
                emptySquares++;
            }
            else
            {
                if (emptySquares > 0)
                {
                    fen += std::to_string(emptySquares); // Append empty squares
                    emptySquares = 0;                    // Reset empty squares counter
                }
                fen += piece; // Append the piece to the FEN string
            }
        }

        // Handle trailing empty squares at the end of the rank
        if (emptySquares > 0)
        {
            fen += std::to_string(emptySquares);
        }

        // Add the rank separator, unless it's the last rank
        if (rank < 7)
        {
            fen += "/";
        }
    }

    fen += whiteToMove ? " w " : " b ";
    fen += WhiteCanCastleK ? "K" : "";
    fen += WhiteCanCastleQ ? "Q" : "";
    fen += blackCanCastleK ? "k" : "";
    fen += blackCanCastleQ ? "q" : "";
    if (!WhiteCanCastleK && !WhiteCanCastleQ && !blackCanCastleK && !blackCanCastleQ)
    {
        fen += " -";
    }
    int squareIndex = bitScanForward(enPassantTarget);

    char file = 'a' + (squareIndex % 8);
    char rank = '1' + (squareIndex / 8);

    fen += enPassantTarget ? " " + std::string(1, file) + std::string(1, rank) : " -";

    fen += " 0 1";

    return fen;
}

/**
 * @brief Undoes a move on the chessboard, restoring the previous state.
 *
 * This function reverts the most recent move by updating the board to reflect its previous state. It handles
 * the restoration of piece positions, castling rights, en passant capture, and pawn promotions, as well as
 * tracking the game state and move history through Zobrist hashing.
 *
 * @param from The starting square of the moved piece (indexed from 0 to 63).
 * @param to The destination square of the moved piece (indexed from 0 to 63).
 * @param capturedPiece The piece that was captured during the move (or ' ' if no piece was captured).
 * @param enpSquare The square where en passant was available before the move.
 * @param lastMoveEnPassant1 Boolean indicating if the last move involved en passant capture.
 * @param enpassantCapturedSquare1 The square of the captured pawn during en passant, if applicable.
 * @param enpassantCapturedPiece1 The type of the captured pawn during en passant, if applicable.
 * @param wasPromotion Boolean indicating if the move was a pawn promotion.
 * @param originalPawn The type of the pawn before promotion (used for reverting the promotion).
 * @param WhiteCastleKBefore The castling status of White's kingside before the move.
 * @param WhiteCastleQBefore The castling status of White's queenside before the move.
 * @param BlackCastleKBefore The castling status of Black's kingside before the move.
 * @param BlackCastleQBefore The castling status of Black's queenside before the move.
 * @param hash The Zobrist hash representing the state of the game before the move.
 * @param whiteTurn Boolean indicating whether it is White's turn after the move.
 *
 * This function:
 * - Restores the positions of the pieces involved in the move (including the castling rook if applicable).
 * - Reverts the pawn promotion if it was a promotion move.
 * - Restores the captured piece and the en passant captured piece (if any).
 * - Updates castling rights for both White and Black.
 * - Reverts the en passant target square to its previous state.
 * - Updates the game history to reflect the undone move.
 * - Restores the game to the state before the move, based on the Zobrist hash.
 *
 * @note This function is called in mimimax and when using AI only the latest move applied is stored.
 */
void Board::undoMove(int from, int to, char capturedPiece, uint64_t enpSquare,
                     bool lastMoveEnPassant1, int enpassantCapturedSquare1,
                     char enpassantCapturedPiece1, bool wasPromotion,
                     char originalPawn, bool WhiteCastleKBefore,
                     bool WhiteCastleQBefore, bool BlackCastleKBefore,
                     bool BlackCastleQBefore,
                     uint64_t hash, bool whiteTurn)
{

    if (gameFensHistory.find(hash) != gameFensHistory.end())
    {
        gameFensHistory[hash]--;
        if (gameFensHistory[hash] == 0)
        {
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

    allPieces = getBlackPieces() | getWhitePieces();
}

/**
 * @brief Updates the bitboard for the piece at the given square.
 *
 * This function updates the bitboard of the corresponding piece (pawn, knight, bishop, rook, queen, or king)
 * based on the piece type. The piece is placed on the specified square by setting the appropriate bit in the
 * corresponding bitboard (e.g., for a white pawn, the bit corresponding to the square is set in the whitePawns bitboard).
 *
 * @param square The square where the piece is to be placed (indexed from 0 to 63, representing the 64 squares of the chessboard).
 * @param piece The type of piece being placed on the board (e.g., 'p' for black pawn, 'P' for white pawn, etc.).
 *
 * This function handles both uppercase and lowercase pieces, where uppercase indicates white pieces and lowercase
 * indicates black pieces. The bitboard of the corresponding color and piece type is updated by setting the bit for
 * the provided square.
 */
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

/**
 * @brief Executes a move on the chessboard, updating all relevant game states.
 *
 * This function handles the movement of a piece from one square to another. It performs
 * all necessary updates to the bitboards, handles castling, en passant, and promotion,
 * and stores the resulting game state after the move. It also handles the logic for
 * checking whether a move involves special cases like en passant or castling.
 *
 * @param from The starting square of the piece being moved.
 * @param to The destination square for the piece.
 *
 * @return `true` if the move was successfully applied, otherwise `false`.
 *
 * The function performs the following tasks:
 * 1. **Computes Zobrist hash**: Calculates and stores the Zobrist hash for the new game state.
 * 2. **Handles Castling**:
 *    - If the king moves two squares, it handles the castling logic for both White and Black.
 *    - If a rook moves, it updates the castling rights for the player who moved the rook.
 * 3. **Clears captured pieces**: If a piece is captured, it is removed from the corresponding bitboard.
 * 4. **Handles En Passant**: If the move is an en passant capture, it clears the captured pawn.
 * 5. **Updates Bitboards**: Updates the bitboards for the moving piece.
 * 6. **Handles Pawn Promotion**: If a pawn reaches the last rank, it is promoted to a queen.
 * 7. **Stores the move**: Records the move in the history, including details about en passant, castling rights, and promotions.
 * 8. **Updates En Passant Target**: If the move involves a pawn advancing two squares, it sets the en passant target square.
 * 9. **Switches Turn**: After the move, the turn changes to the opponent.
 *
 * **Special Cases**:
 * - **Castling**: If the king moves two squares, it checks if castling is allowed and updates the bitboards accordingly.
 * - **En Passant**: If the move is an en passant capture, the pawn is removed from the correct square, and the en passant state is updated.
 * - **Pawn Promotion**: If a pawn reaches the last rank, it is promoted to a queen.
 * - **Clearing Captured Pieces**: Any captured piece is cleared from the bitboard.
 *
 * **Edge Cases**:
 * - If a pawn moves two squares and lands on a square where en passant is possible, the en passant state is updated.
 * - Castling rights are updated if a rook or king moves, or if the castling move itself is executed.
 */
bool Board::movePiece(int from, int to)
{

    // computeZobristHash();
    uint64_t hash = getZobristHash();
    gameFensHistory[hash]++;

    char piece = getPieceAtSquare(from);
    char destPiece = getPieceAtSquare(to);

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

    allPieces = getBlackPieces() | getWhitePieces();
    return result;
}

/**
 * @brief Clears the captured piece from the corresponding bitboard.
 *
 * This function handles the removal of a captured piece from the bitboard.
 * It accounts for regular captures, en passant captures, and ensures that
 * the correct piece is cleared based on the destination square and piece type.
 *
 * @param to The destination square where the piece was captured.
 * @param destPiece The piece being captured, represented by its character
 *                  ('p', 'n', 'b', 'r', 'q', 'k' for black pieces and 'P', 'N',
 *                  'B', 'R', 'Q', 'K' for white pieces).
 *
 * The function works by:
 * 1. Checking if the captured piece was part of an en passant capture. In this case,
 *    it removes the captured pawn from the appropriate square behind or ahead of the
 *    pawn involved in the en passant.
 * 2. If the capture is a regular one, it determines whether the piece is black or white,
 *    and clears the captured piece from the corresponding bitboard.
 *
 * The function ensures that only valid pieces are cleared and that edge cases like
 * en passant captures are handled correctly.
 *
 * @note This function assumes that the destination square is occupied by a valid
 *       captured piece, and that en passant is handled by the game logic before
 *       calling this function.
 */
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

/**
 * @brief Returns the piece at a given square.
 *
 * This function checks if a piece exists at a particular square using bitboards.
 * It will return the character representing the piece (upper case for white,
 * lower case for black) or a space character if no piece is found.
 *
 * @param square The index of the square (0 to 63).
 *
 * @return A character representing the piece at the square ('P', 'N', 'B', 'R', 'Q', 'K'
 *         for white pieces; 'p', 'n', 'b', 'r', 'q', 'k' for black pieces; or ' ' for an empty square).
 */
char Board::getPieceAtSquare(int square)
{

    if ((allPieces & (1ULL << square)) == 0)
    {
        return ' ';
    }

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

/**
 * @brief Checks if a move is valid for the piece at the given square.
 *
 * This function determines if a move from the square `from` to the square `to`
 * is valid based on the type of the piece located at `from`. It calls the
 * appropriate helper function for each piece type (pawn, knight, bishop, rook,
 * queen, or king) to validate the move.
 *
 * @param from The starting square of the piece.
 * @param to The destination square for the move.
 *
 * @return True if the move is valid for the piece, false otherwise.
 */
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

/**
 * @brief Finds the checkers attacking the king.
 *
 * This function determines which opponent pieces are currently checking the king.
 * It identifies pawns, knights, rooks, bishops, and queens attacking the king,
 * and it also computes the "check mask" that indicates where the king can move
 * to escape the check.
 *
 * The function checks each piece type that can potentially attack the king:
 * 1. **Pawns**: Check if they are attacking diagonally towards the king.
 * 2. **Knights**: Check if a knight can jump to the king's square.
 * 3. **Rooks and Queens**: Check if a rook or queen is attacking the king through open lines.
 * 4. **Bishops and Queens**: Check if a bishop or queen is attacking the king along diagonals.
 *
 *
 * @param squareOfKing The square where the king is positioned.
 * @param king The character representing the king ('K' for white, 'k' for black).
 * @param checkMask A reference to a variable that will hold the check mask.
 *
 * @return A bitboard representing the squares where the checkers are located.
 */
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

    uint64_t kingBit = 1ULL << squareOfKing;

    char piece = color ? 'P' : 'p';
    int colorPawn = std::islower(piece) ? 0 : 1;
    while (attackingPawns)
    {
        int pawnSquare = bitScanForward(attackingPawns);

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
        if (attackTable.knightMovesTable[square] & kingBit)
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
        if (attackTable.rookTable[square][hash] & kingBit)
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
        if (attackTable.bishopTable[square][hash] & kingBit)
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

/**
 * @brief Identifies pieces that are pinned to the king.
 *
 * This function determines which pieces are pinned relative to the king's position.
 * A piece is considered pinned if:
 * - It lies on the same attack path as an enemy sliding piece (bishop, rook, or queen).
 * - Moving it would expose the king to an attack.
 *
 * The function performs the following steps:
 * 1. **Identify attack lines**: Determines potential pinning attack paths using precomputed attack tables.
 * 2. **Check for enemy sliding pieces**: Finds opposing bishops, rooks, and queens along the king’s attack paths.
 * 3. **Detect pinned pieces**: If there is exactly one piece between the king and an enemy attacker, it is pinned.
 * 4. **Store pin masks**: Stores legal move masks for pinned pieces to restrict their movement along the attack path.
 *
 * @param squareOfKing The square where the king is positioned.
 * @param king The character representing the king ('K' for white, 'k' for black).
 *
 * @return A bitboard representing the pinned pieces, with each bit corresponding to a pinned piece’s position.
 */
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

/**
 * @brief Determines if a piece is pinned to its king.
 *
 * A piece is considered pinned if:
 * - Moving it would expose the king to an attack from an enemy sliding piece (bishop, rook, or queen).
 * - It lies along the same attack path as the king and an opposing sliding piece.
 *
 * This function relies on the precomputed pinned pieces bitboard, which is obtained from
 * `findPinnedPieces(squareOfKing, king)`. It checks whether the given piece is present
 * in that bitboard.
 *
 * @param pieceSquare The square of the piece being evaluated.
 * @param squareOfKing The square where the king is positioned.
 * @param king The character representing the king ('K' for white, 'k' for black).
 *
 * @return True if the piece is pinned to the king, otherwise false.
 */
bool Board::isPiecePinned(int pieceSquare, int squareOfKing, char king)
{
    uint64_t pinnedPieces = findPinnedPieces(squareOfKing, king);
    return (pinnedPieces >> pieceSquare) & 1;
}

/**
 * @brief Determines if a piece is pinned to its king by an opposing sliding piece.
 *
 * A piece is considered pinned if:
 * - It lies on the same line (rank, file, or diagonal) as the king and an enemy sliding piece (bishop, rook, or queen).
 * - Removing the piece would expose the king to an attack.
 *
 * This function checks:
 * 1. Whether the piece is on the same line of sight as the king.
 * 2. Whether the king is also on the same line.
 * 3. Whether there are any blockers between them (if not, the piece is pinned).
 *
 * @param pieceSquare The square of the piece being evaluated.
 * @param squareOfKing The square where the king is positioned.
 * @param lineOfSight A bitboard representing the attack path of a sliding piece (rook, bishop, or queen).
 *
 * @return True if the piece is pinned to the king, otherwise false.
 */
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

/**
 * @brief Generates all legal pawn moves for a given square, considering the current board state.
 *
 * This function calculates all possible pawn moves while applying board constraints such as:
 * - **Basic Movement**: Pawns move forward one square if unoccupied and can move two squares on their first move.
 * - **Captures**: Pawns can capture diagonally but not move diagonally without a capture.
 * - **En Passant**: If an en passant target square is available, the pawn can capture accordingly.
 * - **Pins**: If the pawn is pinned to its king, its movement is restricted to valid pin-aligned moves.
 * - **Checks**: If the king is in check, only moves that block or capture the checking piece are considered.
 *
 * The function ensures that illegal moves are filtered out based on the board state.
 *
 * @param square The starting square of the pawn for which legal moves are being generated.
 * @param piece The character representing the pawn ('P' for white, 'p' for black).
 *
 * @return A 64-bit integer representing the legal moves for the pawn, with each bit corresponding
 * to a valid destination square.
 */
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

/**
 * @brief Generates pawn attack moves that could threaten the opposing king.
 *
 * This function determines the squares a pawn would attack, which is useful in scenarios such as:
 * - **King Safety**: Checking which squares are controlled by enemy pawns to avoid illegal king moves.
 * - **Threat Detection**: Identifying pawn attacks for tactical evaluations.
 *
 * The function considers standard pawn attack moves (diagonal captures) based on color:
 * - White pawns attack diagonally forward (-9 and -7 squares).
 * - Black pawns attack diagonally forward (+7 and +9 squares).
 *
 * Note: This function does not check for actual board state legality or pinned pieces.
 *
 * @param square The current square of the pawn.
 * @param piece The character representing the pawn ('P' for white, 'p' for black).
 *
 * @return A 64-bit integer representing the squares attacked by the pawn, with each bit corresponding to a threatened square.
 */
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

/**
 * @brief Generates all legal knight moves for a given square, considering the current board state.
 *
 * This function computes all possible moves for a knight while applying board constraints such as:
 * - **Pins**: Although knights are not usually affected by pins, a knight might still be restricted if it's pinned.
 * - **Checks**: If the king is in check, only moves that block or capture the checking piece are considered.
 * - **Friendly Pieces**: The knight cannot move to squares occupied by its own pieces.
 *
 * The function utilizes a precomputed attack table for efficient move generation.
 *
 * @param square The starting square of the knight for which legal moves are being generated.
 * @param piece The character representing the knight ('N' for white, 'n' for black).
 *
 * @return A 64-bit integer representing the legal moves for the knight, with each bit corresponding to a valid destination square.
 */
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

/**
 * @brief Generates knight moves, including protected squares, without considering pins.
 *
 * This function computes all possible knight moves while incorporating certain constraints:
 * - **Check Handling**: If the king is in check, only moves that block or capture the checking piece are considered.
 * - **No Pin Consideration**: Unlike sliding pieces, knights cannot be pinned since they move in an "L" shape.
 *
 * The function uses a precomputed attack table for efficient move generation.
 *
 * @param square The starting square of the knight for which legal moves are being generated.
 * @param piece The character representing the knight ('N' for white, 'n' for black).
 *
 * @return A 64-bit integer representing the legal moves for the knight, with each bit corresponding to a valid destination square.
 */
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

/**
 * @brief Generates all legal bishop moves for a given square, considering the current board state.
 *
 * This function computes all possible moves for a bishop, taking into account board constraints such as:
 * - **Pinned Pieces**: If the bishop is pinned to the king, its movement is restricted to the pin's direction.
 * - **Checks**: If the king is in check, only moves that block or capture the checking piece are considered.
 * - **Friendly Pieces**: The bishop cannot move to squares occupied by its own pieces.
 *
 * The function utilizes magic bitboards for efficient move generation.
 *
 * @param square The starting square of the bishop for which legal moves are being generated.
 * @param piece The character representing the bishop ('B' for white, 'b' for black).
 *
 * @return A 64-bit integer representing the legal moves for the bishop, with each bit corresponding to a valid destination square.
 */
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

/**
 * @brief Generates bishop moves, including protected squares, without considering pins.
 *
 * This function computes all possible bishop moves while incorporating certain constraints:
 * - **Check Handling**: If the king is in check, only moves that block or capture the checking piece are considered.
 * - **Opponent King Exclusion**: The bishop cannot move into the square occupied by the opponent's king.
 *
 * Unlike a standard bishop move generator, this function does not consider pinned pieces, allowing for
 * move generation even when a pin might exist.
 *
 * @param square The starting square of the bishop for which legal moves are being generated.
 * @param piece The character representing the bishop ('B' for white, 'b' for black).
 *
 * @return A 64-bit integer representing the legal moves for the bishop, with each bit corresponding to a valid destination square.
 */
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

/**
 * @brief Generates all legal rook moves for a given square, considering the current board state.
 *
 * This function computes all possible moves for a rook, factoring in board constraints such as:
 * - **Pinned Pieces**: If the rook is pinned to the king, it can only move along the pin line.
 * - **Checks**: If the king is in check, only moves that block or capture the checking piece are considered.
 * - **Friendly Pieces**: The rook cannot move to squares occupied by its own pieces.
 *
 * The function leverages magic bitboards to efficiently generate possible move sets.
 *
 * @param square The starting square of the rook for which legal moves are being generated.
 * @param piece The character representing the rook ('R' for white, 'r' for black).
 *
 * @return A 64-bit integer representing the legal moves for the rook, with each bit corresponding to a valid destination square.
 */

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

/**
 * @brief Generates rook moves, including protected squares, without considering pins,
 * since a king cant move on to a square seen by a rook.
 *
 * This function computes all possible rook moves while incorporating additional constraints:
 * - **Check Handling**: If the king is in check, only moves that block or capture the checking piece are considered.
 * - **Opponent King Exclusion**: The rook cannot move into the square occupied by the opponent's king.
 *
 * Unlike `generateRookMoves`, this function does not consider pinned pieces.
 *
 * @param square The starting square of the rook for which legal moves are being generated.
 * @param piece The character representing the rook ('R' for white, 'r' for black).
 *
 * @return A 64-bit integer representing the legal moves for the rook, with each bit corresponding to a valid destination square.
 */
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

Move Board::getAllLegalMovesAsArray(Move movesList[], bool maximizingPlayer)
{
    int moveCount = 0;
    int captureCount = 0;
    int attackedCount = 0;

    Move captureMoves[218];
    Move nonCaptureMoves[218];
    Move pieceUnderAttack[218];

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
    auto prioritizeMove = [&](Move move) -> bool
    {
        int fromSquare = move.from;
        int toSquare = move.to;
        char piece = getPieceAtSquare(fromSquare);

        // Prioritize king castling moves
        if (piece == 'k' || piece == 'K')
        {
            if (fromSquare - move.from == 2 || fromSquare - move.to == -2) // Castling move
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
            !(occupied & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59))) &&
            !(opponentAttacks & ((1ULL << 60) | (1ULL << 59) | (1ULL << 58))) &&
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
/*
__forceinline int Board::bitScanForward(uint64_t bitboard)
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
}*/

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
