#include <QApplication>
#include "ChessBoardWidget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    auto board = std::make_shared<Board>(); 

    ChessBoardWidget *widget = new ChessBoardWidget(board);
    widget->show();


    return app.exec();
}
