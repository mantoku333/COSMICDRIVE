#pragma once
#include <map>
#include <vector>
#include <mutex>
#include "Actor.h"
#include "Ref.h"
#include "Debug.h"

/*
〇コンポーネントの説明
・オーバーライドできるイベント関数は、「Begin」「Activate」「DeActivate」の3種類あります。
	・「Begin」:コンポーネント初期化されたタイミングで呼ばれます。
				必ずAddComponentと同じスレッドで呼び出されます。

	・「Activate」
		・SceneのInit関数内でコンポーネントが生成された場合
					この関数はSceneのActivate時に呼ばれます。
					この時、SceneのActivateと同じスレッドで呼び出されます。

		・動的にコンポーネントを追加した場合
					この関数はBeginの直後に呼ばれます。
					この時、AddComponentと同じスレッドで呼び出されます。

	・「DeActivate」:アクターが削除された時に呼び出されます。
					 全コマンドが実行された後に、必ずメインスレッドから呼び出されます。


簡単に説明すると、
	Begin・・・UnityのAwake関数
	Activate・・・UnityのStart関数
	DeActivate・・・UnityのOnDestroy関数

	ただし、基本的にBeginを使用

・Update関数について
	・ComponentにはUpdate関数がありません。Commandというシステムを使用します。

		Commandをメンバ変数として定義
		↓
		Bind関数で毎フレーム処理する関数を登録
		↓
		これで毎フレーム関数を呼び出せます
		Commandは同じウェイトの場合、マルチスレッドで同時に呼び出されます
		設定できるウェイトは0～7です。(デフォルトは1)
*/


namespace sf
{
	//コンポーネント基底クラス
	class Component
	{
	public:
		virtual ~Component() {}

		/// <summary>
		/// (イベント関数)読み込み時に呼び出される
		/// </summary>
		virtual void Begin() {}

		/// <summary>
		/// (イベント関数)コンポーネントがアクティベート状態になる時に呼ばれる
		/// </summary>
		virtual void Activate(){}

		/// <summary>
		/// (イベント関数)コンポーネントがディアクティベート状態になる時に呼ばれる
		/// </summary>
		virtual void DeActivate(){}

		/// <summary>
		/// コンポーネントが付随するアクターが存在しているかどうか
		/// </summary>
		/// <returns></returns>
		bool IsValid()const;

		/// <summary>
		/// アクターのセット
		/// </summary>
		/// <param name="other"></param>
		void Set(const Actor* other);

		/// <summary>
		/// 正面方向ベクトル
		/// </summary>
		/// <param name="x"></param>
		/// <param name="y"></param>
		/// <param name="z"></param>
		/// <returns></returns>
		DirectX::XMVECTOR WorldVector(float x, float y, float z) const;

		/// <summary>
		/// Target方向を向く時の角度
		/// </summary>
		/// <param name="target"></param>
		/// <returns></returns>
		Vector3 LookAt(Vector3 target)const;

	public:
		//このコンポーネントが付随するアクター参照
		ref::Ref<Actor> actorRef;
	};
}