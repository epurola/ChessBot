#include "ChessBoard.h"

ChessBoard::ChessBoard(int w, int h, std::shared_ptr<Board> board)
	: Grid(w, h), board(board)
{
	InitializeBoard();
}

ChessBoard::~ChessBoard()
{
}

void ChessBoard::MovePiece(int from, int to)
{
	auto selectedPiece = static_cast<ChessPiece *>(GetChild(from));
	auto toPiece = static_cast<ChessPiece *>(GetChild(to));
	if (selectedPiece)
	{
		if(toPiece)
		{
			RemoveChild(to);
		}
		RemoveChild(from);
		InsertChild(selectedPiece, to);
		board->movePiece(from, to);
	}
	UpdateChildSizes();
}

bool ChessBoard::MousePressEvent(CFrame::MouseButtonDownEvent &event)
{
	selectedPiece = nullptr;
	float x = event.GetX();
	float y = event.GetY();

	int localX = x - GetX();
	int localY = y - GetY();

	cellWidth = GetWidth() / 8;
	cellHeight = GetHeight() / 8;

	int col = localX / cellWidth;
	int row = localY / cellHeight;
	index = row * 8 + col;

	selectedPiece = static_cast<ChessPiece *>(GetChild(index));

	return true;
}

bool ChessBoard::MouseMoveEvent(CFrame::MouseMovedEvent &event)
{
	if (selectedPiece)
	{
		int x = event.GetX();
		int y = event.GetY();

		selectedPiece->SetY(y - (cellWidth / 2));
		selectedPiece->SetX(x - (cellHeight / 2));
		selectedPiece->UpdateVertices();
		return true;
	}
	return false;
}

bool ChessBoard::MouseReleaseEvent(CFrame::MouseButtonReleasedEvent &event)
{
	if (selectedPiece)
	{
		RemoveChild(index); // Won't delete the child
		int x = event.GetX();
		int y = event.GetY();

		int localX = x - GetX();
		int localY = y - GetY();

		int cellWidth = GetWidth() / 8;
		int cellHeight = GetHeight() / 8;

		int col = localX / cellWidth;
		int row = localY / cellHeight;
		int toIndex = row * 8 + col;
		// This can be used to place an child to a different cell after it was removed.
		auto toPiece = static_cast<ChessPiece *>(GetChild(toIndex));

		if (toPiece == nullptr)
		{
			InsertChild(selectedPiece, toIndex); // Do not use AddChild() since it will also add to the child list
			board->movePiece(index, toIndex);
		}
		else if(toPiece->GetColor() == selectedPiece->GetColor())
		{
			InsertChild(selectedPiece, index); // Put it back if the cell is already in use
		}
		else
		{
      		RemoveChild(toIndex);
			InsertChild(selectedPiece, toIndex);
			board->movePiece(index, toIndex);
		}

		UpdateChildSizes();
		selectedPiece = nullptr;
		return true;
	}
	return false;
}

void ChessBoard::InitializeBoard()
{
	ChessPiece *b1 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-rook.png", PieceColor::BLACK, PieceType::ROOK);
	ChessPiece *b2 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-knight.png", PieceColor::BLACK, PieceType::KNIGHT);
	ChessPiece *b3 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-bishop.png", PieceColor::BLACK, PieceType::BISHOP);
	ChessPiece *b4 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-queen.png", PieceColor::BLACK, PieceType::QUEEN);
	ChessPiece *b5 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-king.png", PieceColor::BLACK, PieceType::KING);
	ChessPiece *b6 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-bishop.png", PieceColor::BLACK, PieceType::BISHOP);
	ChessPiece *b7 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-knight.png", PieceColor::BLACK, PieceType::KNIGHT);
	ChessPiece *b8 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-rook.png", PieceColor::BLACK, PieceType::ROOK);
	ChessPiece *b9 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-pawn.png", PieceColor::BLACK, PieceType::PAWN);

	ChessPiece *b10 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-pawn.png", PieceColor::BLACK, PieceType::PAWN);
	ChessPiece *b11 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-pawn.png", PieceColor::BLACK, PieceType::PAWN);
	ChessPiece *b12 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-pawn.png", PieceColor::BLACK, PieceType::PAWN);
	ChessPiece *b13 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-pawn.png", PieceColor::BLACK, PieceType::PAWN);
	ChessPiece *b14 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-pawn.png", PieceColor::BLACK, PieceType::PAWN);
	ChessPiece *b15 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-pawn.png", PieceColor::BLACK, PieceType::PAWN);
	ChessPiece *b16 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/black-pawn.png", PieceColor::BLACK, PieceType::PAWN);

	ChessPiece *w1 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-rook.png", PieceColor::WHITE, PieceType::ROOK);
	ChessPiece *w2 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-knight.png", PieceColor::WHITE, PieceType::KNIGHT);
	ChessPiece *w3 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-bishop.png", PieceColor::WHITE, PieceType::BISHOP);
	ChessPiece *w4 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-queen.png", PieceColor::WHITE, PieceType::QUEEN);
	ChessPiece *w5 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-king.png", PieceColor::WHITE, PieceType::KING);
	ChessPiece *w6 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-bishop.png", PieceColor::WHITE, PieceType::BISHOP);
	ChessPiece *w7 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-knight.png", PieceColor::WHITE, PieceType::KNIGHT);
	ChessPiece *w8 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-rook.png", PieceColor::WHITE, PieceType::ROOK);

	ChessPiece *w9 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-pawn.png", PieceColor::WHITE, PieceType::PAWN);
	ChessPiece *w10 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-pawn.png", PieceColor::WHITE, PieceType::PAWN);
	ChessPiece *w11 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-pawn.png", PieceColor::WHITE, PieceType::PAWN);
	ChessPiece *w12 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-pawn.png", PieceColor::WHITE, PieceType::PAWN);
	ChessPiece *w13 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-pawn.png", PieceColor::WHITE, PieceType::PAWN);
	ChessPiece *w14 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-pawn.png", PieceColor::WHITE, PieceType::PAWN);
	ChessPiece *w15 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-pawn.png", PieceColor::WHITE, PieceType::PAWN);
	ChessPiece *w16 = new ChessPiece("C:/dev/CFrame/SandBox/src/Images/white-pawn.png", PieceColor::WHITE, PieceType::PAWN);

	SetLayout(8, 8);
	SetBackgroundImage("C:/dev/CFrame/SandBox/src/Images/board.png");
	SetColor(Color::Blue);
	AddChild(b1, 0);
	AddChild(b2, 1);
	AddChild(b3, 2);
	AddChild(b4, 3);
	AddChild(b5, 4);
	AddChild(b6, 5);
	AddChild(b7, 6);
	AddChild(b8, 7);
	AddChild(b9, 8);

	AddChild(b10, 9);
	AddChild(b11, 10);
	AddChild(b12, 11);
	AddChild(b13, 12);
	AddChild(b14, 13);
	AddChild(b15, 14);
	AddChild(b16, 15);

	AddChild(w1, 63);
	AddChild(w2, 62);
	AddChild(w3, 61);
	AddChild(w4, 59);
	AddChild(w5, 60);
	AddChild(w6, 58);
	AddChild(w7, 57);
	AddChild(w8, 56);
	AddChild(w9, 55);

	AddChild(w10, 54);
	AddChild(w11, 53);
	AddChild(w12, 52);
	AddChild(w13, 51);
	AddChild(w14, 50);
	AddChild(w15, 49);
	AddChild(w16, 48);
}
