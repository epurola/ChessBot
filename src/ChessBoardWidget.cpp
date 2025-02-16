#include "ChessboardWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include "Board.h"
#include <thread>

ChessBoardWidget::ChessBoardWidget(std::shared_ptr<Board> board, QWidget *parent)
    : QWidget(parent), board(board), dragging(false), draggedSquare(-1)
{
    setFixedSize(800, 800);
    loadImages();
    // Load images for chess pieces

    QPushButton *undoButton = new QPushButton("Undo Move", this);
    undoButton->setGeometry(0, 10, 120, 40);
    connect(undoButton, &QPushButton::clicked, this, &ChessBoardWidget::undoMove);

    QPushButton *toggleButton = new QPushButton("Toggle AI", this);
    toggleButton->setGeometry(650, 60, 120, 40);
    connect(toggleButton, &QPushButton::clicked, this, &ChessBoardWidget::toggleAIMove);

    QPushButton *isWhiteButton = new QPushButton("IsWhite", this);
    isWhiteButton->setGeometry(0, 60, 120, 40);
    connect(isWhiteButton, &QPushButton::clicked, this, &ChessBoardWidget::toggleIsWhite);

    isWhite = true;
    /*evalLabel = new QLabel(this);
      evalLabel->setGeometry(10, 10, 200, 30); // Set position and size of the label
      evalLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: red; }");

      evalLabel->setText("Eval: 0"); */
}

void ChessBoardWidget::toggleAIMove()
{

    Node root;
    Evaluation evaluate(board);
    auto start = std::chrono::high_resolution_clock::now();
    auto [bestScore, bestmove] = root.iterativeDeepening(board, 6, isWhite, evaluate);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    // Print the elapsed time in seconds
    std::cout << "Minimax search took " << duration.count() << " seconds." << std::endl;

    std::cout << "Making move: " << bestmove.first << "::" << bestmove.second << std::endl;
    board->movePiece(bestmove.first, bestmove.second);
    update();
}

void ChessBoardWidget::toggleIsWhite()
{
    isWhite = !isWhite;
}

void ChessBoardWidget::undoMove()
{
    Move lastMove = board->getLastMove();

    board->undoMove(lastMove.from, lastMove.to, lastMove.capturedPiece, lastMove.enpSquare, lastMove.wasEnPassant,
                    lastMove.enPassantCapturedSquare, lastMove.enPassantCapturedPiece, 
                    lastMove.wasPromotion, lastMove.originalPawn);

    update();
}

ChessBoardWidget::~ChessBoardWidget() {}

void ChessBoardWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    int tileSize = width() / 8;
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            // Alternate colors for squares
            painter.setBrush((row + col) % 2 == 0 ? Qt::white : Qt::gray);
            painter.drawRect(col * tileSize, row * tileSize, tileSize, tileSize);

            int square = row * 8 + col;

            // Highlight valid moves
            if (std::find(highlightedSquares.begin(), highlightedSquares.end(), square) != highlightedSquares.end())
            {
                painter.setBrush(QColor(255, 255, 0, 128)); // Yellow semi-transparent
                painter.drawRect(col * tileSize, row * tileSize, tileSize, tileSize);
            }

            // Get the piece at this square
            char piece = board->getPieceAtSquare(square);

            // Draw the piece images based on the square
            if (piece == 'P')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, whitePawnPixmap);
            }
            else if (piece == 'p')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, blackPawnPixmap);
            }
            else if (piece == 'R')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, whiteRookPixmap);
            }
            else if (piece == 'r')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, blackRookPixmap);
            }
            else if (piece == 'N')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, whiteKnightPixmap);
            }
            else if (piece == 'n')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, blackKnightPixmap);
            }
            else if (piece == 'B')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, whiteBishopPixmap);
            }
            else if (piece == 'b')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, blackBishopPixmap);
            }
            else if (piece == 'Q')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, whiteQueenPixmap);
            }
            else if (piece == 'q')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, blackQueenPixmap);
            }
            else if (piece == 'K')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, whiteKingPixmap);
            }
            else if (piece == 'k')
            {
                painter.drawPixmap(col * tileSize, row * tileSize, tileSize, tileSize, blackKingPixmap);
            }
        }
    }

    // If a piece is being dragged move it with the mouse
    if (dragging && draggedPiece)
    {
        QPoint mousePos = mapFromGlobal(QCursor::pos());
        painter.drawPixmap(mousePos.x() - dragOffset.x(), mousePos.y() - dragOffset.y(),
                           tileSize, tileSize, *draggedPiece);
    }
}

void ChessBoardWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging)
    {
        int tileSize = width() / 8;
        int row = event->position().y() / tileSize;
        int col = event->position().x() / tileSize;

        int hoveredSquare = row * 8 + col;
        if (std::find(highlightedSquares.begin(), highlightedSquares.end(), hoveredSquare) != highlightedSquares.end())
        {
            update();
        }
    }
}

