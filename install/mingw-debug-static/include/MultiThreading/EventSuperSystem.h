#pragma once
#include "../Namespaces.h"

#include "EventSystem.h"

namespace MultiThreading
{
	//a combined event system class to handle multiple event policies
	template<typename... EventPolicies>
	class EventSuperSystem
	{
	private:
		template<typename... Ts>
		struct has_duplicates;

		template<>
		struct has_duplicates<> : std::false_type {};

		template<typename T, typename... Rest>
		struct has_duplicates<T, Rest...> :
			std::bool_constant<
			(std::is_same_v<T, Rest> || ...) ||
			has_duplicates<Rest...>::value
			> {
		};

		static_assert(!has_duplicates<EventPolicies...>::value,
			"Duplicate policies are not allowed in EventSuperSystem");

		std::tuple<EventSystem<EventPolicies>...> m_eventSystems;

		template<typename EventEnum, typename EventPolicy, typename... Rest>
		constexpr auto& recursiveFind()
		{
			if constexpr (std::is_same_v<EventEnum, typename EventSystem<EventPolicy>::EventEnum>)
				return std::get<EventSystem<EventPolicy>>(m_eventSystems);
			else if constexpr (sizeof...(Rest) > 0)
				return recursiveFind<EventEnum, Rest...>();
			else
				static_assert(sizeof...(Rest) != 0, "No matching EventSystem found for EventEnum");
		}

		template<typename EventEnum>
		constexpr auto& findEventSystem() {
			return recursiveFind<EventEnum, EventPolicies...>();
		};

		template<typename EventEnum, typename EventPolicy, typename... Rest>
		constexpr const auto& recursiveFind() const
		{
			if constexpr (std::is_same_v<EventEnum, typename EventSystem<EventPolicy>::EventEnum>)
				return std::get<EventSystem<EventPolicy>>(m_eventSystems);
			else if constexpr (sizeof...(Rest) > 0)
				return recursiveFind<EventEnum, Rest...>();
			else
				static_assert(sizeof...(Rest) != 0, "No matching EventSystem found for EventEnum");
		}

		template<typename EventEnum>
		constexpr const auto& findEventSystem() const {
			return recursiveFind<EventEnum, EventPolicies...>();
		};

	public:

		EventSuperSystem() : m_eventSystems() {};

		EventSuperSystem(const EventSuperSystem&) = delete;
		EventSuperSystem& operator=(const EventSuperSystem&) = delete;

		EventSuperSystem(EventSuperSystem&&) noexcept = default;
		EventSuperSystem& operator=(EventSuperSystem&&) noexcept = default;

		template<auto E>
		auto subscribe(auto callback) {
			using EventEnum = decltype(E);
			auto& eventSystem = findEventSystem<EventEnum>();
			return eventSystem.template subscribe<E>(std::move(callback));
		}

		template<auto E, typename... Args>
		void emit(Args&&... args) const {
			using EventEnum = decltype(E);
			const auto& eventSystem = findEventSystem<EventEnum>();
			eventSystem.template emit<E>(std::forward<Args>(args)...);
		}

		bool hasSubscribers() const {
			return (std::get<EventSystem<EventPolicies>>(m_eventSystems).hasSubscribers() || ...);
		}

		template<typename EventPolicy>
		bool hasSubscribers() const {
			static_assert((std::is_same_v<EventPolicy, EventPolicies> || ...),
				"EventPolicy must be one of the policies passed to EventSuperSystem"
				);
			return std::get<EventSystem<EventPolicy>>(m_eventSystems).hasSubscribers();
		}

		template<auto E>
		bool hasSubscribers() const {
			using EventEnum = decltype(E);
			const auto& eventSystem = findEventSystem<EventEnum>();
			return eventSystem.template hasSubscribers<E>();
		}

		void clear() {
			(std::get<EventSystem<EventPolicies>>(m_eventSystems).clear(), ...);
		}

		template<typename EventPolicy>
		void clear() {
			static_assert((std::is_same_v<EventPolicy, EventPolicies> || ...),
				"EventPolicy must be one of the policies passed to EventSuperSystem"
				);
			std::get<EventSystem<EventPolicy>>(m_eventSystems).clear();
		}

		template<auto E>
		void clear() {
			using EventEnum = decltype(E);
			auto& eventSystem = findEventSystem<EventEnum>();
			eventSystem.template clear<E>();
		}
	};
}