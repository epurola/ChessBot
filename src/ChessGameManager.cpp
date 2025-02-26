#include "ChessGameManager.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QPushButton>
#include "ChessboardWidget.h"
#include <QTimer>
#include <QThread>

ChessGameManager::ChessGameManager(std::shared_ptr<Board> board, QObject *parent)
    : QObject(parent), board(board), isWhiteTurn(true), whiteWins(0), blackWins(0)
{

    // Connect signals to read output when the engine responds
    connect(&engine1, &QProcess::readyReadStandardOutput, this, &ChessGameManager::readEngine1Output);
    connect(&engine2, &QProcess::readyReadStandardOutput, this, &ChessGameManager::readEngine2Output);

    initPositions();

    QWidget *window = new QWidget;
    window->setWindowTitle("Chess Game Manager");
    QHBoxLayout *layout = new QHBoxLayout;
    QVBoxLayout *layout2 = new QVBoxLayout;
    QPushButton *startButton = new QPushButton("Start Game");
    QPushButton *stopButton = new QPushButton("Stop Game");
    QPushButton *undoMove = new QPushButton("Undo Move");
    QPushButton *aiMove = new QPushButton("AI Move");
    QPushButton *switchPlayer = new QPushButton("toggle player");

    layout2->setSpacing(10);
    layout2->setContentsMargins(10, 10, 10, 10);

    layout->setContentsMargins(10, 10, 10, 10);

    startButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px; border-radius: 5px; font-size: 14px; } QPushButton:hover { background-color: #45a049; }");
    stopButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 10px; border-radius: 5px; font-size: 14px; } QPushButton:hover { background-color: #da190b; }");
    undoMove->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 10px; border-radius: 5px; font-size: 14px; } QPushButton:hover { background-color: #0b79d4; }");
    aiMove->setStyleSheet("QPushButton { background-color: #FF9800; color: white; padding: 10px; border-radius: 5px; font-size: 14px; } QPushButton:hover { background-color: #e68900; }");
    switchPlayer->setStyleSheet("QPushButton { background-color: #9E9E9E; color: white; padding: 10px; border-radius: 5px; font-size: 14px; } QPushButton:hover { background-color: #757575; }");

    layout2->addWidget(startButton);
    layout2->addWidget(stopButton);
    layout2->addWidget(undoMove);
    layout2->addWidget(aiMove);
    layout2->addWidget(switchPlayer);
    QWidget *container = new QWidget;
    container->setMaximumWidth(300);
    container->setMinimumHeight(600);
    container->setStyleSheet(" border-radius: 10px; padding: 10px;");
    container->setLayout(layout2);
    layout->addWidget(container);

    QVBoxLayout *boardLayout = new QVBoxLayout; // Layout for chessboard + labels
    chessBoard = new ChessBoardWidget(board);
    boardLayout->addWidget(chessBoard, 1); // Chessboard first
    chessBoard->setMinimumSize(600, 600);

    // Create labels for displaying scores
    whiteWins = new QLabel("Engine 1 Wins: 0");
    blackWins = new QLabel("Engine 2 Wins: 0");
    draw = new QLabel("Draws: 0");

    // Style the labels for better visibility
    whiteWins->setAlignment(Qt::AlignCenter);
    blackWins->setAlignment(Qt::AlignCenter);
    draw->setAlignment(Qt::AlignCenter);
    whiteWins->setStyleSheet("font-size: 16px; font-weight: bold;");
    blackWins->setStyleSheet("font-size: 16px; font-weight: bold;");
    draw->setStyleSheet("font-size: 16px; font-weight: bold;");

    // Add labels under the chessboard
    boardLayout->addWidget(whiteWins);
    boardLayout->addWidget(blackWins);
    boardLayout->addWidget(draw);

    // Add everything to the main layout
    layout->addLayout(boardLayout);
    layout->setAlignment(boardLayout, Qt::AlignTop | Qt::AlignRight);

    window->setFixedSize(1000, 700);
    window->setLayout(layout);

    connect(startButton, &QPushButton::clicked, this, &ChessGameManager::startGame);
    connect(stopButton, &QPushButton::clicked, this, &ChessGameManager::stopGame);
    connect(undoMove, &QPushButton::clicked, chessBoard, &ChessBoardWidget::undoMove);
    connect(aiMove, &QPushButton::clicked, chessBoard, &ChessBoardWidget::toggleAIMove);
    connect(switchPlayer, &QPushButton::clicked, chessBoard, &ChessBoardWidget::toggleIsWhite);

    connect(this, &ChessGameManager::boardUpdated, chessBoard, &ChessBoardWidget::updateBoard);

    window->show();

    // Start engines"C:\Users\eelip\chessEngine.exe"
    engine1.start("C:/Users/eelip/chessEngine/build/Debug/chessEngine.exe");
    engine2.start("C:/Users/eelip/chessEngine/build/Debug/chessEngine_v1.exe");

    if (!engine1.waitForStarted() || !engine2.waitForStarted())
    {
        qDebug() << "Error: Chess engines failed to start!";
    }

    //sendCommandToEngine(&engine1, "uci");
    //sendCommandToEngine(&engine2, "uci");
}

