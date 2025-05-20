#include "vendor/CFrame/CFrame/src/CFrame.h"
#include "ChessBoard/ChessBoard.h"

class SandBox : public CFrame::Application 
{
public:
	SandBox() : CFrame::Application(2000, 1500)
	{

		CFrame::Scene* main = new CFrame::Scene();

		CFrame::VBox* vbox = new CFrame::VBox();
		CFrame::HBox* hbox = new CFrame::HBox();

		CFrame::VBox* buttons = new CFrame::VBox();

		vbox->AddChild(hbox);

		ChessBoard* grid = new ChessBoard(1300, 1300);

		hbox->AddChild(buttons);
		hbox->AddChild(grid);
		
		
		main->AddElement(vbox);

		AddScene(main);
	}

	~SandBox()
	{

	}
};



/*Client implementasion of CreateApplication returns a new instance of users application
* that inherits from CFrame::Application */
CFrame::Application* CFrame::CreateApplication()
{
	return new SandBox();
}