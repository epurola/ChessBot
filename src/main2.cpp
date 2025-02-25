#include <QApplication>
#include "ChessBoardWidget.h"
#include "Uci.h"
#include "ChessGameManager.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    auto board = std::make_shared<Board>(); 

   
    //ChessBoardWidget *widget = new ChessBoardWidget(board);
   // widget->show();

   Uci uci;
   uci.init();

   std::string command;
   while (std::getline(std::cin, command)) {  // Read input from UCI GUI or user
       uci.processCommand(command);
       if (command == "quit") {
           break;  // Exit loop on "quit" command
       }
   }

  
    return app.exec();
}