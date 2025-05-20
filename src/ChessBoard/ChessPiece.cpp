#include "ChessPiece.h"

ChessPiece::ChessPiece( std::string path)
	:Image(path)
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
