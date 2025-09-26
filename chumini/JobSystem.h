#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>
#include <condition_variable>
#include <atomic>

namespace sf
{
	namespace jobsystem
	{
		class JobSystem {
		public:
			/// <summary>
			/// ジョブシステムの初期化
			/// </summary>
			/// <param name="_throwJob">ジョブ内で例外がスローされた時のコールバック関数</param>
			/// <param name="numThreads">必要スレッド数</param>
			JobSystem(std::function<void(const std::exception& hoge)> _throwJob, size_t numThreads);
			JobSystem(std::function<void(const std::exception& hoge)> _throwJob);
			JobSystem(size_t numThreads);
			JobSystem();
			~JobSystem();

			/// <summary>
			/// ジョブの追加
			/// </summary>
			/// <param name="job">ジョブ</param>
			void addJob(std::function<void()> job);

			/// <summary>
			/// 全てのジョブが完了されるのを待つ
			/// </summary>
			void waitForAllJobs();

			/// <summary>
			/// 全てのジョブが完了したか
			/// </summary>
			/// <returns></returns>
			bool EndAllJobs();

		private:
			void Constructor(size_t numThreads);

		private:
			std::function<void(const std::exception& hoge)> throwJob = [](const std::exception&) {};
			
			//ワーカースレッド
			std::vector<std::thread> workers;

			//ジョブキュー
			std::queue<std::function<void()>> jobQueue;

			//ジョブキューの保護用ミューテックス
			std::mutex queueMutex;

			//スレッド同期用
			std::condition_variable condition;

			//全ジョブ完了用
			std::condition_variable allJobsDone;

			//スレッド停止フラグ
			std::atomic<bool> stopFlag;

			//現在実行中のジョブ数
			int activeJobs = 0;
		};
	}
}