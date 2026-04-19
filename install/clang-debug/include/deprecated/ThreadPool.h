#pragma once
#include "Namespaces.h"
#include "MultiThreading/Deque.h"
#include "MultiThreading/Vector.h"

#include <thread>
#include <string>
#include <chrono>
#include <mutex>
#include <map>
#include <atomic>
#include <functional>
#include <iostream>
#include <utility>
#include <type_traits>
#include <cstdint>

namespace MultiThreading
{
	class ThreadPool
	{
	public:
		static inline const size_t s_threadPoolMaxThreads = std::thread::hardware_concurrency() * 4;

	private:
#ifndef NODEBUG
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
		std::atomic<unsigned int> m_workerCount = 0;
		std::atomic<unsigned int> m_freeWorkers = 0;
		std::atomic<unsigned int> m_activeWorkers = 0;
		std::atomic<unsigned int> m_exited = 0;
		Deque<std::function<void(size_t)>> m_tasks;
		Vector<bool> m_activeFlags;
		std::atomic<bool> m_active = 0;
		std::mutex m_taskSubmissionMutex;
		std::condition_variable_any m_poolFinished;

		void workerLoop(size_t threadIndex);

		template<typename Task>
		void pushTask(Task&& task, [[maybe_unused]] std::unique_lock<std::mutex>& accessLock)
		{
			if constexpr (std::is_invocable_v<Task>) {
#ifndef NODEBUG
				m_tasks.pushBack([task = std::forward<Task>(task), this](size_t threadIndex) {
					(void)threadIndex;
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
					});
#else
				m_tasks.pushBack([task = std::forward<Task>(task), this](size_t threadIndex) {
					(void)threadIndex;
					task();
					});
#endif
			}
			else if constexpr (std::is_invocable_v<Task, size_t>) {
#ifndef NODEBUG
				m_tasks.pushBack([task = std::forward<Task>(task), this](size_t threadIndex) {
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
					task(threadIndex);
					{
						std::lock_guard<std::mutex> lock(m_statesMutex);
						m_threadStates[threadId].currentTask = "idle";
					}
					});
#else
				m_tasks.pushBack(std::forward<Task>(task));
#endif
			}
			else {
				static_assert(
					std::is_invocable_v<Task> || std::is_invocable_v<Task, size_t>,
					"Task must be callable as either void() or void(size_t)"
					);
			}
		}

		template<typename Task>
		void pushPriorityTask(std::function<void()> task, [[maybe_unused]] std::unique_lock<std::mutex>& accessLock)
		{
			if constexpr (std::is_invocable_v<Task>) {
#ifndef NODEBUG
				m_tasks.pushFront([task = std::forward<Task>(task), this](size_t threadIndex) {
					(void)threadIndex;
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
					});
#else
				m_tasks.pushFront([task = std::forward<Task>(task), this](size_t threadIndex) {
					(void)threadIndex;
					task();
					});
#endif
			}
			else if constexpr (std::is_invocable_v<Task, size_t>) {
#ifndef NODEBUG
				m_tasks.pushFront([task = std::forward<Task>(task), this](size_t threadIndex) {
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
					task(threadIndex);
					{
						std::lock_guard<std::mutex> lock(m_statesMutex);
						m_threadStates[threadId].currentTask = "idle";
					}
					});
#else
				m_tasks.pushFront(std::forward<Task>(task));
#endif
			}
			else {
				static_assert(
					std::is_invocable_v<Task> || std::is_invocable_v<Task, size_t>,
					"Task must be callable as either void() or void(size_t)"
					);
			}
		}

