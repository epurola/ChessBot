#ifndef CHESSBOARDWIDGET_H
#define CHESSBOARDWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QMouseEvent>
#include <QPoint>
#include <QLabel>
#include "board.h"
#include "Bitboard.h"
#include "Evaluation.h"
#include "Node.h"
#include <thread>
#include <chrono>
#include <QPainter>
#include <QDebug>
#include <QPushButton>

class ChessBoardWidget : public QWidget
{
    Q_OBJECT

    public slots:
    void updateBoard();

public:
    explicit ChessBoardWidget(std::shared_ptr<Board> board, QWidget *parent = nullptr);
    ~ChessBoardWidget();

    

    std::thread worker;

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    int getSquareIndex(const std::string &square);
    std::pair<int, int> parseMove(const std::string &move);
    void undoMove();
    void toggleAIMove();
    void toggleIsWhite();

private:
    std::shared_ptr<Board> board;
    QPixmap whitePawnPixmap;
    QPixmap blackPawnPixmap;
    QPixmap whiteRookPixmap;
    QPixmap blackRookPixmap;
    QPixmap whiteKnightPixmap;
    QPixmap blackKnightPixmap;
    QPixmap whiteBishopPixmap;
    QPixmap blackBishopPixmap;
    QPixmap whiteQueenPixmap;
    QPixmap blackQueenPixmap;
    QPixmap whiteKingPixmap;
    QPixmap blackKingPixmap;

    QLabel *evalLabel;
    bool aiMoveEnabled;

    bool isWhite;

    QPixmap *draggedPiece;
    int draggedSquare;
    std::vector<int> highlightedSquares;
    bool dragging;
    QPoint dragOffset;

    void loadImages();
    
};

#endif
