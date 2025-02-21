#include "../../include/MultiThreading/ThreadPool.h"


namespace MultiThreading
{
	void ThreadPool::init(int numThreads)
	{
		if (m_active.load())
			shutdown();
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!numThreads)
			return;
		m_workerThreads.reserve(numThreads);
		m_activeFlags.resize(numThreads);

		for (int i = 0; i < numThreads; i++)
		{
			m_activeFlags[i] = true;
			m_workerThreads.emplace_back(&ThreadPool::workerLoop, this, i);
		}
		m_freeWorkers = numThreads;
		m_active.store(true);
	}

	void ThreadPool::workerLoop(size_t threadIndex)
	{
#ifdef _DEBUG
		ThreadInfo info{ std::this_thread::get_id(), "initializing",
						  std::chrono::steady_clock::now(), nullptr };
		{
			std::lock_guard<std::mutex> lock(m_statesMutex);
			m_threadStates[info.id] = info;
		}
#endif
		std::function<void()> task;
		m_activeWorkers++;
		while (1)
		{
			m_freeWorkers++;
			m_poolFinished.notify_one();
			if (!m_activeFlags[threadIndex] || !m_active.load()) {
#ifdef _DEBUG
				logThreadState("exiting");
#endif
				break;  // Exit the thread
			}
#ifdef _DEBUG
			logThreadState("waiting for task");
#endif
			if (!m_tasks.waitAndPopFrontFor(task, std::chrono::milliseconds(100)))
				continue;

			m_freeWorkers--;
#ifdef _DEBUG
			logThreadState("executing task");
			try {
				task();
			}
			catch (const std::exception& e) {
				std::cerr << "Task exception in thread "
					<< std::this_thread::get_id()
					<< ": " << e.what() << "\n";
				m_errors.pushBack(e.what());
			}
			logThreadState("task completed");

#else
			task();
#endif
		}
		m_activeWorkers--;
	}

	void ThreadPool::shutdown() //safely exits
	{
		auto lock = waitForAllAndPause();
		m_active.store(0);
		if (!m_workerThreads.size())
			return;

		for (size_t i = 0; i < m_workerThreads.size(); i++) {
			m_activeFlags[i] = false;
		}

		for (auto& worker : m_workerThreads) {
			worker.join();
		}
		m_workerThreads.clear();
	}

	void ThreadPool::terminate() //terminates immediately, abandons pending tasks
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_active.store(0);
		for (size_t i = 0; i < m_workerThreads.size(); i++) {
			m_activeFlags[i] = false;
		}

		for (auto& worker : m_workerThreads) {
			worker.join();
		}
		m_workerThreads.clear();
	}

	void ThreadPool::pushTask(std::function<void()> task)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_active.load())
			return;

#ifdef _DEBUG
		auto wrappedTask = [this, task]() {
			auto threadId = std::this_thread::get_id();
			{
				std::lock_guard<std::mutex> lock(m_statesMutex);
				m_threadStates[threadId].currentTask =
					"Task started at " +
					std::to_string(
						std::chrono::system_clock::now()
						.time_since_epoch()
						.count()
					);
			}

			task();

			{
				std::lock_guard<std::mutex> lock(m_statesMutex);
				m_threadStates[threadId].currentTask = "idle";
			}
			};

		m_tasks.pushBack(std::move(wrappedTask));
#else
		m_tasks.pushBack(std::move(task));
#endif
	}

	void ThreadPool::pushPriorityTask(std::function<void()> task)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_active.load())
			return;
#ifdef _DEBUG
		auto wrappedTask = [this, task]() {
			auto threadId = std::this_thread::get_id();
			{
				std::lock_guard<std::mutex> lock(m_statesMutex);
				m_threadStates[threadId].currentTask =
					"Task started at " +
					std::to_string(
						std::chrono::system_clock::now()
						.time_since_epoch()
						.count()
					);
			}

			task();

			{
				std::lock_guard<std::mutex> lock(m_statesMutex);
				m_threadStates[threadId].currentTask = "idle";
			}
			};

		m_tasks.pushFront(std::move(wrappedTask));
#else
		m_tasks.pushFront(std::move(task));
#endif
	}

	void ThreadPool::resize(size_t newSize) {
		std::lock_guard<std::mutex> lock(m_mutex);

		if (newSize > m_workerThreads.size()) {
			for (size_t i = m_workerThreads.size(); i < newSize; ++i) {
				m_activeFlags.pushBack(true);
				m_workerThreads.emplace_back(&ThreadPool::workerLoop, this, i);
			}
		}
		else if (newSize < m_workerThreads.size()) {
			for (size_t i = newSize; i < m_workerThreads.size(); ++i) {
				m_activeFlags[i] = false;
				m_workerThreads[i].join();
			}
			m_workerThreads.resize(newSize);
		}
	}

	std::vector<std::thread::id> ThreadPool::getWorkerIds()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		std::vector<std::thread::id> ids;
		for (int i = 0; i < m_workerThreads.size(); i++)
			ids.emplace_back(m_workerThreads[i].get_id());

		return ids;
	}

	ThreadPool::~ThreadPool()
	{
		shutdown();
	}
}