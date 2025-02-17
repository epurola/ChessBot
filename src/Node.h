#ifndef NODE_H
#define NODE_H

#include "Board.h"
#include <vector>
#include <utility> 
#include "Evaluation.h"
#include <iostream>
#include <unordered_map>
#include <random>
#include <cstdint>

class Node
{
public:
    Node() {}

    std::pair<int, std::pair<int, int>> iterativeDeepening(std::shared_ptr<Board> board, int maxDepth, bool maximizingPlayer, Evaluation &evaluate);

    std::pair<int, std::pair<int, int>> iterativeDeepeningM(std::shared_ptr<Board> board, int maxDepth, bool maximizingPlayer, Evaluation &evaluate, std::pair<int,int> moves[]);

    std::pair<int, std::pair<int, int>> minimaxM(std::shared_ptr<Board> board, int depth, bool maximizingPlayer, int alpha, int beta, Evaluation &evaluate, std::pair<int,int> moves[]);

    std::pair<int, std::pair<int, int>> setUpMultiThreading(std::shared_ptr<Board> board, int maxDepth, bool maximizingPlayer);

    std::pair<int, std::pair<int, int>> minimax(std::shared_ptr<Board> board, int depth, bool maximizingPlayer, int alpha, int beta, Evaluation &evaluate);

    int from = -10, to = -1;
    int score = 0;
    char capturedPiece;
    std::string FEN;
    int nodesExplored = 0;
    bool gameOver = false;
    int hits = 0;
    std::vector<std::pair<int, int>> previousBestMoves;
    std::unordered_map<int, std::pair<int, int>> killerMoves; 

private:
    void printMoves(const std::vector<std::string> &moves);
    std::pair<int, int> previousBestMove = {-1, -1};
};

#endif
