#include "Uci.h"

/**
 * Initializes the UCI engine by printing engine details and setting up the board.
 */
void Uci::init()
{
    std::cout << "BotFish" << std::endl;
    std::cout << "Eeli Purola" << std::endl;
    std::cout << "uciok" << std::endl;

    board = std::make_shared<Board>(); // Initialize the internal board
}

/**
 * Processes incoming UCI commands and delegates them to appropriate handlers.
 * 
 * @param command The command string received from the user.
 */
void Uci::processCommand(const std::string &command)
{
    if (command.empty())
        return;
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == "uci")
    {
        handleUci();
    }
    else if (cmd == "isready")
    {
        handleIsReady();
    }
    else if (cmd == "setoption")
    {
        std::string option;
        std::getline(iss, option);
        handleSetOption(option);
    }
    else if (cmd == "ucinewgame")
    {
        handleUciNewGame();
    }
    else if (cmd == "position")
    {
        std::string position;
        std::getline(iss, position);
        handlePosition(position);
    }
    else if (cmd == "go")
    {
        std::string parameters;
        std::getline(iss, parameters);
        handleGo(parameters);
    }
    else if (cmd == "stop")
    {
        handleStop();
    }
    else if (cmd == "quit")
    {
        handleQuit();
    }
    else
    {
        std::cerr << "Unknown command: " << command << std::endl;
    }
}

/**
 * Handles the "uci" command by responding with engine name and author.
 */
void Uci::handleUci()
{
    std::cout << "id name MyChessEngine" << std::endl;
    std::cout << "id author YourName" << std::endl;
    std::cout << "uciok" << std::endl;
}

/**
 * Handles the "isready" command by confirming engine readiness.
 */
void Uci::handleIsReady()
{
    std::cout << "readyok" << std::endl;
}

/**
 * Handles the "setoption" command, allowing configuration of engine parameters.
 * 
 * @param option The option string received (expected format: "setoption <value>").
 */
void Uci::handleSetOption(const std::string &option)
{
    if (option.empty())
    {
        std::cerr << "Invalid option command received!" << std::endl;
        return;
    }

    size_t spacePos = option.find(" ");

    if (spacePos != std::string::npos)
    {
        std::string depthValue = option.substr(spacePos + 1);
        try
        {
            depth = std::stoi(depthValue);
            std::cout << "Option set: depth = " << depth << std::endl;
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "Invalid depth value received: " << depthValue << std::endl;
        }
    }
    else
    {
        std::cerr << "Invalid option command format! Expected format: 'depth <value>'" << std::endl;
    }
}

/**
 * Handles the "ucinewgame" command by resetting the board for a new game.
 */
void Uci::handleUciNewGame()
{
    board->resetBoard(); // Reset board
    std::cout << "New game started" << std::endl;
}

/**
 * Handles the "position" command by setting the board position from a FEN string.
 * 
 * @param position The FEN string received.
 */
void Uci::handlePosition(const std::string &position)
{
    if (!board)
    {
        std::cerr << "Error: Board is uninitialized!" << std::endl;
        return;
    }

    if (position.empty())
    {
        std::cerr << "Error: Received empty position string!" << std::endl;
        return;
    }
    board->setFen(trimLeadingSpace(position));
    std::cout << "Position set:" << position << std::endl;
}

/**
 * Trims leading spaces from a string.
 * 
 * @param str The input string.
 * @return The trimmed string without leading spaces.
 */
std::string Uci::trimLeadingSpace(const std::string &str)
{
    size_t start = str.find_first_not_of(' ');
    return (start == std::string::npos) ? "" : str.substr(start);
}

/**
 * Handles the "go" command, initiating move search using iterative deepening.
 * 
 * @param parameters Search parameters (e.g., time controls, depth constraints).
 */
void Uci::handleGo(const std::string &parameters)
{
    std::cout << "Go command received with parameters: " << parameters << std::endl;

    if (!board)
    {
        std::cerr << "Error: Board is uninitialized!" << std::endl;
        return;
    }

    Node root;
    Evaluation evaluate(board);

    auto start = std::chrono::high_resolution_clock::now();

    auto [bestScore, bestmove] = root.iterativeDeepening(board, depth, board->whiteToMove, evaluate);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Search took " << duration.count() << " seconds." << std::endl;

    if (bestmove.first == -1 || bestmove.second == -1)
    {
        std::cerr << "Error: No valid move found!" << std::endl;
        return;
    }

    std::cout << "bestmove " << board->moveToString(bestmove.first, bestmove.second) << std::endl;

    applyBestMove(bestmove);
}

/**
 * Applies the best move found by the engine.
 * 
 * @param bestmove The best move as a pair of integers.
 */
void Uci::applyBestMove(const std::pair<int, int> &bestmove)
{
    if (!board)
    {
        std::cerr << "Error: Board is uninitialized!" << std::endl;
        return;
    }

    board->movePiece(bestmove.first, bestmove.second);
}

/**
 * Handles the "stop" command, halting the search process.
 */
void Uci::handleStop()
{
    std::cout << "Engine stopped " << board->getFen() << std::endl;
}

/**
 * Handles the "quit" command, shutting down the engine.
 */
void Uci::handleQuit()
{
    std::cout << "Engine quitting" << std::endl;
    exit(0);
}
