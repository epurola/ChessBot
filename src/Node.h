
#ifndef NODE_H
#define NODE_H

#include <random>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include "Board.h" 
#include "Evaluation.h"  

class Node
{
public:
    Node() 
        : from(-10), to(-1), score(0), capturedPiece(0), gameOver(false), hits(0) 
    {
        for (int i = 0; i < 4; ++i)
        {
            previousBestMoves[i] = {-1, -1};
        }
    }

    std::pair<int, std::pair<int, int>> iterativeDeepening(std::shared_ptr<Board> board, int maxDepth, bool maximizingPlayer, Evaluation &evaluate);

    std::pair<int, std::pair<int, int>> minimax(std::shared_ptr<Board> board, int depth, bool maximizingPlayer, int alpha, int beta, Evaluation &evaluate, bool isPV, std::pair<int, int> pvMove);

    std::pair<int, std::pair<int, int>> minimax(std::shared_ptr<Board> board, int depth, bool maximizingPlayer, int alpha, int beta, Evaluation &evaluate);
  
    int pieceValue(char piece);

    int from = -10, to = -1;
    int score = 0;
    char capturedPiece = 0;
    std::string FEN;
    int nodesExplored = 0;
    bool gameOver = false;
    int hits = 0;
    std::pair<int, int> previousBestMoves[4]; 
    std::unordered_map<int, std::pair<int, int>> killerMoves;  

private:
    void printMoves(const std::vector<std::string>& moves);
    std::pair<int, int> previousBestMove = {-1, -1}; 
};

#endif // NODE_H