	public:
		ThreadPool() = default;
		~ThreadPool() { shutdown(); };

		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		ThreadPool(ThreadPool&&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		void init(size_t numThreads);
		void shutdown();
		void terminate();

		template<typename Task>
		bool pushTask(Task&& task) {
			if (!m_active.load())
				return false;
			std::unique_lock<std::mutex> lock(m_taskSubmissionMutex);
			pushTask(std::forward<Task>(task), lock);
			return true;
		};

		// Vector of tasks with move semantics
		template<typename Task>
		bool pushTasks(std::vector<Task>&& tasks) {
			if (!m_active.load())
				return false;
			std::unique_lock<std::mutex> lock(m_taskSubmissionMutex);
			for (auto& task : tasks)
				pushTask(std::move(task), lock);
			return true;
		}

		// Const reference vector version
		template<typename Task>
		bool pushTasks(const std::vector<Task>& tasks) {
			if (!m_active.load())
				return false;
			std::unique_lock<std::mutex> lock(m_taskSubmissionMutex);
			for (const auto& task : tasks)
				pushTask(task, lock);
			return true;
		}

		// implementation doesn't work, commented out for now
		//// Iterator range version
		//template<typename Iterator>
		//bool pushTasks(Iterator begin, Iterator end) {
		//	// Use std::decay_t to get the actual value type from the iterator
		//	//using ValueType = typename std::decay_t<decltype(*std::declval<Iterator>())>;

		//	//static_assert(std::is_convertible_v<
		//	//	ValueType,
		//	//	std::function<void()>
		//	//>, "Iterator must point to compatible task type");

		//	if (!m_active.load())
		//		return false;
		//	std::unique_lock<std::mutex> lock(m_taskSubmissionMutex);
		//	for (auto it = begin; it != end; ++it) {
		//		std::function<void()> task = *it; // Will fail to compile if not convertible
		//		pushTask(std::move(task), lock);
		//	}
		//	return true;
		//}

		//// Variadic template for multiple individual tasks
		//template<typename... Tasks>
		//bool pushTasks(Tasks&&... tasks) {
		//	if (!m_active.load())
		//		return false;

		//	//static_assert((std::is_convertible_v<
		//	//	std::decay_t<Tasks>,
		//	//	std::function<void()>
		//	//> && ...), "All tasks must be convertible to std::function<void()>");

		//	std::unique_lock<std::mutex> lock(m_taskSubmissionMutex);
		//	(pushTask(std::function<void()>(std::forward<Tasks>(tasks)), lock), ...);
		//	return true;
		//}

		template<typename Task>
		bool pushPriorityTask(Task&& task) {
			if (!m_active.load())
				return false;
			std::unique_lock<std::mutex> lock(m_taskSubmissionMutex);
			pushPriorityTask(std::forward<Task>(task), lock);
			return true;
		};

		// Vector of tasks with move semantics
		template<typename Task>
		bool pushPriorityTasks(std::vector<Task>&& tasks) {
			if (!m_active.load())
				return false;
			std::unique_lock<std::mutex> lock(m_taskSubmissionMutex);
			for (auto& task : tasks)
				pushPriorityTask(std::move(task), lock);
			return true;
		}

		// Const reference vector version
		template<typename Task>
		bool pushPriorityTasks(const std::vector<Task>& tasks) {
			if (!m_active.load())
				return false;
			std::unique_lock<std::mutex> lock(m_taskSubmissionMutex);
			for (const auto& task : tasks)
				pushPriorityTask(task, lock);
			return true;
		}

		void resize(size_t newSize);

		// waits for all tasks to finish and returns a lock that keeps the pool paused until released
		std::unique_lock<std::mutex> pausePool() {
			std::unique_lock<std::mutex> lock(m_taskSubmissionMutex);

			m_poolFinished.wait(lock, [this]() {
				return m_tasks.empty() && m_freeWorkers == m_workerThreads.size();
				});

			return lock;
		}

		size_t clearPendingTasks() {
			std::lock_guard<std::mutex> lock(m_taskSubmissionMutex);
			size_t cleared = 0;
			std::function<void(size_t)> task;
			while (m_tasks.popFront(task)) {
				cleared++;
			}
			return cleared;
		}

		unsigned int getFreeWorkers() { return m_freeWorkers.load(); };
		unsigned int getWorkerAmount() {
			std::lock_guard<std::mutex> lock(m_taskSubmissionMutex);
			return m_workerThreads.size();
		};

		std::vector<std::thread::id> getWorkerIds();

#ifndef NODEBUG
		std::vector<std::string> getErrors() { return m_errors.getCopy(); };
#endif
		size_t getQueueSize() const { return m_tasks.size(); };
		size_t getWorkerCount() const { return m_workerCount; };

#ifndef NODEBUG
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

	private:
		void pushTask(std::function<void()> task, std::unique_lock<std::mutex>& accessLock);
		void pushPriorityTask(std::function<void()> task, std::unique_lock<std::mutex>& accessLock);
	};
}

