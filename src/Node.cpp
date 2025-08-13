#include "Node.h"
#include "Board.h"
#include "Evaluation.h"

constexpr int NEG_INF = std::numeric_limits<int>::min();
constexpr int POS_INF = std::numeric_limits<int>::max();
constexpr Move NULL_MOVE{-1, -1};
constexpr Move RESIGN_MOVE{-4, -1};
constexpr Move STALEMATE_MOVE{-2, -1};

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
        return {eval, {NULL_MOVE}};
    }

    if (board.isThreefoldRepetition())
    {
        if (maximizingPlayer)
            return {alpha >= 0 ? -30 : 0, NULL_MOVE};
        else
            return {beta <= 0 ? 30 : 0, NULL_MOVE};
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
            if (entry.flag == TTFlag::EXACT) [[likely]]
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

    Move moves[256];
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
            return {eval, NULL_MOVE};
        }
        else [[likely]]
        {
            return {0, NULL_MOVE};
        }
    }

    int curr = 0;
    for (int j = 0; j < moveCount; ++j)
    {
        if (std::find(std::begin(previousBestMoves),
                      std::end(previousBestMoves), moves[j]) != std::end(previousBestMoves))
        {
            std::swap(moves[curr++], moves[j]);
        }
    }

    if (killerMoves[depth] != NULL_MOVE)
    {
        Move killerMove = killerMoves[depth];

        auto it = std::find(moves, moves + moveCount, killerMove);
        if (it != moves + moveCount)
        {
            std::swap(moves[0], *it);
        }
    }

    int bestScore = maximizingPlayer ? NEG_INF : POS_INF;
    Move bestMove = {moves[0].from, moves[0].to};

    for (int i = 0; i < moveCount; ++i)
    {
        int moveFrom = moves[i].from;
        int moveTo = moves[i].to;

        board.movePiece(moveFrom, moveTo); // Make this return last move
        lastMove = board.getLastMove();

        int newDepth = depth - 1;
        int childScore;

        if (depth >= 6 && i >= 7)
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

        // nodesExplored++;

        board.undoMove(lastMove);

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

    if (bestMove != NULL_MOVE) [[likely]]
    {
        // Check if bestMove is already in the previousBestMoves array
        if (std::find(std::begin(previousBestMoves),
                      std::end(previousBestMoves),
                      bestMove) == std::end(previousBestMoves))
        {
            std::move_backward(std::begin(previousBestMoves),
                               std::end(previousBestMoves) - 1,
                               std::end(previousBestMoves));
            previousBestMoves[0] = bestMove;
        }
        killerMoves[depth] = bestMove;
    }else [[unlikely]]
    {
        bestMove = moves[0];
        std::cout << "Null move returned" << std::endl;
    }

    board.storeTransposition(positionHash, depth, bestScore, alpha, beta, bestMove.from, bestMove.to);

    return {bestScore, bestMove};
}