#pragma once
#include "../vendor/CFrame/CFrame/src/CFrame/UIElements/UIElement.h"
#include "../vendor/CFrame/CFrame/src/CFrame/UIElements/Grid.h"
#include "../vendor/CFrame/CFrame/src/CFrame/CFrameEvent/CFrameEvent.h"
#include "ChessPiece.h"


class ChessBoard : public CFrame::Grid 
{
public:
	ChessBoard(int w, int h);
	~ChessBoard();

	bool MousePressEvent(CFrame::MouseButtonDownEvent& event);
	bool MouseMoveEvent(CFrame::MouseMovedEvent& event);
	bool MouseReleaseEvent(CFrame::MouseButtonReleasedEvent& event);

private:
	void InitializeBoard();

	ChessPiece* selectedPiece = nullptr;
	int index = -1;
	int cellWidth = 0;
	int cellHeight = 0;
};