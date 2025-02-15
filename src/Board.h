#ifndef BOARD_H
#define BOARD_H

#include "BitBoard.h"
#include <cctype>
#include <cstdlib>
#include <AttackTable.h>
#include <unordered_map>
#include <random>
#include <cstdint>

constexpr int TABLE_SIZE = 1000000;

class TranspositionEntry
{
public:
    int evaluation;
    int depth;
    int bestFrom, bestTo;
    int flag; 
    uint64_t zobristKey;

    TranspositionEntry(int eval = 0, int d = 0, int f = 0, int bf = -1, int bt = -1)
        : evaluation(eval), depth(d), flag(f), bestFrom(bf), bestTo(bt) {}
};

struct Move
{
    int from, to;
    char capturedPiece;
    uint64_t enpSquare;
    bool wasEnPassant;
    int enPassantCapturedSquare;
    char enPassantCapturedPiece;
    bool wasPromotion;
    char originalPawn;
};

class Board
{
public:
    Board();

    void setFen(const std::string &fen);
    bool gameOver(bool maximizingPlayer);
    uint64_t generatePawnMovesForKing(int square, char piece);
    uint64_t generateRookMovesForKing(int square, char piece);

    bool movePiece(int from, int to);
    void undoMove(int from, int to, char capturedPiece, uint64_t enpSquare,
                  bool lastMoveEnPassant, int enpassantCapturedSquare, char enpassantCapturedPiece, bool wasPromotion, char originalPawn);

    void restoreCapturedPiece(int square, char piece);
    bool updateBitboards(char piece, int from, int to);

    uint64_t findCheckers(int squareOfKing, char king, uint64_t &checkMask);

    int countAttackedSquares(int square, char piece);

    void printBitboard(uint64_t bitboard, const std::string &label);
    uint64_t getOpponentAttacks(char piece);
    int bitScanForward(uint64_t bitboard);
    uint64_t findPinnedPieces(int squareOfKing, char piece);
    bool isPiecePinnedToKing(int pieceSquare, int squareOfKing, uint64_t lineOfSight);
    bool isPiecePinned(int pieceSquare, int squareOfKing, char king);

    bool legalPawnMove(int from, int to);
    bool legalKnightMove(int from, int to);
    bool legalBishopMove(int from, int to);
    bool legalRookMove(int from, int to);
    bool legalQueenMove(int from, int to);
    bool legalKingMove(int from, int to);

    uint64_t generatePawnMoves(int square, char piece);
    uint64_t generateKnightMoves(int square, char piece);
    uint64_t generateBishopMoves(int square, char piece);
    uint64_t generateRookMoves(int square, char piece);
    uint64_t generateQueenMoves(int square, char piece);
    uint64_t generateKingMoves(int square, char piece);

    uint64_t generateKnightMovesWithProtection(int square, char piece);
    uint64_t generateBishopMovesWithProtection(int square, char piece);
    uint64_t generateRookMovesWithProtection(int square, char piece);
    uint64_t generateQueenMovesWithProtection(int square, char piece);
    uint64_t getOpponentAttacksWithProtection(char piece);

    int getAllLegalMovesAsArray(std::pair<int, int> movesList[], bool maximizingPlayer);
    uint16_t *getAllLegalMoves(bool maximizingPlayer);
    int getSquareIndex(const std::string &square);
    std::pair<int, int> parseMove(const std::string &move);
    std::string moveToString(int fromSquare, int toSquare);

    void clearCapturedPiece(int square, char destPiece);

    Bitboard getWhitePawns() const { return whitePawns; };
    Bitboard getBlackPawns() const { return blackPawns; };
    char getPieceAtSquare(int square);
    uint64_t getOccupiedSquares();
    uint64_t getWhitePieces();
    uint64_t getBlackPieces();
    uint64_t getEmptySquares();

    int evaluatePosition();
    int evaluateMaterial();
    int evaluatePieceSet(Bitboard &bitboard, int value);

    Bitboard whitePawns;
    Bitboard blackPawns;
    Bitboard whiteKnights;
    Bitboard blackKnights;
    Bitboard whiteBishops;
    Bitboard blackBishops;
    Bitboard whiteRooks;
    Bitboard blackRooks;
    Bitboard whiteQueens;
    Bitboard blackQueens;
    Bitboard whiteKing;
    Bitboard blackKing;
    uint64_t enPassantTarget;
    uint64_t pinMasks[64];

    AttackTable attackTable;
    bool lastMoveEnPassant;
    int enpassantCapturedSquare;
    char enpassantCapturedPiece;

    bool blackCanCastleQ;
    bool blackCanCastleK;
    bool WhiteCanCastleQ;
    bool WhiteCanCastleK;

    std::vector<Move> moveHistory; 
    Move getLastMove();
    void storeMove(int from, int to, char capturedPiece, uint64_t enpSquare,
                   bool wasEnPassant, int enPassantCapturedSquare, char enPassantCapturedPiece, bool wasPromotion, char originalPawn);

    bool isValidMove(int from, int to);
    bool isCaptureMove(int fromSquare, int toSquare);

    void initializeZobrist();
    int pieceToIndex(char piece);
    uint64_t computeZobristHash();
    uint64_t zobristHash;
    static constexpr int PIECES = 12;  
    static constexpr int SQUARES = 64; 
    static constexpr int CASTLING_RIGHTS = 4;
    static constexpr int EN_PASSANT_FILES = 8;
    static constexpr int SIDES_TO_MOVE = 1;

    uint64_t zobristTable[PIECES][SQUARES];    
    uint64_t castlingTable[CASTLING_RIGHTS];   
    uint64_t enPassantTable[EN_PASSANT_FILES]; 
    uint64_t sideToMoveHash;

    void updateZobristHash(uint64_t newHash) { zobristHash = newHash; }
    uint64_t getZobristHash() const { return zobristHash; }
    std::vector<TranspositionEntry> transpositionTable;

    void initTranspositionTable(); 
    bool probeTranspositionTable(uint64_t hash, int depth, int alpha, int beta, std::pair<int, std::pair<int, int>> &result);
    void storeTransposition(uint64_t hash, int depth, int eval, int alpha, int beta, int from, int to);
    bool whiteToMove;

    uint64_t kingMovesTable[63];
    void precomputeKingMoves();

    uint64_t generateKingMoves(int square);
    bool isDraw(bool maximizingPlayer);
    bool isKingInCheck(bool maximizingPlayer);


    uint64_t checkers;
    uint64_t checkMask;
};

#endif // BOARD_H