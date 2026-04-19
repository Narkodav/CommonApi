#pragma once
#include "Namespaces.h"

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
#include <deque>
#include <condition_variable>
#include <shared_mutex>
#include <memory>

namespace MultiThreading
{
	class ThreadPool
	{
	private:
		std::vector<std::thread> m_threads;
		std::deque<std::function<void(size_t)>> m_tasks;
		std::mutex m_taskMutex;
		std::condition_variable m_threadWakeUp;
		std::condition_variable m_taskFinished;
		std::condition_variable m_threadExited;

		std::ostream* m_errorStream = nullptr;

		size_t m_threadAmountToRun;
		size_t m_workingThreadCount;
		size_t m_activeThreadCount;

#ifndef NODEBUG
		enum class ThreadState {
			Waiting,
			Working,
			Inactive,
			CatchingError,
		};

		struct ErrorInfo {
			std::string what;
			std::chrono::steady_clock::time_point timestamp;
		};

		struct ThreadInfo {
			std::thread::id id;
			ThreadState state;
			uintptr_t waitingOn;  // Track locked mutex address
			std::vector<ErrorInfo> errors;
		};

		std::vector<ThreadInfo> m_threadStates;
		std::vector<std::unique_ptr<std::shared_mutex>> m_stateMutexes;
#endif

