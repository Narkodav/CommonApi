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

	public:
		ThreadPool() noexcept = default;
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		ThreadPool(ThreadPool&&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		~ThreadPool() {
			destroy();
		}

		ThreadPool(size_t threadCount, std::ostream& errorStream = std::cerr) {
			init(threadCount, errorStream);
		}

		void init(size_t threadCount, std::ostream& errorStream = std::cerr) {
			auto lock = destroy();
			m_errorStream = &errorStream;
			m_threads.resize(threadCount);
			m_threadAmountToRun = threadCount;
			m_activeThreadCount = 0;
			m_workingThreadCount = 0;

			for(size_t i = 0; i < m_threads.size(); ++i) {
				m_threads[i] = std::thread([this, i](){ threadLoop(i); });
			}
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

		// Waits until threads finish current tasks they are performing
		inline std::unique_lock<std::mutex> waitAndLock() {
			std::unique_lock<std::mutex> lock(m_taskMutex);
			m_taskFinished.wait(lock, [this](){ return m_workingThreadCount == 0; });
			return lock;
		}

		// Wait until all tasks in the queue are complete
		inline std::unique_lock<std::mutex> waitTasksAndLock() {
			std::unique_lock<std::mutex> lock(m_taskMutex);
			m_taskFinished.wait(lock, [this](){ return m_tasks.empty() && m_workingThreadCount == 0; });
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

			for(size_t i = oldSize; i < m_threads.size(); ++i) {
				m_threads[i] = std::thread([this, i](){ threadLoop(i); });
			}

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

	private:

		void threadLoop(size_t threadIndex) {	
			auto lock = this->lock();
			++m_activeThreadCount;
			while(true) {							
				m_threadWakeUp.wait(lock, [&](){ return !m_tasks.empty() || m_threadAmountToRun <= threadIndex; });
				if(m_threadAmountToRun <= threadIndex) break;
				std::function task = m_tasks.front();
				m_tasks.pop_front();
				++m_workingThreadCount;
				lock.unlock();
				try {					
					task(threadIndex);
					lock.lock();
				} catch(std::exception e) {
					lock.lock();
					*m_errorStream << e.what() << std::endl;
				}
				--m_workingThreadCount;
				m_taskFinished.notify_all();
			}
			--m_activeThreadCount;
			m_threadExited.notify_all();
		}
	};
}

