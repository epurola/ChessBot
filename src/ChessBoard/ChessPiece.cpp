#include "ChessPiece.h"

ChessPiece::ChessPiece( std::string path, PieceColor color, PieceType type)
	:Image(path), type(type), color(color)
{
	//SetColor(Color::Blue);
	//SetAngle(20);
	SetOpacity(0);
}

ChessPiece::~ChessPiece()
{

}
