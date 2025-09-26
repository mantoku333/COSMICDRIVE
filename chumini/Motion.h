#pragma once
#include "Component.h"
#include "MotionArray.h"
#include "GeometryFbx.h"
namespace sf
{
	namespace motion
	{
		class RootMotionBone;

		//モーションデータ用モーションボーン
		class MotionBone
		{
		public:
			MotionBone(MotionBone* _parent);
			~MotionBone();

			void Create(RootMotionBone* root, const std::map<std::string, Bone>& maps, const Node* node);

			/// <summary>
			/// ボーンオフセット行列の取得
			/// </summary>
			/// <returns></returns>
			const DirectX::XMMATRIX& GetOffsetMatrix()const { return offsetMatrix; }
			const DirectX::XMMATRIX& GetBoneMatrix()const { return matrix; }

			void SetBoneMatrix(const DirectX::XMMATRIX& wMtx);

			/// <summary>
			/// このボーンのモーションIDを変更し、子ボーン全てに適応させる
			/// </summary>
			/// <param name="id"></param>
			void SetMotionIdForChild(int id);

			/// <summary>
			/// このボーンの時間を変更し、子ボーン全てに適応させる
			/// </summary>
			/// <param name="_t"></param>
			void SetTimeForChild(float _t);

			/// <summary>
			/// このボーンのモーショスピードを変更し、子ボーン全てに適応させる
			/// </summary>
			/// <param name="_speed"></param>
			void SetMotionSpeed(float _speed);

			/// <summary>
			/// モーションボーンに設定されたモーションIDの取得
			/// </summary>
			/// <returns></returns>
			int GetMotionId()const { return motionId; }

			void UpdateBoneMotion(int duration, float _motionSpeed);

			float GetTime()const { return time; }

		public:
			RootMotionBone* root = nullptr;
			MotionBone* parent = nullptr;

			std::vector<MotionBone*> children;

			int idx = 0;

			std::string boneName;

			//自分の事だけを考えた行列、ローカル行列
			DirectX::XMMATRIX animationMatrix = DirectX::XMMatrixIdentity();

			float motionSpeed = 1.0f;

			bool loop = true;

		protected:
			//計算結果となる親子関係を考慮した行列
			DirectX::XMMATRIX matrix = DirectX::XMMatrixIdentity();

			//ボーンオフセット行列
			DirectX::XMMATRIX offsetMatrix = DirectX::XMMatrixIdentity();

			int motionId = 0;

			float time = 0.0f;
		};

		//モーションデータ用モーションルートボーン
		class RootMotionBone :public MotionBone
		{
		public:
			RootMotionBone();

			void Create(const std::map<std::string, Bone>& maps,
				const Node* node);

		public:
			std::map<std::string, MotionBone*> bones;

			friend MotionBone;
		};

		//モーションコンポーネント
		class Motion :public Component
		{
		public:
			void Begin()override;

			void Activate()override;
			void DeActivate()override;

			/// <summary>
			/// モーションの設定
			/// </summary>
			void SetMotion(int id, float _t = 0.0f);

			/// <summary>
			/// 時間の変更
			/// </summary>
			/// <param name="_t"></param>
			void SetTime(float _t);

			void SetMotionSpeed(float _speed) { motionSpeed = _speed; }

			void SetGeomeyryMotion(const geometry::GeometryFbx& _geomeyry, const MotionArray& _motionArray);

			const std::vector<DirectX::XMMATRIX>& GetMotions()const { return mtx; }

			void SetGPU()const;

			bool EndMotion()const;

		public:
			//ルートモーションボーン
			RootMotionBone rootMotionBone;

			MotionBone* motionBone_Head = nullptr;
			MotionBone* motionBone_Spine = nullptr;

			MotionBone* motionBone_LeftUpLeg = nullptr;
			MotionBone* motionBone_LeftLeg = nullptr;
			MotionBone* motionBone_LeftToeBase = nullptr;
			MotionBone* motionBone_LeftShoulder = nullptr;
			MotionBone* motionBone_LeftArm = nullptr;
			MotionBone* motionBone_LeftForeArm = nullptr;
			MotionBone* motionBone_LeftHand = nullptr;

			MotionBone* motionBone_RightUpLeg = nullptr;
			MotionBone* motionBone_RightLeg = nullptr;
			MotionBone* motionBone_RightToeBase = nullptr;
			MotionBone* motionBone_RightShoulder = nullptr;
			MotionBone* motionBone_RightArm = nullptr;
			MotionBone* motionBone_RightForeArm = nullptr;
			MotionBone* motionBone_RightHand = nullptr;

			bool loop = true;

			std::vector<MotionBone*> motionBones;

		private:
			void Update();

			void UpdateBoneMatrix(MotionBone* bone, DirectX::XMMATRIX _mtx);

			void ConvertMtx(const MotionBone* bone);

			/// <summary>
			/// 階層別モーションのGUI
			/// </summary>
			void HierarchyMotionGUI(MotionBone* bone, int n);

		private:
			float motionSpeed = 1.0f;

			int motionId = 0;

			const MotionArray* motionArray = nullptr;

			const geometry::GeometryFbx* geomtryFbx = nullptr;

			std::vector<DirectX::XMMATRIX> mtx;

			command::Command<> motionCommand;

			std::vector<std::string> nodeTree;

			float time = 0.0f;

			friend MotionBone;
		};
	}
}