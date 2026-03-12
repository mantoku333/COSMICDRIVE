#include "Command.h"
#include "Debug.h"
#include "Config.h" // Added for Config

void CommandThrowJob(const std::exception& hoge);

std::map<uintptr_t, sf::command::ICommand*> sf::command::ICommand::commands[COMMANDIDX_MAX];
std::mutex sf::command::ICommand::commandMapMtx;
sf::jobsystem::JobSystem sf::command::ICommand::commandJob(CommandThrowJob);

void CommandThrowJob(const std::exception& hoge)
{
	sf::debug::Debug::LogError("Commandで例外がスローされました\n" + std::string(hoge.what()));
}

sf::command::ICommand::ICommand(unsigned int order) :updateOrder(order)
{

}

sf::command::ICommand::~ICommand()
{
	UnBind();
}

void sf::command::ICommand::Bind(std::function<void(const ICommand&)> function)
{
	//sf::debug::Debug::Log("Command is binded" + std::to_string(this->GetRef()));
	fun = std::move(function);

	{
		std::lock_guard<std::mutex> lock(commandMapMtx);
		commands[GetUpdateOrder()][GetRef()] = this;
	}
}

void sf::command::ICommand::UnBind() const
{
	if (app::test::gGameConfig.enableCommandLog) {
		 sf::debug::Debug::Log("Command is unbinded:" + std::to_string(this->GetRef()));
	}
	{
		std::lock_guard<std::mutex> lock(commandMapMtx);
		std::map<uintptr_t, ICommand*>& maps = commands[GetUpdateOrder()];
		auto it = maps.find(GetRef());
		if (it != maps.end())
		{
			maps.erase(it);
		}
	}
}

void sf::command::ICommand::Call() const
{
	fun(*this);
}

void sf::command::ICommand::CallAll()
{
	for (int i = 0; i < COMMANDIDX_MAX; i++)
	{
		{
			std::lock_guard<std::mutex> lock(commandMapMtx);
			for (auto& command : commands[i]) {
				commandJob.addJob(std::bind(&ICommand::Call, command.second));
			}
		}
		commandJob.waitForAllJobs();
	}
}
