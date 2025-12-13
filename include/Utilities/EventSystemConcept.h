#pragma once
#include "../Namespaces.h"

#include <utility>

//namespace Example
//{
//	struct Policy
//	{
//		enum class Type
//		{
//			OptionA = 0,
//			OptionB,
//			Count
//		};
//
//		template <Type T>
//		struct Trait {};
//
//		template<Type T>
//		static void handleError(std::exception_ptr e)
//		{
//			std::cout << "Event " << Trait<T>::s_name << " error caught" << std::endl;
//		}
//	};
//
//	template <>
//	struct Policy::Trait<Policy::Type::OptionA> {
//		using Signature = void(int x);
//		static inline const std::string s_name = "OptionA";
//	};
//
//	template <>
//	struct Policy::Trait<Policy::Type::OptionB> {
//		using Signature = void(int x, int y);
//		static inline const std::string s_name = "OptionB";
//	};
//};

namespace Utilities {
	namespace Detail {
		template<typename Policy, size_t... Is>
		constexpr bool checkAllTraits(std::index_sequence<Is...>) {
			return (requires { typename Policy::template Trait<static_cast<typename Policy::Type>(Is)>::Signature; } && ...);
		}
	};

	template <typename Policy>
	concept EventSystemPolicy = requires
	{
		typename Policy::Type;
		requires std::is_enum_v<typename Policy::Type>;
		typename Policy::template Trait<Policy::Type::Count>;
	} && Detail::checkAllTraits<Policy>(std::make_index_sequence<static_cast<size_t>(Policy::Type::Count)>{});
}