#include "../../include/MultiThreading/ThreadPool.h"

namespace MultiThreading
{
	const unsigned int ThreadPool::THREAD_POOL_MAX_THREADS = std::thread::hardware_concurrency() * 4;

	void ThreadPool::init(int numThreads)
	{
		if (m_active.load())
			shutdown();

		std::lock_guard<std::mutex> lock(m_taskSubmissionMutex);

		if (!numThreads)
			return;

		m_workerThreads.reserve(numThreads);
		m_activeFlags.resize(numThreads);
		m_workerCount = numThreads;

		for (int i = 0; i < numThreads; i++)
		{
			m_activeFlags[i] = true;
			m_workerThreads.emplace_back(&ThreadPool::workerLoop, this, i);
		}

		// Wait for all threads to reach their main loop
		while (m_activeWorkers < numThreads || m_freeWorkers < numThreads) {
			std::this_thread::yield();
		}

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
		std::function<void(size_t)> task;
		m_activeWorkers++;
		m_freeWorkers++;

		while (true)
		{
			m_poolFinished.notify_one();
			if (!m_activeFlags[threadIndex]) {
#ifdef _DEBUG
				logThreadState("exiting");
#endif
				break;  // Exit the thread
			}
#ifdef _DEBUG
			logThreadState("waiting for task");
#endif
			if (m_tasks.waitAndPopFrontFor(task, std::chrono::milliseconds(100)))
			{
				m_freeWorkers--;
#ifdef _DEBUG
				logThreadState("executing task");
				try {
					task(threadIndex);
				}
				catch (const std::exception& e) {
					std::cerr << "Task exception in thread "
						<< std::this_thread::get_id()
						<< ": " << e.what() << "\n";
					m_errors.pushBack(e.what());
				}
				logThreadState("task completed");

#else
				task(threadIndex);
#endif
				m_freeWorkers++;
			}
		}
		m_freeWorkers--;
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
		std::lock_guard<std::mutex> lock(m_taskSubmissionMutex);
		m_active.store(0);
		for (size_t i = 0; i < m_workerThreads.size(); i++) {
			m_activeFlags[i] = false;
		}

		for (auto& worker : m_workerThreads) {
			worker.join();
		}
		m_workerThreads.clear();
	}

	/**
	* @brief Resizes the thread pool to the specified number of threads
	* @param numThreads New number of threads
	* @note Resizing to 0 threads will effectively shutdown the pool
	*       Once shutdown, the pool will reject new tasks until resized to >0 threads
	*/
	void ThreadPool::resize(size_t newSize) {

		if (newSize == 0)
		{
			shutdown();
			return;
		}

		if (newSize > THREAD_POOL_MAX_THREADS)
			newSize = THREAD_POOL_MAX_THREADS;

		if (!m_active.load())
		{
			init(newSize);
			return;
		}
		else if (newSize == m_workerThreads.size())
			return;

		if (newSize > m_workerThreads.size()) {
			for (size_t i = m_workerThreads.size(); i < newSize; ++i) {
				m_activeFlags.pushBack(true);
				m_workerThreads.emplace_back(&ThreadPool::workerLoop, this, i);
			}
			while (m_activeWorkers < newSize) {
				std::this_thread::yield();
			}
		}
		else if (newSize < m_workerThreads.size()) {
			for (size_t i = newSize; i < m_workerThreads.size(); ++i) {
				m_activeFlags[i] = false;
				m_workerThreads[i].join();
			}
			m_activeFlags.resize(newSize);
			m_workerThreads.resize(newSize);
		}
		m_workerCount = newSize;
	}

	std::vector<std::thread::id> ThreadPool::getWorkerIds()
	{
		std::lock_guard<std::mutex> lock(m_taskSubmissionMutex);
		std::vector<std::thread::id> ids;
		for (int i = 0; i < m_workerCount; i++)
			ids.emplace_back(m_workerThreads[i].get_id());

		return ids;
	}
}