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

	public:

		EventSuperSystem() : m_eventSystems() {};

		EventSuperSystem(const EventSuperSystem&) = delete;
		EventSuperSystem& operator=(const EventSuperSystem&) = delete;

		EventSuperSystem(EventSuperSystem&& other) noexcept = default;
		EventSuperSystem& operator=(EventSuperSystem&&) noexcept = default;

		template<typename EventPolicy, EventSystem<EventPolicy>::EventEnum E>
		typename EventSystem<EventPolicy>::template Subscription subscribe
		(typename EventSystem<EventPolicy>::template Event<E>::Callback callback) {
			static_assert((std::is_same_v<EventPolicy, EventPolicies> || ...),
				"EventPolicy must be one of the policies passed to EventSuperSystem"
				);
			return std::get<EventSystem<EventPolicy>>(m_eventSystems).template subscribe<E>(std::move(callback));
		}

		template<typename EventPolicy, EventSystem<EventPolicy>::EventEnum E, typename... Args>
		void emit(Args&&... args) const {
			static_assert((std::is_same_v<EventPolicy, EventPolicies> || ...),
				"EventPolicy must be one of the policies passed to EventSuperSystem"
				);
			std::get<EventSystem<EventPolicy>>(m_eventSystems).template emit<E>(std::forward<Args>(args)...);
		}

		bool hasSubscribers() const {
			return (std::get<EventSystem<EventPolicies>>(m_eventSystems).hasSubscribers() ||...);
		}

		template<typename EventPolicy>
		bool hasSubscribers() const {
			static_assert((std::is_same_v<EventPolicy, EventPolicies> || ...),
				"EventPolicy must be one of the policies passed to EventSuperSystem"
				);
			return std::get<EventSystem<EventPolicy>>(m_eventSystems).hasSubscribers();
		}

		template<typename EventPolicy, EventSystem<EventPolicy>::EventEnum E>
		bool hasSubscribers() const {
			static_assert((std::is_same_v<EventPolicy, EventPolicies> || ...),
				"EventPolicy must be one of the policies passed to EventSuperSystem"
				);
			return std::get<EventSystem<EventPolicy>>(m_eventSystems).template hasSubscribers<E>();
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

		template<typename EventPolicy, EventSystem<EventPolicy>::EventEnum E>
		void clear() {
			static_assert((std::is_same_v<EventPolicy, EventPolicies> || ...),
				"EventPolicy must be one of the policies passed to EventSuperSystem"
				);
			std::get<EventSystem<EventPolicy>>(m_eventSystems).template clear<E>();
		}
	};
}