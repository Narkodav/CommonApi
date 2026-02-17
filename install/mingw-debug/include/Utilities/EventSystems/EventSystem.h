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

	template<EventSystemPolicy Policy>
	class EventSystem
	{
	public:
		template <Policy::Type T>
		using Trait = typename Policy::template Trait<T>;

		template<Policy::Type T>
		using Signature = Trait<T>::Signature;

		template<Policy::Type T>
		using Subscriber = std::function<Signature<T>>;

		using SubscriptionId = uint64_t;

		template<Policy::Type T>
		using SubscriptionInternal = std::pair<SubscriptionId, Subscriber<T>>;

		template<Policy::Type T>
		class Subscription
		{
			friend class EventSystem;
		public:
			using EventSystem = EventSystem<Policy>;

		private:
			SubscriptionId m_id;

			Subscription(SubscriptionId id)
				: m_id(id)
			{
			}
		public:

			Subscription() = default;
			Subscription(const Subscription&) = delete;
			Subscription(Subscription&&) = default;
			Subscription& operator=(const Subscription&) = delete;
			Subscription& operator=(Subscription&&) = default;

			operator SubscriptionId() const
			{
				return m_id;
			}
		};

		static inline const EventSystemType s_eventSystemType = EventSystemType::Default;

	private:

		template<typename P = Policy, P::Type T = static_cast<P::Type>(0)>
		static constexpr bool hasErrorHandler() {
			return requires { P::template handleError<T>(std::declval<std::exception_ptr>()); };
		}

		template<size_t... Is>
		static constexpr auto makeContainer(std::index_sequence<Is...>) {
			return std::tuple<std::vector<SubscriptionInternal<static_cast<Policy::Type>(Is)>>...>{};
		}

		using Subscribers = decltype(makeContainer(std::make_index_sequence<static_cast<size_t>(Policy::Type::Count)>{}));

		Subscribers m_subscribers;
		SubscriptionId m_nextId = 0;

		SubscriptionId getId() {
			assert(m_nextId < std::numeric_limits<SubscriptionId>::max() && "ID wraparound - system has been running for 584 years");
			return m_nextId++;
		}

	public:

		EventSystem() = default;
		~EventSystem() = default;

		EventSystem(EventSystem&&) = default;
		EventSystem& operator=(EventSystem&&) = default;

		EventSystem(const EventSystem&) = delete;
		EventSystem& operator=(const EventSystem&) = delete;

		template<Policy::Type T, typename CallbackT>
		Subscription<T> subscribe(CallbackT&& callback)
		{
			auto id = getId();
			SubscriptionInternal<T> internal = { id, std::forward<CallbackT>(callback) };
			std::get<static_cast<size_t>(T)>(m_subscribers).push_back(internal);
			return id;
		}

		template<Policy::Type T, typename Handler, typename CallbackT>
		Subscription<T> subscribe(CallbackT&& callback, Handler& handler)
		{
			Subscriber<T> wrappedSubscriber = [&handler, callbackInt = std::forward<CallbackT>(callback)](auto&&... args) {
				(handler.*callbackInt)(std::forward<decltype(args)>(args)...);
				};
			auto id = getId();
			SubscriptionInternal<T> internal = { id, wrappedSubscriber };
			std::get<static_cast<size_t>(T)>(m_subscribers).push_back(internal);
			return id;
		}

		template<Policy::Type T, typename... Args>
		const EventSystem& emit(Args&&... args) const
		{
			const auto& subscribers = std::get<static_cast<size_t>(T)>(m_subscribers);
			for (const auto& subscriber : subscribers)
			{
				try
				{
					subscriber.second(std::forward<Args>(args)...);
				}
				catch (...) {
					if constexpr (hasErrorHandler<Policy, T>()) {
						Policy::template handleError<T>(std::current_exception());
					}
				}
			}
			return *this;
		}

		template<Policy::Type T>
		EventSystem& unsubscribe(const Subscription<T>& id) {
			auto& subscribers = std::get<static_cast<size_t>(T)>(m_subscribers);
			subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(), [&id](auto& subscriber) {
				return subscriber.first == id;
				}), subscribers.end());
			return *this;
		}
	};
}