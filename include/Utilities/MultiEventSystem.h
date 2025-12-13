#pragma once
#include "../Namespaces.h"

#include "EventSystem.h"

namespace Utilities {
	template<template <typename Policy> typename EventSystemType, EventSystemPolicy... Policies>
	class MultiEventSystem
	{
	private:

		template<typename... Ts>
		struct HasDuplicates;

		template<>
		struct HasDuplicates<> : std::false_type {};

		template<typename T, typename... Rest>
		struct HasDuplicates<T, Rest...> :
			std::bool_constant<
			(std::is_same_v<T, Rest> || ...) ||
			HasDuplicates<Rest...>::value> {
		};

		static_assert(!HasDuplicates<Policies...>::value,
			"Duplicate policies are not allowed in MultiEventSystem");

	private:
		std::tuple<EventSystemType<Policies>...> m_eventSystems;

		template<typename T, typename Policy, typename... Rest>
		constexpr auto& recursiveFind()
		{
			if constexpr (std::is_same_v<T, typename Policy::Type>)
				return std::get<EventSystemType<Policy>>(m_eventSystems);
			else if constexpr (sizeof...(Rest) > 0)
				return recursiveFind<T, Rest...>();
			else
				static_assert(sizeof...(Rest) != 0, "No matching EventSystem found for T");
		}

		template<typename T, typename Policy, typename... Rest>
		constexpr const auto& recursiveFind() const
		{
			if constexpr (std::is_same_v<T, typename Policy::Type>)
				return std::get<EventSystemType<Policy>>(m_eventSystems);
			else if constexpr (sizeof...(Rest) > 0)
				return recursiveFind<T, Rest...>();
			else
				static_assert(sizeof...(Rest) != 0, "No matching EventSystem found for T");
		}

		template<typename T>
		constexpr auto& findEventSystem() {
			return recursiveFind<T, Policies...>();
		};

		template<typename T>
		constexpr const auto& findEventSystem() const {
			return recursiveFind<T, Policies...>();
		};

	public:

		template<auto T>
		auto subscribe(auto&& callback) {
			return findEventSystem<decltype(T)>().subscribe<T>(callback);
		};

		template<auto T, typename Handler>
		auto subscribe(auto&& callback, Handler& handler) {
			return findEventSystem<decltype(T)>().subscribe<T>(callback, handler);
		};

		template<auto T, typename... Args>
		const MultiEventSystem& emit(Args&&... args) const {
			findEventSystem<decltype(T)>().emit<T>(std::forward<Args>(args)...);
			return *this;
		}

		template<typename SubscriptionType>
		MultiEventSystem& unsubscribe(const SubscriptionType& id) requires(!SubscriptionType::EventSystemType::s_isScoped) {
			std::get<typename SubscriptionType::EventSystemType>(m_eventSystems).unsubscribe(id);
			return *this;
		}
	};
}