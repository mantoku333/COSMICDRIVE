#pragma once
#include "Transform.h"
#include "Command.h"
#include "Delegate.h"
namespace sf
{
	namespace ui
	{
		class MovablePosition
		{
		public:
			MovablePosition(Transform& _t) :t(_t) {}

			void DoMove(Vector3 _end, float _time, EASE _ease = EASE::EaseLinear);

		private:
			void UpdateMove(const command::ICommand& command);

		public:
			sf::del::VDelegate End;

		private:
			Transform& t;

			Vector3 start;
			Vector3 end;
			float time = 0;
			float limit = 0;

			EASE ease = EASE::EaseLinear;

			sf::command::Command<> c;
		};

		//UIŠî’êƒNƒ‰ƒX
		class UI :public MovablePosition
		{
		public:
			UI() :MovablePosition(transform) {}
			virtual ~UI() {}

			virtual void Activate() {}
			virtual void DeActivate() {}

			virtual void Draw() {}

			DirectX::XMMATRIX GetMatrix()const;

		public:
			Transform transform;

			float width = 100.0f;
			float height = 100.0f;

			bool enable = true;
		};
	}
}