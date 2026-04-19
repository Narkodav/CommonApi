#pragma once
#include "CommonApi/Namespaces.h"
#include "CommonApi/PlatformAbstractions/ErrorMapper.h"
#include "CommonApi/Utilities/StringConversions.h"
#include "CommonApi/Utilities/MeyersSingleton.h"
#include "CommonApi/Utilities/PackedRecordBuffer.h"

#include <mutex>
#include <string>
#include <array>
#include <string_view>
#include <stdexcept>
#include <format>
#include <sstream>
#include <vector>
#include <shared_mutex>
#include <chrono>
#include <queue>

namespace MultiThreading
{
	// class ConsoleException : public std::runtime_error {
	// public:
	// 	ConsoleException(Platform::Error error, const std::string& message = "")
	// 		: std::runtime_error(message.empty() ? Platform::ErrorMapper::toString(error) : message)
	// 		, m_error(error) {
	// 	}

	// 	Platform::Error getErrorCode() const { return m_error; }

	// private:
	// 	Platform::Error m_error;
	// };
	

	class Console : public Utilities::MeyersSingleton<Console>
	{
	public:
		enum class CallType
		{
			Read,
			Write,
			Error,
			Num
		};

		static inline const size_t s_defaultWriteSize = 1024;

		class Buffer {
		public:
			struct Metadata {
				std::chrono::time_point<std::chrono::steady_clock> timestamp;
			};

			using Records = Utilities::PackedRecordBuffer<Metadata, uint8_t, s_defaultWriteSize>;

			struct CompareNode {
				bool operator()(const Records::Node& left, const Records::Node& right) const {
					return left.meta.timestamp > right.meta.timestamp;
				}
			};

		private:
			Records m_records;
			size_t m_index;
			std::string m_buffer;
		public:
			
			Buffer() {
				auto& console = Console::getInstance();
				auto lock = console.lock();
				m_index = console.m_buffers.size();
				console.m_buffers.push_back(this);
			}

			~Buffer() {
				auto& console = Console::getInstance();
				auto lock = console.lock();
				console.flushInternal(CallType::Write);
				console.m_buffers.erase(console.m_buffers.begin() + m_index);
				for(size_t i = m_index; i < console.m_buffers.size(); ++i) {
					console.m_buffers[i]->m_index--;
				}
			}

			Records& getRecords() { return m_records; }
			const Records& getRecords() const { return m_records; }

			size_t& getIndex() { return m_index; }
			const size_t& getIndex() const { return m_index; }

			std::string& getBuffer() { return m_buffer; }
			const std::string& getBuffer() const { return m_buffer; }
		};


		friend class Buffer;

	private:

#ifdef _WIN32
		using HandleType = void*;
#else
		using HandleType = int;
#endif
		static const std::array<HandleType, static_cast<size_t>(CallType::Num)> s_handles;

		std::shared_mutex m_mutex;
		static inline thread_local Buffer m_threadBuffer;
		std::vector<Buffer*> m_buffers;
		std::priority_queue<Buffer::Records::Node, 
		std::vector<Buffer::Records::Node>, Buffer::CompareNode> m_queue;

		std::string m_commonBuffer;

		static inline const size_t s_defaultReadSize = 1024;

	protected:
		using Base = Utilities::MeyersSingleton<Console>;
		using Base::Base;

	private:
		template <CallType type, typename... Args>
		size_t writeBase(std::string_view format, Args... args)
		{
			auto& buffer = m_threadBuffer.getBuffer();
			buffer = std::vformat(format, std::make_format_args(args...));
			if constexpr (type == CallType::Error) {
				auto lock = this->lock();
				size_t bytesWritten = flushInternal(CallType::Write);
				Platform::Error error;
				bytesWritten += write(CallType::Error, buffer.data(), buffer.size(), error);
				return bytesWritten;
			}
			else
			{
				auto lockShared = this->lockShared();
				if(!m_threadBuffer.getRecords().canFit(buffer.size())) {
					lockShared.unlock();
					auto lock = this->lock();
					if(!m_threadBuffer.getRecords().canFit(buffer.size())) flushInternal(CallType::Write);
				}
				m_threadBuffer.getRecords().pushBack({ std::chrono::steady_clock::now() }, 
				std::span<uint8_t>(reinterpret_cast<uint8_t*>(buffer.data()), buffer.size()));
				return 0;
			}
		}

		size_t flushInternal(CallType type) {
			Platform::Error error;

			for(size_t i = 0 ; i < m_buffers.size(); ++i) {
				auto& records = m_buffers[i]->getRecords();
				for(size_t j = 0 ; j < records.size(); ++j) {
					m_queue.emplace(records[j]);
				}				
			}
			if(m_queue.empty()) return 0;

			while(!m_queue.empty()) {
				auto& chunk = m_queue.top().chunk;
				m_commonBuffer.insert(m_commonBuffer.end(), chunk.begin(), chunk.end());
				m_queue.pop();
			}
			size_t bytesWritten = write(type, m_commonBuffer.data(), m_commonBuffer.size(), error);
			m_commonBuffer.clear();

			for(size_t i = 0 ; i < m_buffers.size(); ++i) m_buffers[i]->getRecords().clear();

			if (error != Platform::Error::Ok)
				throw Platform::Exception(error);
			return bytesWritten;
		}

	public:

		~Console() { flush(); };

		template <typename... Args>
		size_t write(std::string_view format, Args... args)
		{
			return writeBase<CallType::Write>(std::string(format), std::forward<Args>(args)...);
		}

		template <typename... Args>
		size_t error(std::string_view format, Args... args)
		{
			return writeBase<CallType::Error>(std::string(format), std::forward<Args>(args)...);
		}

		template <typename... Args>
		size_t writeLine(std::string_view format, Args... args)
		{
			return writeBase<CallType::Write>(std::string(format) + "\n", std::forward<Args>(args)...);
		}

		template <typename... Args>
		size_t errorLine(std::string_view format, Args... args)
		{
			return writeBase<CallType::Error>(std::string(format) + "\n", std::forward<Args>(args)...);
		}

		template <typename... Data>
		size_t read(Data&... data)
		{
			auto lock = this->lock();
			flushInternal(CallType::Write);
			Platform::Error error;
			m_commonBuffer.resize(s_defaultReadSize);
			size_t byteRead = read(CallType::Read, m_commonBuffer.data(), s_defaultReadSize, error);

			if (error != Platform::Error::Ok)
				throw Platform::Exception(error);

			if constexpr (sizeof...(data) == 0) {
				return byteRead;
			}

			std::istringstream iss(m_commonBuffer);
			((iss >> data), ...);
			return byteRead;
		}

		size_t flush() {
			auto lock = this->lock();
			return flushInternal(CallType::Write);
		}

		// Thin platform-agnostic wrappers around OS-level read/write calls.
		// Mimic POSIX read/write semantics, but work on Windows and Linux.
		// Return number of bytes read/written and report errors via Platform::Error.
		// These bypass buffering and behave like direct ::write/WriteFile calls.
		// Safe to use but output can be interlieved in multithreaded contexts
		static size_t write(CallType callType, const void* buf, size_t count, Platform::Error& error);
		static size_t read(CallType callType, void* buf, size_t count, Platform::Error& error);

		std::unique_lock<std::shared_mutex> lock() { return std::unique_lock<std::shared_mutex>(m_mutex); }
		std::shared_lock<std::shared_mutex> lockShared() { return std::shared_lock<std::shared_mutex>(m_mutex); }
	};
}

