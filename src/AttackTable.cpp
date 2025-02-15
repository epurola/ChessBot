#include "AttackTable.h"

void AttackTable::initialize()
{
    for (int i = 0; i < 64; i++)
    {
        rookMask[i] = createRookMovementMask(i);
        bishopMask[i] = createBishopMovementMask(i);
        rookMaskFull[i] = createRookLegalMoveBitboard(i, 0);
        bishopMaskFull[i] = createBishopLegalMoveBitboard(i, 0);
    }
    createRookTable();
    createBishopTable();
    initBetweenTable();
    initializeKnightMoves();
}

void AttackTable::initializeKnightMoves()
{
    for (int square = 0; square < 64; square++)
    {
        uint64_t moves = 0ULL;
        int rank = square / 8;
        int file = square % 8;

        int offsets[8][2] = {
            {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};

        for (auto &offset : offsets)
        {
            int newRank = rank + offset[0];
            int newFile = file + offset[1];

            if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8)
            {
                moves |= (1ULL << (newRank * 8 + newFile));
            }
        }

        knightMovesTable[square] = moves;
    }
}

uint64_t AttackTable::squaresBetween(int from, int to)
{
    uint64_t mask = 0ULL;
    int fromRank = from / 8, fromFile = from % 8;
    int toRank = to / 8, toFile = to % 8;

    if (fromRank == toRank)
    {
        int minFile = std::min(fromFile, toFile);
        int maxFile = std::max(fromFile, toFile);
        for (int file = minFile + 1; file < maxFile; ++file)
            mask |= (1ULL << (fromRank * 8 + file));
    }

    else if (fromFile == toFile)
    {
        int minRank = std::min(fromRank, toRank);
        int maxRank = std::max(fromRank, toRank);
        for (int rank = minRank + 1; rank < maxRank; ++rank)
            mask |= (1ULL << (rank * 8 + fromFile));
    }

    else if ((toRank - fromRank) == (toFile - fromFile))
    {
        int minSquare = std::min(from, to);
        int maxSquare = std::max(from, to);
        for (int square = minSquare + 9; square < maxSquare; square += 9)
            mask |= (1ULL << square);
    }

    else if ((toRank - fromRank) == -(toFile - fromFile))
    {
        int minSquare = std::min(from, to);
        int maxSquare = std::max(from, to);
        for (int square = minSquare + 7; square < maxSquare; square += 7)
            mask |= (1ULL << square);
    }
    return mask;
}

void AttackTable::initBetweenTable()
{
    for (int from = 0; from < 64; ++from)
    {
        for (int to = 0; to < 64; ++to)
        {
            if (from != to)
                betweenTable[from][to] = squaresBetween(from, to);
            else
                betweenTable[from][to] = 0;
        }
    }
}

std::vector<uint64_t> AttackTable::createBlockerBitBoards(uint64_t movementMask)
{

    std::vector<int> moveSquareIndices;
    for (int i = 0; i < 64; i++)
    {
        if (((movementMask >> i) & 1) == 1)
        {
            moveSquareIndices.push_back(i);
        }
    }

    int numberOfBitboards = 1 << moveSquareIndices.size();

    std::vector<uint64_t> blockerBitBoards;

    for (int i = 0; i < numberOfBitboards; i++)
    {
        uint64_t blockerBitBoard = 0;
        for (int j = 0; j < moveSquareIndices.size(); j++)
        {
            if ((i >> j) & 1)
            {
                blockerBitBoard |= (1ULL << moveSquareIndices[j]);
            }
        }
        blockerBitBoards.push_back(blockerBitBoard);
    }

    return blockerBitBoards;
}

void AttackTable::createRookTable()
{
    uint64_t blockers;

    for (int square = 0; square < 64; square++)
    {
        uint64_t movementMask = createRookMovementMask(square);
        std::vector<uint64_t> blockerBitBoards = createBlockerBitBoards(movementMask);

        for (uint64_t blockerBitBoard : blockerBitBoards)
        {

            blockers = blockerBitBoard & movementMask;

            uint64_t key = (blockers * rookMagics[square]) >> (64 - rookIndex[square]);

            uint64_t legalBitBoard = createRookLegalMoveBitboard(square, blockers);

            rookTable[square][key] = legalBitBoard;
        }
    }
}

