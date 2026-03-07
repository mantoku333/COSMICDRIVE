#include "JobSystem.h"

static sf::jobsystem::JobSystem* instance = nullptr;

sf::jobsystem::JobSystem* sf::jobsystem::JobSystem::Instance()
{
    return instance;
}

void sf::jobsystem::JobSystem::Create(size_t numThreads)
{
    if (instance == nullptr)
    {
        instance = new JobSystem(numThreads);
    }
}

void sf::jobsystem::JobSystem::Destroy()
{
    if (instance != nullptr)
    {
        delete instance;
        instance = nullptr;
    }
}

sf::jobsystem::JobSystem::JobSystem(std::function<void(const std::exception& hoge)> _throwJob, size_t numThreads) :
	throwJob(_throwJob)
	, stopFlag(false)
{
	Constructor(numThreads);
}

sf::jobsystem::JobSystem::JobSystem(std::function<void(const std::exception& hoge)> _throwJob) :throwJob(_throwJob)
{
	Constructor(std::thread::hardware_concurrency());
}

sf::jobsystem::JobSystem::JobSystem(size_t numThreads)
{
	Constructor(numThreads);
}

sf::jobsystem::JobSystem::JobSystem()
{
	Constructor(std::thread::hardware_concurrency());
}

sf::jobsystem::JobSystem::~JobSystem()
{
	{
		std::unique_lock<std::mutex> lock(queueMutex);
		stopFlag = true;
	}
	condition.notify_all();
	for (std::thread& worker : workers)
		worker.join();
}

void sf::jobsystem::JobSystem::addJob(std::function<void()> job)
{
	{
		std::unique_lock<std::mutex> lock(queueMutex);
		jobQueue.push(std::move(job));
	}
	condition.notify_one();
}

void sf::jobsystem::JobSystem::waitForAllJobs()
{
	std::unique_lock<std::mutex> lock(queueMutex);
	allJobsDone.wait(lock, [this] {
		return jobQueue.empty() && activeJobs == 0;
		});
}

bool sf::jobsystem::JobSystem::EndAllJobs()
{
	{
		std::unique_lock<std::mutex> lock(queueMutex);

		if (jobQueue.empty())
		{
			if (activeJobs == 0)
			{
				return true;
			}
		}
	}
	return false;
}

// ----------------------------------------------------------------------
void sf::jobsystem::JobSystem::Constructor(size_t numThreads)
{
	// Threads loop
	for (size_t i = 0; i < numThreads; ++i) {
		workers.emplace_back([this] {
			while (true) {
				std::function<void()> job;
				{
					std::unique_lock<std::mutex> lock(queueMutex);
					condition.wait(lock, [this] {
						return stopFlag || !jobQueue.empty();
						});

					if (stopFlag && jobQueue.empty())
						return;

					job = std::move(jobQueue.front());
					jobQueue.pop();
					++activeJobs; 
				}
				try
				{
					job();
				}
				catch (const std::exception& hoge)
				{
					throwJob(hoge);
				}
				{
					std::unique_lock<std::mutex> lock(queueMutex);
					--activeJobs;
					if (jobQueue.empty() && activeJobs == 0) {
						allJobsDone.notify_all();
					}
				}
			}
			});
	}
}
