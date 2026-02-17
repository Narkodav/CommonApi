#pragma once
#include "Namespaces.h"
#include "Utilities/EventSystems/EventSystemConcept.h"

#include <functional>
#include <vector>
#include <array>
#include <tuple>
#include <utility>
#include <memory>

#include <cassert>

namespace Utilities {

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

	template<template <typename Policy> typename EventSystemT, EventSystemPolicy... Policies>
	class MultiEventSystem
	{
	private:

		static_assert(!HasDuplicates<Policies...>::value,
			"Duplicate policies are not allowed in MultiEventSystem");

		static_assert(sizeof...(Policies) > 0, "At least one policy required");

		static inline const EventSystemType s_eventSystemType =
			EventSystemT<std::tuple_element_t<0, std::tuple<Policies...>>>::s_eventSystemType;

	private:
		std::tuple<EventSystemT<Policies>...> m_eventSystems;

		template<typename T, typename Policy, typename... Rest>
		constexpr auto& recursiveFind()
		{
			if constexpr (std::is_same_v<T, typename Policy::Type>)
				return std::get<EventSystemT<Policy>>(m_eventSystems);
			else if constexpr (sizeof...(Rest) > 0)
				return recursiveFind<T, Rest...>();
			else
				static_assert(sizeof...(Rest) != 0, "No matching EventSystemT found for T");
		}

		template<typename T, typename Policy, typename... Rest>
		constexpr const auto& recursiveFind() const
		{
			if constexpr (std::is_same_v<T, typename Policy::Type>)
				return std::get<EventSystemT<Policy>>(m_eventSystems);
			else if constexpr (sizeof...(Rest) > 0)
				return recursiveFind<T, Rest...>();
			else
				static_assert(sizeof...(Rest) != 0, "No matching EventSystemT found for T");
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

		MultiEventSystem() = default;
		MultiEventSystem(const MultiEventSystem&) = delete;
		MultiEventSystem(MultiEventSystem&&) = default;
		MultiEventSystem& operator=(const MultiEventSystem&) = delete;
		MultiEventSystem& operator=(MultiEventSystem&&) = default;

		template<auto T, typename... Args>
		const MultiEventSystem& emit(Args&&... args) const {
			findEventSystem<decltype(T)>().template emit<T>(std::forward<Args>(args)...);
			return *this;
		}

		template<auto T, typename CallbackT>
		auto subscribe(CallbackT&& callback)
			requires(s_eventSystemType == EventSystemType::Default) {
			return findEventSystem<decltype(T)>().template subscribe<T>(std::forward<CallbackT>(callback));
		};

		template<auto T, typename Handler, typename CallbackT>
		auto subscribe(CallbackT&& callback, Handler& handler)
			requires(s_eventSystemType == EventSystemType::Default) {
			return findEventSystem<decltype(T)>().template subscribe<T>(std::forward<CallbackT>(callback), handler);
		};

		template<auto T, typename CallbackT>
		MultiEventSystem& subscribe(CallbackT&& callback)
			requires(s_eventSystemType == EventSystemType::Scoped) {
			findEventSystem<decltype(T)>().template subscribe<T>(std::forward<CallbackT>(callback));
			return *this;
		};

		template<auto T, typename Handler, typename CallbackT>
		MultiEventSystem& subscribe(CallbackT&& callback, Handler& handler)
			requires(s_eventSystemType == EventSystemType::Scoped) {
			findEventSystem<decltype(T)>().template subscribe<T>(std::forward<CallbackT>(callback), handler);
			return *this;
		};


		template<typename SubscriptionType>
		MultiEventSystem& unsubscribe(const SubscriptionType& id)
			requires(s_eventSystemType == EventSystemType::Default) {
			std::get<typename SubscriptionType::EventSystem>(m_eventSystems).unsubscribe(id);
			return *this;
		}

		template<auto T>
		MultiEventSystem& clear()
			requires(s_eventSystemType == EventSystemType::SingleCallback) {
			findEventSystem<decltype(T)>().template clear<T>();
			return *this;
		}

		template<auto T, typename CallbackT>
		MultiEventSystem& set(CallbackT&& callback)
			requires(s_eventSystemType == EventSystemType::SingleCallback) {
			findEventSystem<decltype(T)>().template set<T>(std::forward<CallbackT>(callback));
			return *this;
		}

		template<auto T, typename Handler, typename CallbackT>
		MultiEventSystem& set(CallbackT&& callback, Handler& handler)
			requires(s_eventSystemType == EventSystemType::SingleCallback) {
			findEventSystem<decltype(T)>().template set<T>(std::forward<CallbackT>(callback), handler);
			return *this;
		}
	};
}