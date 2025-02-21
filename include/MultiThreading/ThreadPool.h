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
#include <utility>

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
		std::condition_variable_any m_poolFinished;

		void workerLoop(size_t threadIndex);

	public:
		ThreadPool() = default;
		ThreadPool(int numThreads) { init(numThreads); };
		~ThreadPool();

		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		ThreadPool(ThreadPool&&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		// you can use this, but i have no idea why you would want to, potentially unsafe, wasn't tested
		// 
		//		ThreadPool(ThreadPool&& other) noexcept {
		//			auto lock = other.waitForAllAndPause();
		//#ifdef _DEBUG
		//			m_threadStates = std::exchange(other.m_threadStates, std::map<std::thread::id, ThreadInfo>());
		//			m_errors = std::exchange(other.m_errors, Vector<std::string>());
		//#endif
		//			m_workerThreads = std::exchange(other.m_workerThreads, std::vector<std::thread>());
		//			m_freeWorkers.store(other.m_freeWorkers.load());
		//			other.m_freeWorkers = 0;
		//			m_activeWorkers.store(other.m_activeWorkers.load());
		//			other.m_activeWorkers = 0;
		//			m_exited.store(other.m_exited.load());
		//			other.m_exited = 0;
		//
		//			m_tasks = std::exchange(other.m_tasks, Deque<std::function<void()>>());
		//			m_activeFlags = std::exchange(other.m_activeFlags, Vector<bool>());
		//			m_active.store(other.m_active.load());
		//			other.m_active = 0;
		//		}

		// and here you can clearly see why i commented them out, if you really need them, consider modifying the pool
		// 
		//		ThreadPool& operator=(ThreadPool&& other)
		//		{
		//			if (this != &other)
		//			{
		//				std::unique_lock<std::mutex> thisLock(m_mutex);
		//				std::unique_lock<std::mutex> otherLock(other.m_mutex);
		//
		//				m_poolFinished.wait(thisLock, [this]() {
		//					return m_tasks.size() == 0 && m_freeWorkers == m_workerThreads.size();
		//					});
		//
		//				other.m_poolFinished.wait(otherLock, [&other]() {
		//					return other.m_tasks.size() == 0 && other.m_freeWorkers == other.m_workerThreads.size();
		//					});
		//					
		//#ifdef _DEBUG
		//				m_threadStates = std::exchange(other.m_threadStates, std::map<std::thread::id, ThreadInfo>());
		//				m_errors = std::exchange(other.m_errors, Vector<std::string>());
		//#endif
		//				m_workerThreads = std::exchange(other.m_workerThreads, std::vector<std::thread>());
		//				m_freeWorkers.store(other.m_freeWorkers.load());
		//				other.m_freeWorkers = 0;
		//				m_activeWorkers.store(other.m_activeWorkers.load());
		//				other.m_activeWorkers = 0;
		//				m_exited.store(other.m_exited.load());
		//				other.m_exited = 0;
		//
		//				m_tasks = std::exchange(other.m_tasks, Deque<std::function<void()>>());
		//				m_activeFlags = std::exchange(other.m_activeFlags, Vector<bool>());
		//				m_active.store(other.m_active.load());
		//				other.m_active = 0;
		//			}
		//			return *this;
		//		}

		void init(int numThreads);
		void shutdown();
		void terminate();

		void pushTask(std::function<void()> task);

		// Vector of tasks with move semantics
		void pushTasks(std::vector<std::function<void()>>&& tasks) {
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto& task : tasks)
				pushTask(std::move(task));
		}

		// Const reference vector version
		void pushTasks(const std::vector<std::function<void()>>& tasks) {
			std::lock_guard<std::mutex> lock(m_mutex);
			for (const auto& task : tasks)
				pushTask(task);
		}

		// Iterator range version
		template<typename Iterator>
		void pushTasks(Iterator begin, Iterator end) {
			static_assert(std::is_convertible_v<
				typename std::iterator_traits<Iterator>::value_type,
				std::function<void()>
			>, "Iterator must point to compatible task type");
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto it = begin; it != end; ++it) {
				pushTask(*it);
			}
		}

		// Variadic template for multiple individual tasks
		template<typename... Tasks>
		void pushTasks(Tasks&&... tasks) {
			static_assert((std::is_convertible_v<Tasks, std::function<void()>> && ...),
				"All tasks must be convertible to std::function<void()>");
			std::lock_guard<std::mutex> lock(m_mutex);
			(pushTask(std::forward<Tasks>(tasks)), ...);
		}

		void pushPriorityTask(std::function<void()> task);

		// Vector of tasks with move semantics
		void pushPriorityTasks(std::vector<std::function<void()>>&& tasks) {
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto& task : tasks)
				pushPriorityTask(std::move(task));
		}

		// Const reference vector version
		void pushPriorityTasks(const std::vector<std::function<void()>>& tasks) {
			std::lock_guard<std::mutex> lock(m_mutex);
			for (const auto& task : tasks)
				pushPriorityTask(task);
		}

		// Iterator range version
		template<typename Iterator>
		void pushPriorityTasks(Iterator begin, Iterator end) {
			static_assert(std::is_convertible_v<
				typename std::iterator_traits<Iterator>::value_type,
				std::function<void()>
			>, "Iterator must point to compatible task type");
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto it = begin; it != end; ++it) {
				pushPriorityTask(*it);
			}
		}

		// Variadic template for multiple individual tasks
		template<typename... Tasks>
		void pushPriorityTasks(Tasks&&... tasks) {
			static_assert((std::is_convertible_v<Tasks, std::function<void()>> && ...),
				"All tasks must be convertible to std::function<void()>");
			std::lock_guard<std::mutex> lock(m_mutex);
			(pushPriorityTask(std::forward<Tasks>(tasks)), ...);
		}

		void resize(size_t newSize);

		void waitForAll() {
			std::unique_lock<std::mutex> lock(m_mutex);

			m_poolFinished.wait(lock, [this]() {
				return m_tasks.size() == 0 && m_freeWorkers == m_workerThreads.size();
				});
		}

		// returns the lock after to pause the pool
		// consider careful scoping management
		std::unique_lock<std::mutex> waitForAllAndPause() {
			std::unique_lock<std::mutex> lock(m_mutex);

			m_poolFinished.wait(lock, [this]() {
				return m_tasks.size() == 0 && m_freeWorkers == m_workerThreads.size();
				});

			return lock;
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
		unsigned int getWorkerAmount() {
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_workerThreads.size();
		};

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
