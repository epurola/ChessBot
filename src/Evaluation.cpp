#include "Evaluation.h"
#include "Board.h"

Evaluation::Evaluation(std::shared_ptr<Board> board) : board(board) {}

// Piece-Square Tables
static const int Pawns[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5, 5, 10, 25, 25, 10, 5, 5,
    0, 0, 0, 20, 20, 0, 0, 0,
    5, -5, -10, 0, 0, -10, -5, 5,
    5, 10, 10, -30, -30, 10, 10, 5,
    0, 0, 0, 0, 0, 0, 0, 0};

static const int PawnsEnd[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    80, 80, 80, 80, 80, 80, 80, 80,
    50, 50, 50, 50, 50, 50, 50, 50,
    30, 30, 30, 30, 30, 30, 30, 30,
    20, 20, 20, 20, 20, 20, 20, 20,
    10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10,
    0, 0, 0, 0, 0, 0, 0, 0};

static const int KingSafety[64] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    20, 10, 0, 0, 0, 0, 10, 20,
    40, 50, 10, 0, 0, 10, 50, 40};

static const int KingEnd[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -5, 0, 5, 5, 5, 5, 0, -5,
    -10, -5, 20, 30, 30, 20, -5, -10,
    -15, -10, 35, 45, 45, 35, -10, -15,
    -20, -15, 30, 40, 40, 30, -15, -20,
    -25, -20, 20, 25, 25, 20, -20, -25,
    -30, -25, 0, 0, 0, 0, -25, -30,
    -50, -30, -30, -30, -30, -30, -30, -50};

static const int Knights[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20, 0, 0, 0, 0, -20, -40,
    -30, 0, 10, 15, 15, 10, 0, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 0, 15, 20, 20, 15, 0, -30,
    -30, 5, 20, 15, 15, 20, 5, -30,
    -40, -20, 0, 0, 0, 0, -20, -40,
    -50, -50, -30, -30, -30, -30, -50, -50};

static const int Bishops[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 10, 10, 5, 0, -10,
    -10, 5, 5, 10, 10, 5, 5, -10,
    -10, 0, 10, 10, 10, 10, 0, -10,
    -10, 10, 10, 10, 10, 10, 10, -10,
    -10, 5, 0, 0, 0, 0, 5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20};

static const int Rooks[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    5, 10, 10, 10, 10, 10, 10, 5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 10, 10, 0, 0, -5};

int Evaluation::evaluatePosition()
{

    int materialScore = evaluateMaterial();

    int positionalScore = evaluatePieceSquareTables();

    int pawns = evaluatePassedPawns();

    int rooks = rookOnOpenFile();

    return materialScore + positionalScore + pawns + rooks;
}

// Evaluates material balance
int Evaluation::evaluateMaterial()
{
    int score = 0;

    score += evaluatePieceSet(board->whitePawns, 100);
    score += evaluatePieceSet(board->blackPawns, -100);
    score += evaluatePieceSet(board->whiteKnights, 300);
    score += evaluatePieceSet(board->blackKnights, -300);
    score += evaluatePieceSet(board->whiteBishops, 320);
    score += evaluatePieceSet(board->blackBishops, -320);
    score += evaluatePieceSet(board->whiteRooks, 500);
    score += evaluatePieceSet(board->blackRooks, -500);
    score += evaluatePieceSet(board->whiteQueens, 900);
    score += evaluatePieceSet(board->blackQueens, -900);

    return score;
}

int Evaluation::evaluatePassedPawns()
{
    int score = 0;

    // Evaluate white passed pawns
    for (int i = 0; i < 8; i++)
    {
        uint64_t whitePawnsMask = board->whitePawns.bitboard & (1ULL << i);
        while (whitePawnsMask)
        {
            int square = bitScanForward(whitePawnsMask);

            if (isPassedPawn(square, true))
            {
                score += 50;
            }
            whitePawnsMask &= whitePawnsMask - 1;
        }
    }

    // Evaluate black passed pawns
    for (int i = 0; i < 8; i++)
    {
        uint64_t blackPawnsMask = board->blackPawns.bitboard & (1ULL << i);
        while (blackPawnsMask)
        {
            int square = bitScanForward(blackPawnsMask);

            if (isPassedPawn(square, false))
            {
                score -= 50;
            }
            blackPawnsMask &= blackPawnsMask - 1;
        }
    }

    return score;
}

