#ifndef EVALUATION_H
#define EVALUATION_H

#include "Board.h"
#include <cstdint>
#include <intrin.h>
class Board;

class Evaluation
{
public:
    explicit Evaluation(std::shared_ptr<Board> board);

    char previousPiece;

    // Evaluates the current position
    int evaluatePosition();

private:
    std::shared_ptr<Board> board; // Pointer to the Board object

    // Material evaluation
    int evaluateMaterial();
    int evaluatePieceSet(Bitboard &bitboard, int value);
    bool isPassedPawn(int square, bool isWhite);
    int evaluatePassedPawns();
    int rookOnOpenFile();
    uint64_t getFileMask(int file);

    // Positional evaluation
    int evaluatePieceSquareTables();
    int evaluatePiecePosition(Bitboard &bitboard, const int table[64], bool mirror = false);

    // Utility functions
    int bitScanForward(uint64_t bitboard);
    int mirrorIndex(int square);
};

#endif // EVALUATION_H
