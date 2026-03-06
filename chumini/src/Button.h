#pragma once
#include "App.h"
namespace sf
{
	namespace ui
	{
		class Button :public sf::ui::UI, public sf::ref::autoRef<Button>
		{
		public:
			static void UpdateButton();

			static void SetSelectionButton(sf::SafePtr<Button> button);

		private:
			void Update();

			static bool MoveUp();
			static bool MoveDown();
			static bool MoveLeft();
			static bool MoveRight();
			static bool Press();

		public:
			sf::del::VDelegate OnButtonEnter;	//カーソルが合わさった時
			sf::del::VDelegate OnButtonExit;	//カーソルが出た時
			sf::del::VDelegate OnButtonPress;	//クリック入力された時

			sf::ref::Ref<Button> Up;
			sf::ref::Ref<Button> Down;
			sf::ref::Ref<Button> Left;
			sf::ref::Ref<Button> Right;

		private:
			static sf::ref::Ref<Button> selectingButton;
		};
	}
}