void ChessGameManager::startGame()
{
    QString output;
    sendCommandToEngine(&engine1, "ucinewgame");
    sendCommandToEngine(&engine1, "setoption 5");
    sendCommandToEngine(&engine2, "ucinewgame");
    sendCommandToEngine(&engine2, "setoption 4");
    isWhiteTurn = board->whiteToMove;
    output =  isWhiteTurn ? engine1.readLine() : engine2.readLine();
  
    // sendCommandToEngine(&engine1, "position startpos");
    sendCommandToEngine(&engine1, "go");
}

void ChessGameManager::stopGame()
{
    engine1.kill();
    engine2.kill();
}

void ChessGameManager::restartGame()
{
    board->resetBoard();
    static size_t currentPositionIndex = 0;

    // Ensure there are positions to choose from
    if (positionStrings->empty())
    {
        std::cerr << "No FEN positions available." << std::endl;
        return;
    }

    currentPositionIndex = (currentPositionIndex + 1) % positionStrings->size();
    QString output;
    board->setFen((*positionStrings)[currentPositionIndex]);


    sendCommandToEngine(&engine1, "ucinewgame");
    sendCommandToEngine(&engine1, "position " + QString::fromStdString(board->getFen()));
    sendCommandToEngine(&engine1, "setoption 4");
    sendCommandToEngine(&engine2, "ucinewgame");
    sendCommandToEngine(&engine2, "position " + QString::fromStdString(board->getFen()));
    sendCommandToEngine(&engine2, "setoption 4");

    QThread::msleep(2000);
  
    isWhiteTurn = board->whiteToMove;

    output =  isWhiteTurn ? engine1.readLine() : engine2.readLine();

    sendCommandToEngine(isWhiteTurn ? &engine1 : &engine2, "go");
    
}

void ChessGameManager::sendCommandToEngine(QProcess *engine, const QString &command)
{
    if (engine->state() == QProcess::Running)
    {
        engine->write(command.toUtf8() + "\n");
    }
}

void ChessGameManager::readEngine1Output()
{
    while (engine1.canReadLine() && !board->isThreefoldRepetition() && isWhiteTurn)
    {
        QString output = engine1.readLine().trimmed();
        if (output.startsWith("bestmove"))
        {
            processMove(output, true); // White's move
        }
    }
}

void ChessGameManager::readEngine2Output()
{
    while (engine2.canReadLine() && !board->isThreefoldRepetition() && !isWhiteTurn)
    {

        QString output = engine2.readLine().trimmed();

        if (output.startsWith("bestmove"))
        {
            processMove(output, false); // Black's move
        }
    }
}

void ChessGameManager::processMove(const QString &move, bool isWhite)
{
    std::cout << move.toStdString() << std::endl;

    // Extract the move part after "bestmove "
    std::string moveStr = move.toStdString();                     
    std::string movePart = moveStr.substr(moveStr.find(" ") + 1); // Extract after the first space

   
    auto [from, to] = board->parseMove(movePart);
    if(board->isValidMove(from,to)){
        board->movePiece(from, to);
    }else{
        std::cout << "NOT A VALID MOVE! " << from << "::" << to 
              << " Piece: " << board->getPieceAtSquare(from) << std::endl;
              std::cout << "FEN " << board->getFen() 
              << " Piece: " << board->getPieceAtSquare(from) << std::endl;

    }
    emit boardUpdated();

    checkGameOver();

    isWhiteTurn = !isWhiteTurn;
    sendCommandToEngine(isWhiteTurn ? &engine1 : &engine2, "position " + QString::fromStdString(board->getFen()));
    sendCommandToEngine(isWhiteTurn ? &engine1 : &engine2, "go");
}

