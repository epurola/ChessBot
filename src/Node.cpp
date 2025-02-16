#include "Node.h"
#include <iostream>
#include <algorithm>
#include "Board.h"
#include "Evaluation.h"

std::pair<int, std::pair<int, int>> Node::iterativeDeepening(std::shared_ptr<Board> board, int maxDepth, bool maximizingPlayer, Evaluation &evaluate)
{
    std::pair<int, int> bestMove = {-5, -1};
    int bestEval = maximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    

    for (int depth = 1; depth <= maxDepth; depth++)
    {
        gameOver = false;
        nodesExplored = 0;
        std::pair<int, std::pair<int, int>> result = minimax(board, depth, maximizingPlayer, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), evaluate);
        bestEval = result.first;
        bestMove = result.second;

        if (bestEval == std::numeric_limits<int>::max() || bestEval == std::numeric_limits<int>::min())
        {
            break;
        }
        std::cout << "[DEBUG] Depth " << depth << " completed. Best Move: (" << bestMove.first
                  << " -> " << bestMove.second << "), Eval: " << bestEval
                  << ", Nodes explored: " << nodesExplored << std::endl;
    }

    std::cout << "[DEBUG] Iterative Deepening Complete. Best Move: (" << bestMove.first << " -> " << bestMove.second << "), Final Eval: " << bestEval << std::endl;
    std::cout << "Nodes: " << nodesExplored << std::endl;

    return {bestEval, bestMove};
}

std::pair<int, std::pair<int, int>> Node::minimax(std::shared_ptr<Board> board, int depth, bool maximizingPlayer, int alpha, int beta, Evaluation &evaluate)
{
    if (depth == 0)
    {
        int eval = evaluate.evaluatePosition();
        return {eval, {-4, -1}};
    }
/*
    // checks if opponent lost
     board->computeZobristHash();
    uint64_t positionHash = board->getZobristHash();


    TranspositionEntry entry;  // Declare a TranspositionEntry to store the probe result

    // Probe the transposition table
    if (board->probeTranspositionTable(positionHash, depth, alpha, beta, entry))
    {
        int transpositionEval = entry.evaluation;
        int transpositionFrom = entry.bestFrom;
        int transpositionTo = entry.bestTo;
    
    
        if (depth <= entry.depth)
        {
            if (entry.flag == EXACT)
            {
               
                return {entry.evaluation, {entry.bestFrom, entry.bestTo}};
            }
            else if (entry.flag == LOWER_BOUND && transpositionEval > alpha)
            {
             
                return {entry.evaluation, {entry.bestFrom, entry.bestTo}};
            }
            else if (entry.flag == UPPER_BOUND && transpositionEval < beta)
            {
          
                return {entry.evaluation, {entry.bestFrom, entry.bestTo}};
            }
        }
    }
    */

    constexpr int MAX_MOVES = 256;
    std::pair<int, int> moves[MAX_MOVES];
    int moveCount = board->getAllLegalMovesAsArray(moves, maximizingPlayer);

    if (moveCount == 0)
    {
        gameOver = true;
        nodesExplored++;

        if (board->isKingInCheck(maximizingPlayer))
        {
            int eval = maximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
            return {eval, {-3, -1}};
        }
        else
        {
            return {0, {-2, -1}};
        }
    }

    // Prioritize the previous best move by moving it to the front
    for (size_t i = 0; i < previousBestMoves.size(); i++)
    {
        for (int j = 0; j < moveCount; j++)
        {
            if (moves[j] == previousBestMoves[i])
            {
                std::swap(moves[0], moves[j]);
            }
        }
    }

    if (killerMoves.find(depth) != killerMoves.end())
    {
        std::pair<int, int> killerMove = killerMoves[depth];
        for (int i = 0; i < moveCount; i++)
        {
            if (moves[i] == killerMove)
            {
                std::swap(moves[0], moves[i]);
                break;
            }
        }
    }

    int bestScore = maximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    std::pair<int, int> bestMove = {moves[1].first, moves[1].second};

    for (int i = 0; i < moveCount; i++)
    {

        int moveFrom = moves[i].first;
        int moveTo = moves[i].second;

        board->movePiece(moveFrom, moveTo);
        Move lastMove = board->getLastMove();

        //  Recurse
        int childScore = minimax(board, depth - 1, !maximizingPlayer, alpha, beta, evaluate).first;

        nodesExplored++;

        //  Undo Move
        board->undoMove(lastMove.from, lastMove.to, lastMove.capturedPiece, lastMove.enpSquare, lastMove.wasEnPassant,
                        lastMove.enPassantCapturedSquare, 
                        lastMove.enPassantCapturedPiece, lastMove.wasPromotion, 
                        lastMove.originalPawn);

        if (maximizingPlayer)
        {
            if (childScore > bestScore)
            {

                bestScore = childScore;
                bestMove = {moveFrom, moveTo};
            }
            alpha = std::max(alpha, bestScore);
        }
        else
        {
            if (childScore < bestScore)
            {
                bestScore = childScore;
                bestMove = {moveFrom, moveTo};
            }
            beta = std::min(beta, bestScore);
        }

        if (beta <= alpha)
            break; //  Alpha-Beta Pruning
    }

    if (bestMove.first != -1 && bestMove.second != -1)
    {
        auto it = std::find(previousBestMoves.begin(), previousBestMoves.end(), bestMove);
        if (it == previousBestMoves.end())
        {
            previousBestMoves.push_back(bestMove);
            if (previousBestMoves.size() > 7)
            {
                previousBestMoves.erase(previousBestMoves.begin());
            }
        }
    }
    if (bestMove.first != -1 && bestMove.second != -1)
    {
        killerMoves[depth] = bestMove;
    }
/*
    TranspositionFlag flag = EXACT; // Assume we have an exact value
    if (bestScore <= alpha)
    {
        flag = UPPER_BOUND;
    }
    else if (bestScore >= beta)
    {
        flag = LOWER_BOUND;
    }

    board->storeTransposition(positionHash, depth, bestScore, alpha, beta, bestMove.first, bestMove.second, flag);
*/
    return {bestScore, bestMove};
}

void Node::printMoves(const std::vector<std::string> &moves)
{
    std::cout << "Legal moves: " << std::endl;
    for (const std::string &moveStr : moves)
    {
        std::cout << moveStr << std::endl;
    }
}