	public:
		ThreadPool() noexcept = default;
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		ThreadPool(ThreadPool&&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		~ThreadPool() {
			destroy();
		}

		std::unique_lock<std::mutex> init(size_t threadCount, std::ostream& errorStream = std::cerr) {
			auto lock = destroy();
			m_errorStream = &errorStream;
			m_threads.resize(threadCount);
			m_threadAmountToRun = threadCount;
			m_activeThreadCount = 0;
			m_workingThreadCount = 0;

#ifndef NODEBUG
			m_threadStates.resize(threadCount);
			m_stateMutexes.resize(threadCount);
#endif
			for(size_t i = 0; i < m_threads.size(); ++i) {
#ifndef NODEBUG
				m_threadStates[i].id = m_threads[i].get_id();
				m_threadStates[i].state = ThreadState::Inactive;
				m_stateMutexes[i] = std::make_unique<std::shared_mutex>();
#endif
				m_threads[i] = std::thread([this, i](){ threadLoop(i); });
			}
			
			return lock;
		}

		inline std::unique_lock<std::mutex> destroy() {
			return destroy(lock());
		}

		std::unique_lock<std::mutex> destroy(std::unique_lock<std::mutex>&& lock) {
			if(m_errorStream == nullptr) return std::move(lock);
			m_threadAmountToRun = 0;
			m_threadWakeUp.notify_all();
			m_threadExited.wait(lock, [this](){ return m_activeThreadCount == 0; });

			for(size_t i = 0; i < m_threads.size(); ++i) m_threads[i].join();
			m_threads.clear();
			m_errorStream = nullptr;

			return lock;
		}

		// Locks task submission
		inline std::unique_lock<std::mutex> lock() {
			std::unique_lock<std::mutex> lock(m_taskMutex);
			return lock;
		}

		// Locks task submission and waits until threads finish current tasks they are performing
		inline std::unique_lock<std::mutex> wait() {
			return wait(lock());
		}

		// Wait until all tasks in the queue are complete
		inline std::unique_lock<std::mutex> waitIdle() {
			return waitIdle(lock());
		}

		//flushes the task queue
		inline std::unique_lock<std::mutex> flush() {
			return flush(lock());
		}

		// Locks task submission and waits until threads finish current tasks they are performing
		inline std::unique_lock<std::mutex> wait(std::unique_lock<std::mutex>&& lock) {
			m_taskFinished.wait(lock, [this](){ return m_workingThreadCount == 0; });
			return lock;
		}

		// Wait until all tasks in the queue are complete
		inline std::unique_lock<std::mutex> waitIdle(std::unique_lock<std::mutex>&& lock) {
			m_taskFinished.wait(lock, [this](){ return m_tasks.empty() && m_workingThreadCount == 0; });
			return lock;
		}

		//flushes the task queue
		inline std::unique_lock<std::mutex> flush(std::unique_lock<std::mutex>&& lock) {
			m_tasks.clear();
			return lock;
		}

		template<typename Task>
		inline std::unique_lock<std::mutex> pushTask(Task&& task) {
			return pushTask(std::forward<Task>(task), lock());
		}

		template<typename Task>
		inline std::unique_lock<std::mutex> pushTask(Task&& task, std::unique_lock<std::mutex>&& lock) {
			if constexpr (std::is_invocable_v<Task>) {
				m_tasks.push_back([task = std::forward<Task>(task), this](size_t threadIndex) {
					(void)threadIndex;
					task();
					});
				m_threadWakeUp.notify_one();
			}
			else if constexpr (std::is_invocable_v<Task, size_t>) {
				m_tasks.push_back(std::forward<Task>(task));
				m_threadWakeUp.notify_one();
			}
			else {
				static_assert(
					std::is_invocable_v<Task> || std::is_invocable_v<Task, size_t>,
					"Task must be callable as either void() or void(size_t)"
					);
			}
			return lock;
		}

		template<typename Task>
		inline std::unique_lock<std::mutex> pushPriorityTask(Task&& task) {
			return pushPriorityTask(std::forward<Task>(task), lock());
		}

		template<typename Task>
		inline std::unique_lock<std::mutex> pushPriorityTask(Task&& task, std::unique_lock<std::mutex>&& lock) {
			if constexpr (std::is_invocable_v<Task>) {
				m_tasks.push_front([task = std::forward<Task>(task), this](size_t threadIndex) {
					(void)threadIndex;
					task();
					});
				m_threadWakeUp.notify_one();
			}
			else if constexpr (std::is_invocable_v<Task, size_t>) {
				m_tasks.push_front(std::forward<Task>(task));
				m_threadWakeUp.notify_one();
			}
			else {
				static_assert(
					std::is_invocable_v<Task> || std::is_invocable_v<Task, size_t>,
					"Task must be callable as either void() or void(size_t)"
					);
			}
			return lock;
		}

		template<typename TaskContainer>
		inline std::unique_lock<std::mutex> pushTasks(const TaskContainer& tasks) {		
			return pushTasks(tasks, lock());
		}

		template<typename TaskContainer>
		inline std::unique_lock<std::mutex> pushTasks(const TaskContainer& tasks, std::unique_lock<std::mutex>&& lock) {
			for(size_t i = 0; i < tasks.size(); ++i) lock = pushTask(tasks[i], std::move(lock));			
			return lock;
		}

		template<typename TaskContainer>
		inline std::unique_lock<std::mutex> pushPriorityTasks(const TaskContainer& tasks) {		
			return pushPriorityTasks(tasks, lock());
		}

		template<typename TaskContainer>
		inline std::unique_lock<std::mutex> pushPriorityTasks(const TaskContainer& tasks, std::unique_lock<std::mutex>&& lock) {
			for(size_t i = 0; i < tasks.size(); ++i) lock = pushPriorityTask(tasks[i], std::move(lock));			
			return lock;
		}

		std::unique_lock<std::mutex> resize(size_t newSize) {
			return resize(newSize, lock());
		}

		std::unique_lock<std::mutex> grow(size_t newSize) {
			return grow(newSize, lock());
		}

		std::unique_lock<std::mutex> shrink(size_t newSize) {
			return shrink(newSize, lock());
		}

		std::unique_lock<std::mutex> resize(size_t newSize, std::unique_lock<std::mutex>&& lock) {
			if(newSize > m_threads.size()) return grow(newSize, std::move(lock));
			else if(newSize < m_threads.size()) return shrink(newSize, std::move(lock));
			return lock;
		}

		std::unique_lock<std::mutex> grow(size_t newSize, std::unique_lock<std::mutex>&& lock) {
			size_t oldSize = m_threads.size();
			m_threads.resize(newSize);
			m_threadAmountToRun = newSize;
			for(size_t i = oldSize; i < m_threads.size(); ++i) m_threads[i] = std::thread([this, i](){ threadLoop(i); });
			return lock;
		}

		std::unique_lock<std::mutex> shrink(size_t newSize, std::unique_lock<std::mutex>&& lock) {
			m_threadAmountToRun = newSize;
			m_threadWakeUp.notify_all();
			m_threadExited.wait(lock, [&](){ return m_activeThreadCount == newSize; });
			for(size_t i = newSize; i < m_threads.size(); ++i) m_threads[i].join();
			m_threads.resize(newSize);
			return lock;
		}

		size_t getActiveThreadCount() {			
			auto lock = this->lock();
			return m_activeThreadCount;
		}

		size_t getWorkingThreadCount() {			
			auto lock = this->lock();
			return m_workingThreadCount;
		}

		size_t getThreadCount() {
			auto lock = this->lock();
			return m_workingThreadCount;
		}

	private:

		void threadLoop(size_t threadIndex) {
			auto lock = this->lock();
#ifndef NODEBUG
			auto& threadInfo = m_threadStates[threadIndex];
			auto& threadInfoMutex = *m_stateMutexes[threadIndex];
			threadInfoMutex.lock();
			threadInfo.waitingOn = reinterpret_cast<uintptr_t>(&m_taskMutex);
			threadInfo.state = ThreadState::Inactive;
			threadInfoMutex.unlock();
#endif
			++m_activeThreadCount;
			while(true) {
#ifndef NODEBUG
				threadInfoMutex.lock();
				threadInfo.state = ThreadState::Waiting;
				threadInfoMutex.unlock();
#endif
				m_threadWakeUp.wait(lock, [&](){ return !m_tasks.empty() || m_threadAmountToRun <= threadIndex; });
				if(m_threadAmountToRun <= threadIndex) break;
				std::function task = m_tasks.front();
				m_tasks.pop_front();
				++m_workingThreadCount;
				lock.unlock();
#ifndef NODEBUG
				threadInfoMutex.lock();
				threadInfo.state = ThreadState::Working;
				threadInfo.waitingOn = 0;
				threadInfoMutex.unlock();
#endif
				try {
					task(threadIndex);
					lock.lock();
#ifndef NODEBUG
					threadInfoMutex.lock();
					threadInfo.waitingOn = reinterpret_cast<uintptr_t>(&m_taskMutex);
					threadInfoMutex.unlock();
#endif
				} catch(const std::exception& e) {
					lock.lock();
#ifndef NODEBUG
					threadInfoMutex.lock();
					threadInfo.state = ThreadState::CatchingError;
					threadInfo.waitingOn = reinterpret_cast<uintptr_t>(&m_taskMutex);
					threadInfo.errors.push_back(ErrorInfo{.what = e.what(), .timestamp = std::chrono::steady_clock::now()});
					threadInfoMutex.unlock();
#endif
					*m_errorStream << e.what() << std::endl;
				}
				--m_workingThreadCount;
				m_taskFinished.notify_all();
			}
			--m_activeThreadCount;
#ifndef NODEBUG
			threadInfoMutex.lock();
			threadInfo.state = ThreadState::Inactive;
			threadInfoMutex.unlock();
#endif
			m_threadExited.notify_all();
		}
	};
}

