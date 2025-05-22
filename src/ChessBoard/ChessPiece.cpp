#include "ChessPiece.h"

ChessPiece::ChessPiece( std::string path, PieceColor color, PieceType type)
	:Image(path), type(type), color(color)
{
	SetBorder(5);
	SetBorderColor(Color::Gold);
	SetRadius(5, 5, 5, 5);
	//SetColor(Color::Blue);
	//SetAngle(20);
	SetOpacity(0);
}

ChessPiece::~ChessPiece()
{

}