// Helper function to determine if a pawn is a passed pawn
bool Evaluation::isPassedPawn(int square, bool isWhite)
{
    int file = square % 8;
    uint64_t opponentPawns = isWhite ? board->blackPawns.bitboard : board->whitePawns.bitboard;

    // Check if any opponent pawns are on the same file or adjacent files
    // For white pawns, the opponent's pawns should not be in the same or adjacent files
    // For black pawns, the opponent's pawns should not be in the same or adjacent files
    uint64_t blockMask = (1ULL << file) | (1ULL << (file - 1)) | (1ULL << (file + 1));

    if ((opponentPawns & blockMask) == 0)
    {
        return true;
    }
    return false;
}

int Evaluation::evaluatePieceSquareTables()
{
    int score = 0;

    bool isEndgame = board->whitePawns.count() + board->blackPawns.count() + board->whiteBishops.count() + board->blackBishops.count() + board->whiteKnights.count() + board->blackKnights.count() + board->whiteQueens.count() + board->blackQueens.count() + board->whiteRooks.count() + board->blackRooks.count() < 15;

    score += evaluatePiecePosition(board->whitePawns, isEndgame ? PawnsEnd : Pawns);
    score += evaluatePiecePosition(board->blackPawns, isEndgame ? PawnsEnd : Pawns, true); // Mirrored for Black

    score += evaluatePiecePosition(board->whiteKnights, Knights);
    score += evaluatePiecePosition(board->blackKnights, Knights, true);

    score += evaluatePiecePosition(board->whiteRooks, Rooks);
    score += evaluatePiecePosition(board->blackRooks, Rooks, true);

    score += evaluatePiecePosition(board->whiteBishops, Bishops);
    score += evaluatePiecePosition(board->blackBishops, Bishops, true);

    score += evaluatePiecePosition(board->whiteKing, isEndgame ? KingEnd : KingSafety);
    score += evaluatePiecePosition(board->blackKing, isEndgame ? KingEnd : KingSafety, true);

    return score;
}

int Evaluation::rookOnOpenFile()
{
    uint64_t whiteRooks = board->whiteRooks.bitboard;
    uint64_t blackRooks = board->blackRooks.bitboard;
    uint64_t allPawns = board->whitePawns.bitboard | board->blackPawns.bitboard;

    int score = 0;

    uint64_t whitePawns = board->whitePawns.bitboard;
    uint64_t blackPawns = board->blackPawns.bitboard;

    while (whiteRooks)
    {
        int square = bitScanForward(whiteRooks);
        int file = square % 8;
        uint64_t fileMask = getFileMask(file);

        if ((allPawns & fileMask) == 0)
        { // Open file
            score += 50;
        }
        else if ((whitePawns & fileMask) == 0)
        { // Semi-open (no white pawns)
            score += 25;
        }

        whiteRooks &= whiteRooks - 1;
    }

    while (blackRooks)
    {
        int square = bitScanForward(blackRooks);
        int file = square % 8;
        uint64_t fileMask = getFileMask(file);

        if ((allPawns & fileMask) == 0)
        { // Open file
            score -= 50;
        }
        else if ((blackPawns & fileMask) == 0)
        { // Semi-open (no black pawns)
            score -= 25;
        }

        blackRooks &= blackRooks - 1;
    }
    return score;
}

uint64_t Evaluation::getFileMask(int file)
{
    uint64_t fileMask = 0x0101010101010101ULL; // Column A mask
    return fileMask << file;                   // Shift left to get the correct file
}

int Evaluation::evaluatePieceSet(Bitboard &bitboard, int value)
{
    uint64_t bitMask = bitboard.bitboard;
    __int64 count = _mm_popcnt_u64(bitMask);
    return static_cast<int>(count) * value;
}

int Evaluation::evaluatePiecePosition(Bitboard &bitboard, const int table[64], bool mirror)
{
    uint64_t bitMask = bitboard.bitboard;
    int score = 0;

    while (bitMask)
    {
        int square = bitScanForward(bitMask);
        // char piece = board->getPieceAtSquare(square);

        int position = mirror ? -table[mirrorIndex(square)] : table[square];

        score += position;

        bitMask &= bitMask - 1;
    }

    return score;
}

int Evaluation::bitScanForward(uint64_t bitboard)
{
    unsigned long index;
    if (_BitScanForward64(&index, bitboard))
    {
        return index;
    }
    return -1;
}

// Mirrors square index for Black pieces
inline int Evaluation::mirrorIndex(int square)
{
    return square ^ 63;
}