void AttackTable::createBishopTable()
{
    uint64_t blockers;

    for (int square = 0; square < 64; square++)
    {
        uint64_t movementMask = createBishopMovementMask(square);
        std::vector<uint64_t> blockerBitBoards = createBlockerBitBoards(movementMask);

        for (uint64_t blockerBitBoard : blockerBitBoards)
        {

            blockers = blockerBitBoard & movementMask;

            uint64_t key = (blockers * bishopMagics[square]) >> (64 - bishopIndex[square]);

            uint64_t legalBitBoard = createBishopLegalMoveBitboard(square, blockers);

            bishopTable[square][key] = legalBitBoard;
        }
    }
}

uint64_t AttackTable::createRookLegalMoveBitboard(int square, uint64_t blockers)
{
    uint64_t bitboard = 0;
    int startRank = square / 8;
    int startFile = square % 8;

    for (const auto &[dRank, dFile] : ROOK_DIRECTIONS)
    {
        for (int dst = 1; dst < 8; ++dst)
        {
            int newRank = startRank + dRank * dst;
            int newFile = startFile + dFile * dst;

            if (newRank < 0 || newRank >= 8 || newFile < 0 || newFile >= 8)
                break;

            int newSquare = newRank * 8 + newFile;
            bitboard |= (1ULL << newSquare);

            if ((blockers & (1ULL << newSquare)) != 0)
                break;
        }
    }

    return bitboard;
}

uint64_t AttackTable::createRookMovementMask(int square)
{
    uint64_t mask = 0;

    int rank = square / 8;
    int file = square % 8;

    for (const auto &[dRank, dFile] : ROOK_DIRECTIONS)
    {
        for (int dst = 1; dst < 8; dst++)
        {
            int newRank = rank + dRank * dst;
            int newFile = file + dFile * dst;

            if (newRank < 0 || newRank >= 8 || newFile < 0 || newFile >= 8)
                break;

            if (newRank + dRank < 0 || newRank + dRank >= 8 ||
                newFile + dFile < 0 || newFile + dFile >= 8)
                break;

            mask |= (1ULL << (newRank * 8 + newFile));
        }
    }

    return mask;
}

uint64_t AttackTable::createBishopMovementMask(int square)
{
    uint64_t mask = 0;

    int rank = square / 8;
    int file = square % 8;

    for (const auto &[dRank, dFile] : BISHOP_DIRECTIONS)
    {
        for (int dst = 1; dst < 8; dst++)
        {
            int newRank = rank + dRank * dst;
            int newFile = file + dFile * dst;

            if (newRank < 0 || newRank >= 8 || newFile < 0 || newFile >= 8)
                break;

            if (newRank + dRank < 0 || newRank + dRank >= 8 ||
                newFile + dFile < 0 || newFile + dFile >= 8)
                break;

            mask |= (1ULL << (newRank * 8 + newFile));

            if (newRank + dRank < 0 || newRank + dRank >= 8 ||
                newFile + dFile < 0 || newFile + dFile >= 8)
                break;
        }
    }

    return mask;
}

uint64_t AttackTable::createBishopLegalMoveBitboard(int square, uint64_t blockers)
{
    uint64_t bitboard = 0;
    int startRank = square / 8;
    int startFile = square % 8;

    for (const auto &[dRank, dFile] : BISHOP_DIRECTIONS)
    {
        for (int dst = 1; dst < 8; ++dst)
        {
            int newRank = startRank + dRank * dst;
            int newFile = startFile + dFile * dst;

            if (newRank < 0 || newRank >= 8 || newFile < 0 || newFile >= 8)
                break;

            int newSquare = newRank * 8 + newFile;
            bitboard |= (1ULL << newSquare);
            if ((blockers & (1ULL << newSquare)) != 0)
                break;
        }
    }

    return bitboard;
}