void ChessBoardWidget::mousePressEvent(QMouseEvent *event)
{
    int tileSize = width() / 8;
    int row = event->position().y() / tileSize;
    int col = event->position().x() / tileSize;

    int clickedSquare = row * 8 + col;

    // Check if there's a piece on the clicked square
    char piece = board->getPieceAtSquare(clickedSquare);
    if (piece != ' ')
    { // Only start dragging if there's a piece
        draggedSquare = clickedSquare;
        dragging = true;
        uint64_t moves = 0;
        // Generate valid moves for the selected piece
        if (std::tolower(piece) == 'p')
        {
            moves = board->generatePawnMoves(clickedSquare, piece);
        }
        if (std::tolower(piece) == 'b')
        {
            moves = board->generateBishopMoves(clickedSquare, piece);
        }
        if (std::tolower(piece) == 'r')
        {
            moves = board->generateRookMoves(clickedSquare, piece);
        }
        if (std::tolower(piece) == 'q')
        {
            moves = board->generateQueenMoves(clickedSquare, piece);
        }
        if (std::tolower(piece) == 'k')
        {
            moves = board->generateKingMoves(clickedSquare, piece);
        }

        if (std::tolower(piece) == 'n')
        {
            moves = board->generateKnightMoves(clickedSquare, piece);
        }

        highlightedSquares.clear();

        // Decode the bitboard into individual squares
        for (int square = 0; square < 64; ++square)
        {
            if (moves & (1ULL << square))
            {
                highlightedSquares.push_back(square);
            }
        }

        // Determine which piece is being dragged and load the image
        if (piece == 'P')
        {
            draggedPiece = &whitePawnPixmap;
        }
        else if (piece == 'p')
        {
            draggedPiece = &blackPawnPixmap;
        }
        else if (piece == 'R')
        {
            draggedPiece = &whiteRookPixmap;
        }
        else if (piece == 'r')
        {
            draggedPiece = &blackRookPixmap;
        }
        else if (piece == 'N')
        {
            draggedPiece = &whiteKnightPixmap;
        }
        else if (piece == 'n')
        {
            draggedPiece = &blackKnightPixmap;
        }
        else if (piece == 'B')
        {
            draggedPiece = &whiteBishopPixmap;
        }
        else if (piece == 'b')
        {
            draggedPiece = &blackBishopPixmap;
        }
        else if (piece == 'Q')
        {
            draggedPiece = &whiteQueenPixmap;
        }
        else if (piece == 'q')
        {
            draggedPiece = &blackQueenPixmap;
        }
        else if (piece == 'K')
        {
            draggedPiece = &whiteKingPixmap;
        }
        else if (piece == 'k')
        {
            draggedPiece = &blackKingPixmap;
        }

        // Store the offset of the mouse click within the piece
        dragOffset = event->pos() - QPoint(col * tileSize, row * tileSize);

        // Trigger repaint to show highlights
        update();
    }
}

void ChessBoardWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (dragging)
    {
        int tileSize = width() / 8;
        int row = event->position().y() / tileSize;
        int col = event->position().x() / tileSize;

        int destinationSquare = row * 8 + col;

        // Move the piece on the board if the destination is valid
        if (board->movePiece(draggedSquare, destinationSquare))
        {
            update();
            qDebug() << "Move successful";
        }
        else
        {
            qDebug() << "Invalid move";
        }

        // Reset dragging state
        dragging = false;
        draggedSquare = -1;
        draggedPiece = nullptr;

        // Clear highlights and repaint the board
        highlightedSquares.clear();
        update();
    }
}

int ChessBoardWidget::getSquareIndex(const std::string &square)
{
    if (square.length() != 2)
        return -1;

    char file = square[0]; // 'a' to 'h'
    char rank = square[1]; // '1' to '8'

    int fileIndex = file - 'a';
    int rankIndex = (rank - '1');

    if (fileIndex < 0 || fileIndex > 7 || rankIndex < 0 || rankIndex > 7)
        return -1; // Out of bounds check

    return rankIndex * 8 + fileIndex;
}

// Converts move string like "e2e4" into from and to indices
std::pair<int, int> ChessBoardWidget::parseMove(const std::string &move)
{
    if (move.length() != 4)
        return {-1, -1};

    int from = getSquareIndex(move.substr(0, 2));
    int to = getSquareIndex(move.substr(2, 2));

    return {from, to};
}

void ChessBoardWidget::loadImages()
{
    whitePawnPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\white-pawn.png");
    if (whitePawnPixmap.isNull())
        qDebug() << "Failed to load white pawn image";

    blackPawnPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\black-pawn.png");
    if (blackPawnPixmap.isNull())
        qDebug() << "Failed to load black pawn image";

    whiteRookPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\white-rook.png");
    if (whiteRookPixmap.isNull())
        qDebug() << "Failed to load white rook image";

    blackRookPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\black-rook.png");
    if (blackRookPixmap.isNull())
        qDebug() << "Failed to load black rook image";

    whiteKnightPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\white-knight.png");
    if (whiteKnightPixmap.isNull())
        qDebug() << "Failed to load white knight image";

    blackKnightPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\black-knight.png");
    if (blackKnightPixmap.isNull())
        qDebug() << "Failed to load black knight image";

    whiteBishopPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\white-bishop.png");
    if (whiteBishopPixmap.isNull())
        qDebug() << "Failed to load white bishop image";

    blackBishopPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\black-bishop.png");
    if (blackBishopPixmap.isNull())
        qDebug() << "Failed to load black bishop image";

    whiteQueenPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\white-queen.png");
    if (whiteQueenPixmap.isNull())
        qDebug() << "Failed to load white queen image";

    blackQueenPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\black-queen.png");
    if (blackQueenPixmap.isNull())
        qDebug() << "Failed to load black queen image";

    whiteKingPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\white-king.png");
    if (whiteKingPixmap.isNull())
        qDebug() << "Failed to load white king image";

    blackKingPixmap = QPixmap("C:\\Users\\eelip\\chessEngine\\build\\resources\\images\\black-king.png");
    if (blackKingPixmap.isNull())
        qDebug() << "Failed to load black king image";
}
