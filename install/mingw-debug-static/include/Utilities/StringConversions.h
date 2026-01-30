#pragma once

#include "../Namespaces.h"

#include <string_view>
#include <string>

namespace Utilities
{
	template<typename T>
	struct StringConvert {};

	template<>
	struct StringConvert<int> {
		int operator()(std::string_view str) const {
			return std::stoi(str.data());
		}
	};

	template<>
	struct StringConvert<long> {
		long operator()(std::string_view str) const {
			return std::stol(str.data());
		}
	};

	template<>
	struct StringConvert<long long> {
		long long operator()(std::string_view str) const {
			return std::stoll(str.data());
		}
	};

	template<>
	struct StringConvert<unsigned long> {
		unsigned long operator()(std::string_view str) const {
			return std::stoul(str.data());
		}
	};

	template<>
	struct StringConvert<unsigned long long> {
		unsigned long long operator()(std::string_view str) const {
			return std::stoull(str.data());
		}
	};

	template<>
	struct StringConvert<float> {
		float operator()(std::string_view str) const {
			return std::stof(str.data());
		}
	};

	template<>
	struct StringConvert<double> {
		double operator()(std::string_view str) const {
			return std::stod(str.data());
		}
	};
}