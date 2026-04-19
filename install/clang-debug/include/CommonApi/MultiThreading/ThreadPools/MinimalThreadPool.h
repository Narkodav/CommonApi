#pragma once
#include "CommonApi/Namespaces.h"

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
	class MinimalThreadPool
	{
	public:
		class Lock {
			friend class MinimalThreadPool;
		private:
			std::unique_lock<std::mutex> m_lock;
			Lock(std::mutex& mutex) noexcept : m_lock(mutex) {}

			bool validate(std::mutex& mutex) const { return &mutex == m_lock.mutex(); }
			
		public:
			Lock() noexcept = default;
			~Lock() noexcept = default;
			Lock(const Lock&) noexcept = delete;
			Lock& operator=(const Lock&) noexcept = delete;
			Lock(Lock&&) noexcept = default;
			Lock& operator=(Lock&&) noexcept = default;

			void lock() { m_lock.lock(); }
			void unlock() { m_lock.unlock(); }
			bool isLocked() const { return m_lock.owns_lock(); }

			operator std::unique_lock<std::mutex>&() { return m_lock; }
		};

	private:
		std::vector<std::thread> m_threads;
		std::deque<std::function<void(size_t)>> m_tasks;
		mutable std::mutex m_taskMutex;
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
		MinimalThreadPool() noexcept = default;
		MinimalThreadPool(const MinimalThreadPool&) = delete;
		MinimalThreadPool& operator=(const MinimalThreadPool&) = delete;
		MinimalThreadPool(MinimalThreadPool&&) = delete;
		MinimalThreadPool& operator=(MinimalThreadPool&&) = delete;

		~MinimalThreadPool() {
			destroy();
		}

		inline Lock init(size_t threadCount, std::ostream& errorStream) {
			return init(threadCount, lock(), errorStream);
		}

		Lock init(size_t threadCount, Lock&& lock, std::ostream& errorStream) {
			lock = destroy(std::move(lock));
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

		inline Lock destroy() {
			return destroy(lock());
		}

		Lock destroy(Lock&& lock) {
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
		inline Lock lock() const {
			Lock lock(m_taskMutex);
			return lock;
		}

		// Locks task submission and waits until threads finish current tasks they are performing
		inline Lock wait() {
			return wait(lock());
		}

		// Wait until all tasks in the queue are complete
		inline Lock waitIdle() {
			return waitIdle(lock());
		}

		//flushes the task queue
		inline Lock flush() {
			return flush(lock());
		}

		// Locks task submission and waits until threads finish current tasks they are performing
		inline Lock wait(Lock&& lock) {
			m_taskFinished.wait(lock, [this](){ return m_workingThreadCount == 0; });
			return lock;
		}

		// Wait until all tasks in the queue are complete
		inline Lock waitIdle(Lock&& lock) {
			m_taskFinished.wait(lock, [this](){ return m_tasks.empty() && m_workingThreadCount == 0; });
			return lock;
		}

		//flushes the task queue
		inline Lock flush(Lock&& lock) {
			m_tasks.clear();
			return lock;
		}

		template<typename Task>
		inline Lock pushTask(Task&& task) {
			return pushTask(std::forward<Task>(task), lock());
		}

		template<typename Task>
		inline Lock pushTask(Task&& task, Lock&& lock) {
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
		inline Lock pushPriorityTask(Task&& task) {
			return pushPriorityTask(std::forward<Task>(task), lock());
		}

		template<typename Task>
		inline Lock pushPriorityTask(Task&& task, Lock&& lock) {
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
		inline Lock pushTasks(const TaskContainer& tasks) {		
			return pushTasks(tasks, lock());
		}

		template<typename TaskContainer>
		inline Lock pushTasks(const TaskContainer& tasks, Lock&& lock) {
			for(size_t i = 0; i < tasks.size(); ++i) lock = pushTask(tasks[i], std::move(lock));			
			return lock;
		}

		template<typename TaskContainer>
		inline Lock pushPriorityTasks(const TaskContainer& tasks) {		
			return pushPriorityTasks(tasks, lock());
		}

		template<typename TaskContainer>
		inline Lock pushPriorityTasks(const TaskContainer& tasks, Lock&& lock) {
			for(size_t i = 0; i < tasks.size(); ++i) lock = pushPriorityTask(tasks[i], std::move(lock));			
			return lock;
		}

		inline Lock resize(size_t newSize) {
			return resize(newSize, lock());
		}

		inline Lock grow(size_t newSize) {
			return grow(newSize, lock());
		}

		inline Lock shrink(size_t newSize) {
			return shrink(newSize, lock());
		}

		inline Lock resize(size_t newSize, Lock&& lock) {
			if(newSize > m_threads.size()) return grow(newSize, std::move(lock));
			else if(newSize < m_threads.size()) return shrink(newSize, std::move(lock));
			return lock;
		}

		Lock grow(size_t newSize, Lock&& lock) {
			size_t oldSize = m_threads.size();
			m_threads.resize(newSize);
			m_threadAmountToRun = newSize;
			for(size_t i = oldSize; i < m_threads.size(); ++i) m_threads[i] = std::thread([this, i](){ threadLoop(i); });
			return lock;
		}

		Lock shrink(size_t newSize, Lock&& lock) {
			m_threadAmountToRun = newSize;
			m_threadWakeUp.notify_all();
			m_threadExited.wait(lock, [&](){ return m_activeThreadCount == newSize; });
			for(size_t i = newSize; i < m_threads.size(); ++i) m_threads[i].join();
			m_threads.resize(newSize);
			return lock;
		}

		inline size_t getActiveThreadCount() const {			
			auto lock = this->lock();
			return m_activeThreadCount;
		}

		inline size_t getWorkingThreadCount() const {			
			auto lock = this->lock();
			return m_workingThreadCount;
		}

		inline size_t getThreadCount() const {
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

