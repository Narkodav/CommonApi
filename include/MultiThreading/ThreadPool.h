#pragma once
#include "../Namespaces.h"
#include "Deque.h"
#include "Vector.h"

#include <thread>
#include <string>
#include <chrono>
#include <mutex>
#include <map>
#include <atomic>
#include <functional>
#include <iostream>

namespace MultiThreading
{
	class ThreadPool
	{
	private:
#ifdef _DEBUG
		struct ThreadInfo {
			std::thread::id id;
			std::string currentTask;
			std::chrono::steady_clock::time_point lastActiveTime;
			std::mutex* waitingOn;  // Track locked mutex
		};

		std::map<std::thread::id, ThreadInfo> m_threadStates;
		std::mutex m_statesMutex;
		Vector<std::string> m_errors;
#endif

		std::vector<std::thread> m_workerThreads;
		std::atomic<unsigned int> m_freeWorkers;
		std::atomic<unsigned int> m_activeWorkers = 0;
		std::atomic<unsigned int> m_exited = 0;
		Deque<std::function<void()>> m_tasks;
		Vector<bool> m_activeFlags;
		std::atomic<bool> m_active = 0;
		std::mutex m_mutex;

		void workerLoop(size_t threadIndex);

	public:
		ThreadPool() = default;
		ThreadPool(int numThreads) { init(numThreads); };
		~ThreadPool();

		void init(int numThreads);
		void shutdown();
		void terminate();

		void pushTask(std::function<void()> task);

		// Vector of tasks with move semantics
		void pushTasks(std::vector<std::function<void()>>&& tasks) {
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto& task : tasks)
				pushTask(task);
		}

		// Const reference vector version
		void pushTasks(const std::vector<std::function<void()>>& tasks) {
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto& task : tasks)
				pushTask(task);
		}

		// Iterator range version
		template<typename Iterator>
		void pushTasks(Iterator begin, Iterator end) {
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto it = begin; it != end; ++it) {
				pushTask(*it);
			}
		}

		// Variadic template for multiple individual tasks
		template<typename... Tasks>
		void pushTasks(Tasks&&... tasks) {
			std::lock_guard<std::mutex> lock(m_mutex);
			(pushTask(std::forward<Tasks>(tasks)), ...);
		}

		void pushPriorityTask(std::function<void()> task);

		// Vector of tasks with move semantics
		void pushPriorityTasks(std::vector<std::function<void()>>&& tasks) {
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto& task : tasks)
				pushPriorityTask(task);
		}

		// Const reference vector version
		void pushPriorityTasks(const std::vector<std::function<void()>>& tasks) {
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto& task : tasks)
				pushPriorityTask(task);
		}

		// Iterator range version
		template<typename Iterator>
		void pushPriorityTasks(Iterator begin, Iterator end) {
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto it = begin; it != end; ++it) {
				pushPriorityTask(*it);
			}
		}

		// Variadic template for multiple individual tasks
		template<typename... Tasks>
		void pushPriorityTasks(Tasks&&... tasks) {
			std::lock_guard<std::mutex> lock(m_mutex);
			(pushPriorityTask(std::forward<Tasks>(tasks)), ...);
		}

		void resize(size_t newSize);

		//prevents tusk submission
		void waitForAll() {
			while (m_tasks.size() > 0 || m_freeWorkers < m_workerThreads.size()) {
				std::this_thread::yield();
			}
		}

		void waitForAllAndPause() {
			std::lock_guard<std::mutex> lock(m_mutex);
			while (m_tasks.size() > 0 || m_freeWorkers < m_workerThreads.size()) {
				std::this_thread::yield();
			}
		}

		size_t clearPendingTasks() {
			std::lock_guard<std::mutex> lock(m_mutex);
			size_t cleared = 0;
			std::function<void()> task;
			while (m_tasks.popFront(task)) {
				cleared++;
			}
			return cleared;
		}

		unsigned int getFreeWorkers() { return m_freeWorkers.load(); };
		unsigned int getWorkerAmount() { return m_workerThreads.size(); };

		std::vector<std::thread::id> getWorkerIds();

#ifdef _DEBUG
		std::vector<std::string> getErrors() { return m_errors.getCopy(); };
#endif
		size_t queueSize() const { return m_tasks.size(); };

#ifdef _DEBUG
		void logThreadState(const std::string& state, std::mutex* waitingMutex = nullptr) {
			std::lock_guard<std::mutex> lock(m_statesMutex);
			auto& info = m_threadStates[std::this_thread::get_id()];
			info.lastActiveTime = std::chrono::steady_clock::now();
			info.waitingOn = waitingMutex;
		}

		void checkForDeadlocks() {
			std::lock_guard<std::mutex> lock(m_statesMutex);
			// Check for threads waiting too long
			auto now = std::chrono::steady_clock::now();
			for (const auto& [id, info] : m_threadStates) {
				auto waitTime = now - info.lastActiveTime;
				if (waitTime > std::chrono::seconds(5) && info.currentTask != "idle" && info.currentTask != "") {  // Adjustable threshold
					std::cerr << "Potential deadlock detected:\n"
						<< "Thread " << id << " waiting for > 5s\n"
						<< "Last known task: " << info.currentTask << "\n";
				}
			}
		}
#endif
	};
}
