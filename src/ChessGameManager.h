#ifndef CHESSGAMEMANAGER_H
#define CHESSGAMEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <memory>
#include "Board.h"
#include <QLabel>
#include "ChessBoardWidget.h"

class ChessGameManager : public QObject {
    Q_OBJECT

    signals:
    void boardUpdated();

public:
    explicit ChessGameManager(std::shared_ptr<Board> board, QObject *parent = nullptr);
    void startGame();  // Start AI vs AI
    void stopGame();   // Stop engines
    void restartGame(); // Reset the game and score

private slots:
    void readEngine1Output();
    void readEngine2Output();

private:
    void sendCommandToEngine(QProcess *engine, const QString &command);
    void processMove(const QString &move, bool isWhite);
    void checkGameOver();

    std::shared_ptr<Board> board;
    QProcess engine1;  // First engine process
    QProcess engine2;  // Second engine process
    bool isWhiteTurn;
    int whiteWinCount =0;
    int blackWinCount =0;
    int drawCount =0;
    QLabel *whiteWins = new QLabel("Engine 1 Wins: 0");
    QLabel *blackWins = new QLabel("Engine 2 Wins: 0");
    QLabel *draw = new QLabel("Draws: 0");

    std::vector<std::string> positionStrings[20];
    void initPositions();
    ChessBoardWidget *chessBoard;
};

#endif // CHESSGAMEMANAGER_H
