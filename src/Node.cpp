#include "Node.h"
#include <iostream>
#include <algorithm>
#include "Board.h"
#include "Evaluation.h"
#include <thread>
#include <future>
#include <limits>

std::pair<int, std::pair<int, int>> Node::setUpMultiThreading(std::shared_ptr<Board> board, int maxDepth, bool maximizingPlayer){
       
    constexpr int MAX_MOVES = 256;
    std::pair<int, int> startingMoves[MAX_MOVES];
    int moveCount = board->getAllLegalMovesAsArray(startingMoves, maximizingPlayer);
    std::cout << "Move count: " << moveCount << std::endl;
 // Shared best move


    std::shared_ptr<Board> board1 = std::make_shared<Board>(*board);
    std::shared_ptr<Board> board2 = std::make_shared<Board>(*board);
    std::shared_ptr<Board> board3 = std::make_shared<Board>(*board);
    std::shared_ptr<Board> board4 = std::make_shared<Board>(*board);

    //Divide moves array equally between threads
    int movesPerThread = moveCount / 4;
    std::pair<int, int> moves1[MAX_MOVES];
    std::pair<int, int> moves2[MAX_MOVES];
    std::pair<int, int> moves3[MAX_MOVES];
    std::pair<int, int> moves4[MAX_MOVES];

    for (int i = 0; i < moveCount; i++)
    {
        if (i < movesPerThread)
        {
            moves1[i] = startingMoves[i];
        }
        else if (i < movesPerThread * 2)
        {
            moves2[i - movesPerThread] = startingMoves[i];
        }
        else if (i < movesPerThread * 3)
        {
            moves3[i - movesPerThread * 2] = startingMoves[i];
        }
        else
        {
            moves4[i - movesPerThread * 3] = startingMoves[i];
        }
    }

    moves1[movesPerThread] = std::make_pair(-1, 0);
    moves2[movesPerThread] = std::make_pair(-1, 0);
    moves3[movesPerThread] = std::make_pair(-1, 0);
    moves4[movesPerThread] = std::make_pair(-1, 0);

    std::promise<std::pair<int, std::pair<int, int>>> p1, p2, p3, p4;
    std::future<std::pair<int, std::pair<int, int>>> f1 = p1.get_future();
    std::future<std::pair<int, std::pair<int, int>>> f2 = p2.get_future();
    std::future<std::pair<int, std::pair<int, int>>> f3 = p3.get_future();
    std::future<std::pair<int, std::pair<int, int>>> f4 = p4.get_future();

   std::thread t1([&](){
       Evaluation evaluate(board1);
       Node node1;
       p1.set_value(node1.iterativeDeepeningM(std::make_shared<Board>(board1), maxDepth, maximizingPlayer, evaluate, moves1));
       std::cout << "Thread 1 finished" << std::endl;
   });
   std::thread t2([&](){
       Evaluation evaluate(board2);
       Node node2;
       p2.set_value(node2.iterativeDeepeningM(board2, maxDepth, maximizingPlayer, evaluate, moves2));
       std::cout << "Thread 2 finished" << std::endl;
   });
   std::thread t3([&](){
       Evaluation evaluate(board3);
       Node node3;
       p3.set_value(node3.iterativeDeepeningM(board3, maxDepth, maximizingPlayer, evaluate, moves3));
       std::cout << "Thread 3 finished" << std::endl;
   });
   std::thread t4([&](){
       Evaluation evaluate(board4);
       Node node4;
       p4.set_value(node4.iterativeDeepeningM(board4, maxDepth, maximizingPlayer, evaluate, moves4));
       std::cout << "Thread 4 finished" << std::endl;
   });

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    std::pair<int, std::pair<int, int>> bestMove1 = f1.get();
    std::pair<int, std::pair<int, int>> bestMove2 = f2.get();
    std::pair<int, std::pair<int, int>> bestMove3 = f3.get();
    std::pair<int, std::pair<int, int>> bestMove4 = f4.get();

    // **Select the best move among results**
    std::vector<std::pair<int, std::pair<int, int>>> results = {bestMove1, bestMove2, bestMove3, bestMove4};
    std::pair<int, std::pair<int, int>> bestMove = results[0];

    for (const auto& result : results) {
        if ((maximizingPlayer && result.first > bestMove.first) ||
            (!maximizingPlayer && result.first < bestMove.first)) {
            bestMove = result;
        }
    }

    return bestMove;

}

std::pair<int, std::pair<int, int>> Node::iterativeDeepeningM(std::shared_ptr<Board> board, int maxDepth, bool maximizingPlayer, Evaluation &evaluate, std::pair<int,int> moves[])
{
    std::pair<int, int> bestMove = {-5, -1};
    int bestEval = maximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    
    for (int depth = 1; depth <= maxDepth; depth++)
    {
        gameOver = false;
        nodesExplored = 0;
        std::pair<int, std::pair<int, int>> result = minimaxM(board, depth, maximizingPlayer, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), evaluate, moves);
        bestEval = result.first;
        bestMove = result.second;

        if (bestEval == std::numeric_limits<int>::max() || bestEval == std::numeric_limits<int>::min())
        {
            break;
        }
        
    }

    return {bestEval, bestMove};
}

std::pair<int, std::pair<int, int>> Node::minimaxM(std::shared_ptr<Board> board, int depth, bool maximizingPlayer, int alpha, int beta, Evaluation &evaluate, std::pair<int,int> moves[])
{
  
    if (depth == 0)
    {
        int eval = evaluate.evaluatePosition();
        return {eval, {-4, -1}};
    }

    constexpr int MAX_MOVES = 256;
    std::pair<int, int> localMoves[MAX_MOVES];
    int moveCount = 0;
    std::pair<int, int> bestMove;
    
    if (moves != nullptr) {
        // We're at the root: use the provided moves.
        // Assume the moves array is terminated with a sentinel (e.g., first element -1 when finished)
        while (moveCount < MAX_MOVES && moves[moveCount].first != -1) {
            localMoves[moveCount] = moves[moveCount];
            moveCount++;
 
        }
         bestMove = {moves[1].first, moves[1].second};
    } else {
        // Not at the root: generate moves normally.
        moveCount = board->getAllLegalMovesAsArray(localMoves, maximizingPlayer); 
        bestMove = {-1, -1};
    }
   
   // std::cout << "Move count: " << moveCount << " (" << localMoves[1].first << ", " << localMoves[1].second << ")" << std::endl;
  
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
            if (localMoves[j] == previousBestMoves[i])
            {
                std::swap(localMoves[0], localMoves[j]);
            }
        }
    }

    if (killerMoves.find(depth) != killerMoves.end())
    {
        std::pair<int, int> killerMove = killerMoves[depth];
        for (int i = 0; i < moveCount; i++)
        {
            if (localMoves[i] == killerMove)
            {
                std::swap(localMoves[0], localMoves[i]);
                break;
            }
        }
    }

    int bestScore = maximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
   
  

    for (int i = 0; i < moveCount; i++)
    {
       
        int moveFrom = localMoves[i].first;
        int moveTo   = localMoves[i].second;

        

        board->movePiece(moveFrom, moveTo);
        Move lastMove = board->getLastMove();
       
        //  Recurse
        int childScore = minimaxM(board, depth - 1, !maximizingPlayer, alpha, beta, evaluate, nullptr).first;

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
            if (previousBestMoves.size() > 3)
            {
                previousBestMoves.erase(previousBestMoves.begin());
            }
        }
    }
    if (bestMove.first != -1 && bestMove.second != -1)
    {
        killerMoves[depth] = bestMove;
    }

    return {bestScore, bestMove};
}


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
