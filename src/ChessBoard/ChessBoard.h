#pragma once
#include "../vendor/CFrame/CFrame/src/CFrame/UIElements/UIElement.h"
#include "../vendor/CFrame/CFrame/src/CFrame/UIElements/Grid.h"
#include "../vendor/CFrame/CFrame/src/CFrame/CFrameEvent/CFrameEvent.h"
#include "ChessPiece.h"
#include "../Board.h"


class ChessBoard : public CFrame::Grid 
{
public:
	ChessBoard(int w, int h, std::shared_ptr<Board> board );
	~ChessBoard();

	bool MousePressEvent(CFrame::MouseButtonDownEvent& event);
	bool MouseMoveEvent(CFrame::MouseMovedEvent& event);
	bool MouseReleaseEvent(CFrame::MouseButtonReleasedEvent& event);

	void MovePiece(int from, int to);

private:
	void InitializeBoard();
	

	ChessPiece* selectedPiece = nullptr;
	int index = -1;
	int cellWidth = 0;
	int cellHeight = 0;
	std::shared_ptr<Board> board;
};