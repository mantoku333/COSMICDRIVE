#include "Button.h"

sf::ref::Ref<sf::ui::Button> sf::ui::Button::selectingButton;

void sf::ui::Button::UpdateButton()
{
	if (!selectingButton.IsNull())
	{
		if (selectingButton.IsValid())
		{
			selectingButton.Target()->Update();
		}
		else
		{
			selectingButton = nullptr;
		}
	}
}

void sf::ui::Button::SetSelectionButton(sf::SafePtr<Button> button)
{
	if (!selectingButton.IsNull())
	{
		if (selectingButton.IsValid())
			selectingButton.Target()->OnButtonExit();
	}
	
	selectingButton = button.Get();

	if (!selectingButton.IsNull())
	{
		if (selectingButton.IsValid())
			selectingButton.Target()->OnButtonEnter();
	}
}

void sf::ui::Button::Update()
{
	if (Press())
	{
		OnButtonPress();
		return;
	}

	if (MoveUp())
	{
		//上ボタン
		sf::ref::Ref<Button>& button = Up;
		if (!button.IsNull())
		{
			if (button.IsValid())
			{
				SetSelectionButton(button.Target());
				return;
			}
			else
			{
				button = nullptr;
			}
		}
	}

	if (MoveDown())
	{
		//下ボタン
		sf::ref::Ref<Button>& button = Down;
		if (!button.IsNull())
		{
			if (button.IsValid())
			{
				SetSelectionButton(button.Target());
				return;
			}
			else
			{
				button = nullptr;
			}
		}
	}

	if (MoveLeft())
	{
		//左ボタン
		sf::ref::Ref<Button>& button = Left;
		if (!button.IsNull())
		{
			if (button.IsValid())
			{
				SetSelectionButton(button.Target());
				return;
			}
			else
			{
				button = nullptr;
			}
		}
	}

	if (MoveRight())
	{
		//右ボタン
		sf::ref::Ref<Button>& button = Right;
		if (!button.IsNull())
		{
			if (button.IsValid())
			{
				SetSelectionButton(button.Target());
				return;
			}
			else
			{
				button = nullptr;
			}
		}
	}
}

bool sf::ui::Button::MoveUp()
{
	return SInput::Instance().GetKeyDown(Key::KEY_W);
}

bool sf::ui::Button::MoveDown()
{
	return SInput::Instance().GetKeyDown(Key::KEY_S);
}

bool sf::ui::Button::MoveLeft()
{
	return SInput::Instance().GetKeyDown(Key::KEY_A);
}

bool sf::ui::Button::MoveRight()
{
	return SInput::Instance().GetKeyDown(Key::KEY_D);
}

bool sf::ui::Button::Press()
{
	return SInput::Instance().GetKeyDown(Key::SPACE);
}
