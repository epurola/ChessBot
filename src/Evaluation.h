#ifndef EVALUATION_H
#define EVALUATION_H

#include "Board.h"
#include <cstdint>
#include <intrin.h>
class Board;

    // Piece-Square Tables
    static const int Pawns[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
        5, 5, 10, 25, 25, 10, 5, 5,
        0, -5, -5, 20, 20, -5, -5, 0,
        5, -10, -10, 0, 0, -10, -10, 5,
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

class Evaluation
{
public:
    explicit Evaluation(std::shared_ptr<Board> board);

    char previousPiece;

    // Evaluates the current position
    int evaluatePosition();

private:
    std::shared_ptr<Board> board; 

    // Material evaluation
    int evaluateMaterial();
    inline int evaluatePieceSet(Bitboard &bitboard, int value);
    bool isPassedPawn(int square, bool isWhite);
    int evaluatePassedPawns();
    int rookOnOpenFile();
    uint64_t getFileMask(int file);
    int evaluateCastlingPawns();
    int evaluateRookInLineWithKing();

    // Positional evaluation
    int evaluatePieceSquareTables();
    int evaluatePiecePosition(Bitboard &bitboard, const int table[64], bool mirror = false);

    // Utility functions
    int bitScanForward(uint64_t bitboard);
    int mirrorIndex(int square);

};

#endif // EVALUATION_H
