#ifndef BOARD_H
#define BOARD_H

#include "BitBoard.h"
#include <cctype>
#include <cstdlib>
#include <AttackTable.h>
#include <unordered_map>
#include <random>
#include <cstdint>
#include <iostream>
#include <bitset>
#include <array>
#include <sstream>

constexpr int TABLE_SIZE = 20971521;

constexpr size_t MAX_MOVES = 512;


enum TTFlag
{
    EXACT,
    LOWERBOUND,
    UPPERBOUND
};
struct TTEntry
{
    int evaluation;
    int depth;
    int bestFrom;
    int bestTo;
    TTFlag flag;
    uint64_t hash;

    TTEntry(int eval = 0, int d = 0, TTFlag f = EXACT, int bf = -1, int bt = -1)
        : evaluation(eval), depth(d), flag(f), bestFrom(bf), bestTo(bt), hash(0) {}
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
    bool WhiteCastleKBefore, WhiteCastleQBefore;
    bool BlackCastleKBefore, BlackCastleQBefore;
    uint64_t hash;
    bool whiteTurn;
};

class Board
{
public:
    Board();
    Board(const std::shared_ptr<Board> &other);

    void setFen(const std::string &fen);
    std::string getFen();
    void resetBoard();


    //Move handling
    bool movePiece(int from, int to);
    void undoMove(int from, int to, char capturedPiece, uint64_t enpSquare,
        bool lastMoveEnPassant, 
        int enpassantCapturedSquare, 
        char enpassantCapturedPiece,
        bool wasPromotion, 
        char originalPawn, 
        bool WhiteCastleKBefore,
        bool WhiteCastleQBefore, 
        bool BlackCastleKBefore, 
        bool BlackCastleQBefore, 
        uint64_t hash, 
        bool whiteTurn);
    void restoreCapturedPiece(int square, char piece);
    bool updateBitboards(char piece, int from, int to);
    bool isValidMove(int from, int to);
    bool isCaptureMove(int fromSquare, int toSquare);
    void clearCapturedPiece(int square, char destPiece);

    //Move generation
    uint64_t generatePawnMovesForKing(int square, char piece);
    uint64_t generateRookMovesForKing(int square, char piece);

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
    int64_t generateKingMoves(int square);

    uint64_t generateKnightMovesWithProtection(int square, char piece);
    uint64_t generateBishopMovesWithProtection(int square, char piece);
    uint64_t generateRookMovesWithProtection(int square, char piece);
    uint64_t generateQueenMovesWithProtection(int square, char piece);
    uint64_t getOpponentAttacksWithProtection(char piece);

    uint16_t *getAllLegalMoves(bool maximizingPlayer);
    std::pair<int, int> getAllLegalMovesAsArray(std::pair<int, int> movesList[], bool maximizingPlayer);

    //Move gen helpers
    uint64_t findCheckers(int squareOfKing, char king, uint64_t &checkMask);
    uint64_t getOpponentAttacks(char piece);
    uint64_t findPinnedPieces(int squareOfKing, char piece);
    bool isPiecePinnedToKing(int pieceSquare, int squareOfKing, uint64_t lineOfSight);
    bool isPiecePinned(int pieceSquare, int squareOfKing, char king);
    int countAttackedSquares(int square, char piece);

    //Helpers
    bool gameOver(bool maximizingPlayer);
    void printBitboard(uint64_t bitboard, const std::string &label);

    int bitScanForward(uint64_t bitboard);
    int pieceValue(char piece);
    int getSquareIndex(const std::string &square);
    int pieceToIndex(char piece);

    std::pair<int, int> parseMove(const std::string &move);
    std::string moveToString(int fromSquare, int toSquare);

    Bitboard getWhitePawns() const { return whitePawns; };
    Bitboard getBlackPawns() const { return blackPawns; };

    uint64_t getOccupiedSquares();
    uint64_t getWhitePieces();
    uint64_t getBlackPieces();
    uint64_t getEmptySquares();

    char getPieceAtSquare(int square);
    bool isThreefoldRepetition();
    bool isDraw(bool maximizingPlayer);
    bool isKingInCheck(bool maximizingPlayer);
    void precomputeKingMoves();
    

    //ZobristHash
    void initializeZobrist();
    uint64_t computeZobristHash();
    void updateZobristHash(uint64_t newHash) { zobristHash = newHash; }
    uint64_t getZobristHash() const { return zobristHash; }

    //Transposition Table
    bool probeTranspositionTable(uint64_t hash, int depth, int alpha, int beta, TTEntry &entry);
    void storeTransposition(uint64_t hash, int depth, int eval, int alpha, int beta, int from, int to);

    
    //Bitboards
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
    uint64_t kingMovesTable[63];

    AttackTable attackTable;
    bool lastMoveEnPassant;
    int enpassantCapturedSquare;
    char enpassantCapturedPiece;

    //castling
    bool blackCanCastleQ;
    bool blackCanCastleK;
    bool WhiteCanCastleQ;
    bool WhiteCanCastleK;

    //Move History
    std::array<Move, MAX_MOVES> moveHistory;
    size_t moveCount = 0;
    Move getLastMove();
    void storeMove(int from, int to, char capturedPiece, uint64_t enpSquare,
                   bool wasEnPassant, 
                   int enPassantCapturedSquare,
                   char enPassantCapturedPiece, 
                   bool wasPromotion, 
                   char originalPawn,
                   bool WhiteCastleKBefore,
                   bool WhiteCastleQBefore, 
                   bool BlackCastleKBefore, 
                   bool BlackCastleQBefore, 
                   uint64_t hash, 
                   bool whiteTurn);

    //Zobrist hashing variables
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

    TTEntry transpositionTable[TABLE_SIZE]; 

    bool whiteToMove;

    std::unordered_map<uint64_t, int> gameFensHistory;
};

#endif // BOARD_H