void ChessGameManager::checkGameOver()
{
    if (board->gameOver(!isWhiteTurn) || board->isThreefoldRepetition())
    {
        if (board->isKingInCheck(true) && !board->isThreefoldRepetition())
        {
            blackWinCount++;
        }
        else if (board->isKingInCheck(false) && !board->isThreefoldRepetition())
        {
            whiteWinCount++;
        }
        else
        {
            drawCount++;
        }
        // Update labels correctly
        whiteWins->setText("Engine 1 Wins: " + QString::number(this->whiteWinCount));
        blackWins->setText("Engine 2 Wins: " + QString::number(this->blackWinCount));
        draw->setText("Draws: " + QString::number(this->drawCount));
        board->gameFensHistory.clear();
        
        QTimer::singleShot(2000, this, &ChessGameManager::restartGame);

        if(blackWinCount + whiteWinCount + drawCount ==21){
            stopGame();
        }
    }
}

void ChessGameManager::initPositions()
{

    positionStrings->push_back("r2q1rk1/2p1bppp/p1n1bn2/1p2p3/4P3/2P2N2/PPBN1PPP/R1BQR1K1 b - - 1 12");
    positionStrings->push_back("r1bqrbk1/2p2pp1/p1np1n1p/1p2p3/4P3/PBNP1N1P/1PPB1PP1/R2Q1RK1 w - - 3 12");
    positionStrings->push_back("3r2k1/2p2bpp/p2r4/P2PpP2/BR1q4/7P/5PP1/2R1Q1K1 b - - 2 35");
    positionStrings->push_back("4r1k1/1q1b1pb1/1n1p1npp/2pPp3/2P1P3/2BB1N1P/2QN1PP1/R5K1 w - - 3 25");
    positionStrings->push_back("2rqr1k1/4bpp1/p2p1n1p/1p6/3QP3/1b4NP/PP3PP1/R1B1R1K1 b - - 0 20");
    positionStrings->push_back("r1bq1rk1/bpp2pp1/p1np1n1p/4p3/B3P3/2PP1N1P/PP3PP1/R1BQRNK1 b - - 3 11");
    positionStrings->push_back("r2qr1k1/4bpp1/p1n2n1p/1pppp3/4P3/1PPP1NNP/1P2QPP1/R1B1R1K1 w - - 1 17");

    positionStrings->push_back("r2qkbnr/pppbpppp/2n5/1B2P3/2Pp4/8/PP1P1PPP/RNBQK1NR w KQkq - 1 5");
    positionStrings->push_back("rnb1kb1r/pp1p1ppp/4pn2/q1P5/8/5NP1/PPP1PP1P/RNBQKB1R w KQkq - 1 5");
    positionStrings->push_back("r1bqkbnr/pp2pppp/2n5/3p4/3p4/P3P2P/1PP2PP1/RNBQKBNR w KQkq - 0 5");
    positionStrings->push_back("r2q1rk1/1b3ppp/p5n1/1p1pPN2/4n3/4b2P/PPB2PP1/R2QRNK1 w - - 0 19");
    positionStrings->push_back("r1bq1rk1/5ppp/p1pb4/1p1n4/8/1BPP4/PP3PPP/RNBQR1K1 b - - 2 13");
    positionStrings->push_back("3r1bk1/5pp1/7p/p3nP2/8/1B1pB2P/P4PP1/3R2K1 w - - 0 32");
    positionStrings->push_back("r2q1rk1/1bpnbppp/p2p1n2/1p2p3/4P3/1BPP1N1P/PP1N1PP1/R1BQR1K1 w - - 5 12");
    positionStrings->push_back("5rk1/4bppp/2p5/1p6/3pq3/3P3P/1PPB1PP1/R4K2 w - - 0 24");
    positionStrings->push_back("1r1qrbk1/2p2pp1/3p1n1p/1p1P4/1n2P3/4BN1P/1P1N1PP1/R2QR1K1 w - - 1 20");
    positionStrings->push_back("5rk1/2pq2p1/2nppn1p/4p3/p3P3/2QPPN1P/1PPN2P1/5RK1 b - - 1 19");
    positionStrings->push_back("r2r2k1/4bp2/6pp/pBp1p3/P7/2P1P2P/5KP1/2RR4 w - - 0 32");
    positionStrings->push_back("3r2k1/2b2pp1/1n4bp/1p6/p3N3/2P1B2P/BP3PP1/4R1K1 b - - 0 32");
    positionStrings->push_back("r1br2k1/p1p5/1bp4p/4Npp1/4n3/N1P3B1/PP3PPP/R4RK1 w - - 0 15");
    positionStrings->push_back("6k1/4bppp/8/2P1P3/1p3B2/1B1b3P/5PP1/6K1 b - - 0 35");
}
