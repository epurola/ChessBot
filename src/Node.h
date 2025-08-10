
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

/**
 * @class Node
 * @brief Represents a root node in the game tree used for minimax search in chess.
 *
 * This class implements key functions for iterative deepening and minimax 
 * search with alpha-beta pruning, storing best moves and move-ordering heuristics.
 */
class Node
{
public:
    /**
     * @brief Default constructor initializing move and score values.
     *
     * Initializes the move variables to invalid positions (-10, -1),
     * score to 0, and sets gameOver to false.
     * Also initializes previous best moves with invalid values (-1, -1).
     */
    Node()
        : from(-10), to(-1), score(0), gameOver(false)
    {
        for (int i = 0; i < 7; ++i)
        {
            previousBestMoves[i] = {-1, -1};
        }
    }

    /**
     * @brief Implements Iterative Deepening Depth-First Search (IDDFS) with minimax.
     * 
     * This function iteratively calls the minimax function up to a given depth.
     * It ensures deeper searches are guided by previous best moves, improving efficiency.
     *
     * @param board Shared pointer to the current board state.
     * @param maxDepth Maximum search depth.
     * @param maximizingPlayer True if the current player is maximizing, false if minimizing.
     * @param evaluate Reference to the evaluation function.
     * @return The best move as a pair: (evaluation score, (from, to)).
     */
    std::pair<int, Move> iterativeDeepening(std::shared_ptr<Board> board, int maxDepth, bool maximizingPlayer, Evaluation &evaluate);

    /**
     * @brief Minimax algorithm with alpha-beta pruning and principal variation search (PV).
     *
     * This function evaluates possible moves recursively, using alpha-beta pruning 
     * to eliminate unnecessary branches, improving search efficiency.
     *
     * @param board Shared pointer to the current board state.
     * @param depth Remaining search depth.
     * @param maximizingPlayer True if maximizing, false if minimizing.
     * @param alpha Alpha pruning value (best max score found).
     * @param beta Beta pruning value (best min score found).
     * @param evaluate Reference to the evaluation function.
     * @param isPV True if this is a principal variation search.
     * @param pvMove Move in the principal variation to prioritize.
     * @return The best move as a pair: (evaluation score, (from, to)).
     */
    std::pair<int, Move> minimax(std::shared_ptr<Board> board, int depth, bool maximizingPlayer, int alpha, int beta, Evaluation &evaluate, bool isPV, std::pair<int, int> pvMove);

      /**
     * @brief Standard minimax function with alpha-beta pruning.
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
     * @return The best move as a pair: (evaluation score, (from, to)).
     */
    std::pair<int, Move> minimax(Board& board, int depth, bool maximizingPlayer, int alpha, int beta, Evaluation &evaluate);

    /** Move origin square index. Initialized to -10 (invalid). */
    int from = -10; 

    /** Move destination square index. Initialized to -1 (invalid). */
    int to = -1;

    /** Score of the current node, used in evaluation functions. */
    int score = 0;

    /** Number of nodes explored in the search. */
    int nodesExplored = 0;

    /** Indicates if the game has reached a terminal state (checkmate, draw). */
    bool gameOver = false;

    /** Stores previous best moves for move ordering optimizations (used in iterative deepening). */
    Move previousBestMoves[7];

    /** Stores killer moves (strong moves from previous searches) to improve move ordering. */
    std::unordered_map<int, Move> killerMoves;

};

#endif // NODE_H
