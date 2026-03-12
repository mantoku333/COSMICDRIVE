#pragma once
#include "Component.h"
#include "Singleton.h"
#include "DirectX11.h"


namespace sf
{
	//カメラコンポーネント
	class Camera :public Component
	{
	public:
		void Begin()override;

		/// <summary>
		/// ビュー変換行列
		/// </summary>
		/// <returns></returns>
		DirectX::XMMATRIX GetViewMatrix()const;

		/// <summary>
		/// プロジェクション行列
		/// </summary>
		/// <returns></returns>
		DirectX::XMMATRIX GetMatrixProj()const;

		Vector3 WorldToScreen(Vector3 p)const;

		sf::SafePtr<Camera> Instance() { return main; }

		static void SetGPU();

		void SetGPUShadow()const;

		Vector3 GetPosition() const {
			if (auto actor = actorRef.Target()) {
				return actor->transform.GetPosition();
			}
			return Vector3{};  // 安全側のデフォルト値
		}

		// カメラ（アクター）の位置を設定
		void SetPosition(const Vector3& pos) {
			if (auto actor = actorRef.Target()) {
				actor->transform.SetPosition(pos);
			}
		}
	

		// カメラが特定の位置を見るように設定するメソッド
		void LookAt(const Vector3& target);


	private:
		void Update();

	public:
		static Camera* main;

		static Camera* shadow;

		float fov = 108;

		float nearClip = 0.1f;

		float farClip = 1000.0f;

	private:
		command::Command<7> command;

		static VP vp;

		Vector3 lookAtTarget = Vector3::Zero; // 注視点
		bool hasLookAt = false;
	};
}