#include "Node.h"
#include "Board.h"
#include "Evaluation.h"

constexpr int NEG_INF = std::numeric_limits<int>::min();
constexpr int POS_INF = std::numeric_limits<int>::max();

/**
 * @brief Implements Iterative Deepening Depth-First Search (IDDFS).
 *
 * Iterative deepening repeatedly calls the minimax function, incrementally increasing depth.
 * This ensures that shallower depths guide deeper searches, improving efficiency and move ordering.
 *
 * @param board Shared pointer to the current board state.
 * @param maxDepth Maximum search depth.
 * @param maximizingPlayer True if the current player is maximizing, false if minimizing.
 * @param evaluate Reference to the evaluation function.
 * @return Best move found as a pair: (evaluation score, (from, to)).
 */
std::pair<int, Move> Node::iterativeDeepening(std::shared_ptr<Board> board, int maxDepth, bool maximizingPlayer, Evaluation &evaluate)
{
    Move bestMove = {-5, -1};
    int bestEval = maximizingPlayer ? NEG_INF : POS_INF;

    for (int depth = 1; depth <= maxDepth; depth++)
    {
        gameOver = false;
        nodesExplored = 0;
        std::pair<int, Move> result = minimax(*board, depth, maximizingPlayer,
                                              NEG_INF,
                                              POS_INF,
                                              evaluate);
        bestEval = result.first;
        bestMove = result.second;

        if (bestEval == std::numeric_limits<int>::max() || bestEval == std::numeric_limits<int>::min() || board->isThreefoldRepetition())
        {
            break;
        }
        std::cout << "Depth " << depth << " completed. Best Move: (" << bestMove.from
                  << " -> " << bestMove.to << "), Eval: " << bestEval
                  << ", Nodes explored: " << nodesExplored << std::endl;
    }

    std::cout << "Iterative Deepening Complete. Best Move: (" << bestMove.from << " -> " << bestMove.to << "), Final Eval: " << bestEval << std::endl;
    std::cout << "Nodes: " << nodesExplored << std::endl;

    return {bestEval, bestMove};
}

/**
 * @brief Minimax search with alpha-beta pruning.
 *
 * This function searches for the best move up to a given depth, pruning
 * unpromising branches to improve performance.
 *
 * @param board Shared pointer to the current board state.
 * @param depth Remaining search depth.
 * @param maximizingPlayer True if maximizing, false if minimizing.
 * @param alpha Alpha pruning value.
 * @param beta Beta pruning value.
 * @param evaluate Reference to the evaluation function.
 * @return Best move as a pair: (evaluation score, (from, to)).
 */
std::pair<int, Move> Node::minimax(Board &board, int depth, bool maximizingPlayer, int alpha, int beta, Evaluation &evaluate)
{

    LastMove lastMove;

    if (depth == 0)
    {
        int eval = evaluate.evaluatePosition();
        return {eval, {-4, -1}};
    }

    if (board.isThreefoldRepetition())
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
    

    board.moveCount = 0;

    board.computeZobristHash();
    uint64_t positionHash = board.getZobristHash();

    TTEntry entry;

    // Probe the transposition table
    if (board.probeTranspositionTable(positionHash, depth, alpha, beta, entry))
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
    Move moves[MAX_MOVES];
    Move moveData = board.getAllLegalMovesAsArray(moves, maximizingPlayer);
    int moveCount = moveData.from;
    int nonCaptureStart = moveData.to;

    if (moveCount == 0)
    {
        gameOver = true;
        nodesExplored++;

        if (board.isKingInCheck(maximizingPlayer)) [[unlikely]]
        {
            int eval = maximizingPlayer ? NEG_INF : POS_INF;
            return {eval, {-3, -1}};
        }
        else [[likely]]
        {
            return {0, {-2, -1}};
        }
    }

    int curr = 0;
    for (int j = 0; j < moveCount; ++j)
    {
        // Check if moves[j] is in previousBestMoves
        for (int i = 0; i < 7; ++i)
        {
            if (moves[j] == previousBestMoves[i])
            {
                std::swap(moves[curr], moves[j]);
                curr++;
                break; 
            }
        }
    }

     if (killerMoves[depth] != Move{-1, -1})
    {
        Move killerMove = killerMoves[depth];

        for (int i = 0; i < moveCount; ++i)
        {
            if (moves[i] == killerMove)
            {
                std::swap(moves[0], moves[i]);
                break;
            }
        }
    }


    int bestScore = maximizingPlayer ? NEG_INF : POS_INF;
    Move bestMove = {moves[0].from, moves[0].to};

    for (int i = 0; i < moveCount; ++i)
    {

        int moveFrom = moves[i].from;
        int moveTo = moves[i].to;

        board.movePiece(moveFrom, moveTo);  // Make this return last move
        lastMove = board.getLastMove();

         int newDepth = depth - 1;
        int childScore;

    if (depth >= 3 && i >= 5 )
    {
        // Reduced depth search
        int reducedDepth = newDepth - 1;  
        childScore = minimax(board, reducedDepth, !maximizingPlayer, alpha, beta, evaluate).first;

        if (maximizingPlayer && childScore > alpha && childScore < beta)
        {
            childScore = minimax(board, newDepth, !maximizingPlayer, alpha, beta, evaluate).first;
        }
        else if (!maximizingPlayer && childScore < beta && childScore > alpha)
        {
            childScore = minimax(board, newDepth, !maximizingPlayer, alpha, beta, evaluate).first;
        }
    }
    else
    {
        // Normal full depth search
        childScore = minimax(board, newDepth, !maximizingPlayer, alpha, beta, evaluate).first;
    }

        nodesExplored++;

        board.undoMove(lastMove.from, lastMove.to, lastMove.capturedPiece, lastMove.enpSquare, lastMove.wasEnPassant,
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

        if (beta <= alpha) [[unlikely]]
            break; //  Alpha-Beta Pruning
    }

    if (bestMove.from != -1 || bestMove.to != -1)
    {
        // Check if bestMove is already in the previousBestMoves array
        bool found = false;
        for (int i = 0; i < 7; ++i)
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
            for (int i = 6; i > 0; --i)
            {
                previousBestMoves[i] = previousBestMoves[i - 1];
            }
            // Insert the new bestMove at the front (index 0)
            previousBestMoves[0] = bestMove;
        }
    }

    if (bestMove.from != -1)
    {
        killerMoves[depth] = bestMove;
    }

    board.storeTransposition(positionHash, depth, bestScore, alpha, beta, bestMove.from, bestMove.to);

    if(bestMove.from < 0 || bestMove.to < 0){
        bestMove = {moves[0]};
    }

    return {bestScore, bestMove};
}