#ifndef UCI_H
#define UCI_H

#include <string>
#include "Board.h"
#include "Node.h"
#include <chrono>
#include <memory>


class Uci {
public:
    // Initialize UCI
    void init();

    // Process UCI commands
    void processCommand(const std::string& command);

private:
    // Handle the "uci" command
    void handleUci();

    // Handle the "isready" command
    void handleIsReady();

    // Handle the "setoption" command
    void handleSetOption(const std::string& option);

    // Handle the "ucinewgame" command
    void handleUciNewGame();

    // Handle the "position" command
    void handlePosition(const std::string& position);

    // Handle the "go" command
    void handleGo(const std::string& parameters);

    // Handle the "stop" command
    void handleStop();

    // Handle the "quit" command
    void handleQuit();
    
    //Apply the move on the internal board
    void applyBestMove(const Move& bestmove);

    //Remove extra space from the fen string
    std::string trimLeadingSpace(const std::string& str);

    //pointer to the board object
    std::shared_ptr<Board> board;

    //Initial depth set to 3
    int depth = 3;


};

#endif // UCI_H