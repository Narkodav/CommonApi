#pragma once
#include "../Namespaces.h"
#include "../PlatformAbstractions/ErrorMapper.h"
#include "../Utilities/StringConversions.h"
#include "../Utilities/MeyersSingleton.h"

#include <mutex>
#include <string>
#include <array>
#include <string_view>
#include <stdexcept>
#include <format>

#ifdef _WIN32
#include <windows.h>
#else 
#include <unistd.h>
#endif

namespace MultiThreading
{
	class ConsoleException : public std::runtime_error {
	public:
		ConsoleException(Platform::Error error, const std::string& message = "")
			: std::runtime_error(message.empty() ? Platform::ErrorMapper::toString(error) : message)
			, m_error(error) {
		}

		Platform::Error getErrorCode() const { return m_error; }

	private:
		Platform::Error m_error;
	};

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

	private:

#ifdef _WIN32
		static inline const std::array<HANDLE, static_cast<size_t>(CallType::Num)> s_handles = {
		GetStdHandle(STD_INPUT_HANDLE),
		GetStdHandle(STD_OUTPUT_HANDLE),
		GetStdHandle(STD_ERROR_HANDLE),
		};
#else 
		static inline const std::array<int, static_cast<size_t>(CallType::Num)> s_handles = {
		STDIN_FILENO,
		STDOUT_FILENO,
		STDERR_FILENO,
		};
#endif

		std::mutex m_mutex; //one mutex to preserve write order
		std::string m_buff;
		static inline const size_t s_defaultReadSize = 1024;

	protected:
		using Base = Utilities::MeyersSingleton<Console>;
		using Base::Base;

	private:
		template <CallType type, typename... Args>
		size_t writeBase(std::string_view format, Args... args)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_buff = std::vformat(format, std::make_format_args(args...));

			Platform::Error error;
			size_t bytesWritten = write(type, m_buff.data(), m_buff.size(), error);

			if (error != Platform::Error::Ok)
				throw ConsoleException(error, "Failed to write to console");
			return bytesWritten;
		}

	public:

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
			std::unique_lock<std::mutex> lock(m_mutex);
			Platform::Error error;
			size_t byteRead = read(CallType::Read, m_buff.data(), s_defaultReadSize, error);

			if (error != Platform::Error::Ok)
				throw ConsoleException(error, "Failed to read from console");

			if constexpr (sizeof...(data) == 0)
			{
				return byteRead;
			}

			std::istringstream iss(m_buff);
			((iss >> data), ...);
			return byteRead;
		}

		//posix like wrappers for syscalls
		static size_t write(CallType callType, const void* buf, size_t count, Platform::Error& error);
		static size_t read(CallType callType, void* buf, size_t count, Platform::Error& error);
	};
}

