#include "Node.h"
#include <iostream>
#include <algorithm>
#include "Board.h"
#include "Evaluation.h"
#include <thread>
#include <future>
#include <limits>

/*
       // checks if opponent lost
       board->computeZobristHash();
       uint64_t positionHash = board->getZobristHash();


       TTEntry entry;  // Declare a TranspositionEntry to store the probe result

       // Probe the transposition table
       if (board->probeTranspositionTable(positionHash, depth, alpha, beta, entry))
       {
           int transpositionEval = entry.evaluation;
           if (depth <= entry.depth)
           {
               if (entry.flag == TTFlag::EXACT)
               {
                   return {entry.evaluation, {entry.bestFrom, entry.bestTo}};
               }
               else if (entry.flag == TTFlag::LOWERBOUND && transpositionEval > alpha)
               {
                   return {entry.evaluation, {entry.bestFrom, entry.bestTo}};
               }
               else if (entry.flag == TTFlag::UPPERBOUND && transpositionEval < beta)
               {
                   return {entry.evaluation, {entry.bestFrom, entry.bestTo}};
               }
           }
       }

      */

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

        if (bestEval == std::numeric_limits<int>::max() || bestEval == std::numeric_limits<int>::min() || board->isThreefoldRepetition())
        {
            break;
        }
        std::cout << "Depth " << depth << " completed. Best Move: (" << bestMove.first
                  << " -> " << bestMove.second << "), Eval: " << bestEval
                  << ", Nodes explored: " << nodesExplored << std::endl;
    }

    std::cout << "Iterative Deepening Complete. Best Move: (" << bestMove.first << " -> " << bestMove.second << "), Final Eval: " << bestEval << std::endl;
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

    if (board->isThreefoldRepetition())
    {
        if (maximizingPlayer)
        {
            return {alpha >= 0 ? -30 : 0, {-11, -1}};
        }
        else
        {
            return {beta <= 0 ? 30 : 0, {-11, -1}};
        }
    }

    board->computeZobristHash();
    uint64_t positionHash = board->getZobristHash();

    board->moveCount = 0;

    TTEntry entry; // Declare a TranspositionEntry to store the probe result

    // Probe the transposition table
    if (board->probeTranspositionTable(positionHash, depth, alpha, beta, entry))
    {
        int transpositionEval = entry.evaluation;
        if (depth <= entry.depth)
        {
            if (entry.flag == TTFlag::EXACT)
            {
                return {entry.evaluation, {entry.bestFrom, entry.bestTo}};
            }
            else if (entry.flag == TTFlag::LOWERBOUND && transpositionEval > alpha)
            {
                return {entry.evaluation, {entry.bestFrom, entry.bestTo}};
            }
            else if (entry.flag == TTFlag::UPPERBOUND && transpositionEval < beta)
            {
                return {entry.evaluation, {entry.bestFrom, entry.bestTo}};
            }
        }
    }

    constexpr int MAX_MOVES = 256;
    std::pair<int, int> moves[MAX_MOVES];
    std::pair<int, int> moveData = board->getAllLegalMovesAsArray(moves, maximizingPlayer);
    int moveCount = moveData.first;
    int nonCaptureStart = moveData.second;

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

    for (int i = 0; i < 7; i++)
    {
        for (int j = 0; j < moveCount; j++)
        {
            if (moves[j] == previousBestMoves[i])
            {
                std::swap(moves[0], moves[j]);
                break;
            }
        }
    }

    if (killerMoves[depth] != std::pair<int, int>{-1, -1})
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
    std::pair<int, int> bestMove = {moves[0].first, moves[0].second};

    for (int i = 0; i < moveCount; i++)
    {

        int moveFrom = moves[i].first;
        int moveTo = moves[i].second;

        board->movePiece(moveFrom, moveTo);
        Move lastMove = board->getLastMove();

        //bool isLateMove = (i >= 20) && (depth >= 3);
        
        //int reduction = isLateMove ? 1 : 0;
        int newDepth = depth - 1 ;
        int childScore = minimax(board, newDepth, !maximizingPlayer, alpha, beta, evaluate).first;

        nodesExplored++;

        //  Undo Move
        board->undoMove(lastMove.from, lastMove.to, lastMove.capturedPiece, lastMove.enpSquare, lastMove.wasEnPassant,
                        lastMove.enPassantCapturedSquare, lastMove.enPassantCapturedPiece,
                        lastMove.wasPromotion, lastMove.originalPawn, lastMove.WhiteCastleKBefore,
                        lastMove.WhiteCastleQBefore, lastMove.BlackCastleKBefore, lastMove.BlackCastleQBefore, lastMove.hash, lastMove.whiteTurn);

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
        // Check if bestMove is already in the previousBestMoves array
        bool found = false;
        for (int i = 0; i < 4; i++)
        {
            if (previousBestMoves[i] == bestMove)
            {
                found = true;
                break; // No need to add again, just exit the loop
            }
        }

        if (!found) // Only add if it was not found
        {
            // Shift all moves down by one (to make space for the new move at the front)
            for (int i = 3; i > 0; i--)
            {
                previousBestMoves[i] = previousBestMoves[i - 1];
            }
            // Insert the new bestMove at the front (index 0)
            previousBestMoves[0] = bestMove;
        }
    }

    if (bestMove.second != -1)
    {
        killerMoves[depth] = bestMove;
    }

    board->storeTransposition(positionHash, depth, bestScore, alpha, beta, bestMove.first, bestMove.second);

    return {bestScore, bestMove};
}


/*if (bestMove.first != -1 && bestMove.second != -1)
    {
        // Check if bestMove is already in the previousBestMoves array
        bool found = false;
        for (int i = 0; i < 7; i++)
        {
            if (previousBestMoves[i] == bestMove)
            {
                found = true;
                break; // No need to add again, just exit the loop
            }
        }

        if (!found) // Only add if it was not found
        {
            // Shift all moves down by one (to make space for the new move at the front)
            for (int i = 6; i > 0; i--)
            {
                previousBestMoves[i] = previousBestMoves[i - 1];
            }
            // Insert the new bestMove at the front (index 0)
            previousBestMoves[0] = bestMove;
        }
    }

    if (bestMove.second != -1)
    {
        killerMoves[depth] = bestMove;
    }*/

/*   for (int i = 0; i < 7; i++)
{
    for (int j = 0; j < moveCount; j++)
    {
        if (moves[j] == previousBestMoves[i])
        {
            // Swap the current best move with the move at position 0
            std::swap(moves[0], moves[j]);
            break; // No need to check further, we already moved this one to the front
        }
    }
}
if (killerMoves[depth] != std::pair<int, int>{-1, -1}) // Check if there is a valid killer move
{
    std::pair<int, int> killerMove = killerMoves[depth];

    for (int i = 0; i < moveCount; i++)
    {
        if (moves[i] == killerMove)
        {
            std::swap(moves[0], moves[i]); // Swap the killer move to the front
            break;                         // Exit after swapping
        }
    }
}*/