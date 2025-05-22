#pragma once

#include "../vendor/CFrame/CFrame/src/CFrame/UIElements/Image.h"

enum PieceType{
		PAWN,
		ROOK,
		KING,
		BISHOP,
		QUEEN,
		KNIGHT
	};
	enum PieceColor{
		BLACK,
		WHITE
	};

class ChessPiece : public CFrame::Image 
{
	
public:
	ChessPiece( std::string path, PieceColor color, PieceType type);
	~ChessPiece();

	PieceColor GetColor() {  return color; }
	PieceType GetType() {return type;}

private:
	bool isWhite;
	PieceColor color;
	PieceType type;
};