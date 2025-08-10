#include "vendor/CFrame/CFrame/src/CFrame.h"
#include "ChessBoard/ChessBoard.h"
#include "Board.h"
#include "Node.h"
#include "Evaluation.h"
#include <utility>

class SandBox : public CFrame::Application
{
private:
	std::shared_ptr<Board> board;
	bool isWhite = true;
	ChessBoard *grid = nullptr;

public:
	SandBox() : CFrame::Application(920, 680)
	{

		CFrame::Scene *main = new CFrame::Scene();

		CFrame::VBox *vbox = new CFrame::VBox();
		CFrame::HBox *hbox = new CFrame::HBox();

		CFrame::VBox *buttons = new CFrame::VBox(240, -1);
		buttons->SetPadding(10);
		buttons->SetMinWidth(240);
		buttons->SetColor(Color::Gray);
		CFrame::VBox *Info = new CFrame::VBox(-1, -1);
		buttons->SetAlignment(CFrame::AlignItems::Center, CFrame::AlignItems::Center);

		CFrame::Button *AiMove = new CFrame::Button(200, 50);
		AiMove->SetColor(Color::Blue);
		AiMove->SetBorder(3);
		AiMove->SetBorderColor(Color::DarkGray);
		AiMove->SetText("AI Move");
		AiMove->SetOnClick([this]()
						   { toggleAIMove(); });

		AiMove->SetOnHover([AiMove]()
						   { AiMove->StartAnimation<CFrame::Scale>(1.0f, 1.05f, 0.1f, AnimationEndBehavior::None); });

		AiMove->SetOnLeave([AiMove]()
						   { AiMove->StartAnimation<CFrame::Scale>(1.05f, 1.0f, 0.1f, AnimationEndBehavior::None); });
		
		CFrame::Button *togglePlayer = new CFrame::Button(200, 50);
		togglePlayer->SetColor(Color::Blue);
		togglePlayer->SetBorder(3);
		togglePlayer->SetBorderColor(Color::DarkGray);
		togglePlayer->SetText("Toggle player");
		togglePlayer->SetOnClick([this]()
						   { isWhite = !isWhite; });

		togglePlayer->SetOnHover([togglePlayer]()
						   { togglePlayer->StartAnimation<CFrame::Scale>(1.0f, 1.05f, 0.1f, AnimationEndBehavior::None); });

		togglePlayer->SetOnLeave([togglePlayer]()
						   { togglePlayer->StartAnimation<CFrame::Scale>(1.05f, 1.0f, 0.1f, AnimationEndBehavior::None); });

		vbox->AddChild(hbox);

		board = std::make_shared<Board>();
		grid = new ChessBoard(680, 680, board);

		buttons->AddChild(AiMove);
		buttons->AddChild(togglePlayer);
		buttons->SetSpacing(20);
		hbox->AddChild(buttons);
		hbox->AddChild(grid);
		hbox->AddChild(Info);

		main->AddElement(vbox);

		AddScene(main);
	}

	~SandBox()
	{
	}

	void toggleAIMove()
	{
		Node root;
		Evaluation evaluate(board);
		auto start = std::chrono::high_resolution_clock::now();
		auto [bestScore, bestmove] = root.iterativeDeepening(board, 7, isWhite, evaluate);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration = end - start;

		std::cout << "Minimax search took " << duration.count() << " seconds." << std::endl;
		std::cout << "Making move: " << bestmove.from << "::" << bestmove.to << std::endl;

		//board->movePiece(bestmove.from, bestmove.to);
		grid->MovePiece(bestmove.from, bestmove.to);
	}
};

/*Client implementasion of CreateApplication returns a new instance of users application
 * that inherits from CFrame::Application */
CFrame::Application *CFrame::CreateApplication()
{
	return new SandBox();
}