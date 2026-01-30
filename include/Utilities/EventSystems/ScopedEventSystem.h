#pragma once
#include "../../Namespaces.h"

#include <functional>
#include <vector>
#include <array>
#include <tuple>
#include <utility>

#include "EventSystemConcept.h"

namespace Utilities {

	template<EventSystemPolicy Policy>
	class ScopedEventSystem
	{
	public:
		template <Policy::Type T>
		using Trait = typename Policy::template Trait<T>;

		template<Policy::Type T>
		using Signature = Trait<T>::Signature;

		template<Policy::Type T>
		using Subscriber = std::function<Signature<T>>;

		static inline const EventSystemType s_eventSystemType = EventSystemType::Scoped;

	private:

		template<typename P = Policy, P::Type T = static_cast<P::Type>(0)>
		static constexpr bool hasErrorHandler() {
			return requires { P::template handleError<T>(std::declval<std::exception_ptr>()); };
		}

		template<size_t... Is>
		static constexpr auto makeContainer(std::index_sequence<Is...>) {
			return std::tuple<std::vector<Subscriber<static_cast<Policy::Type>(Is)>>...>{};
		}

		using Subscribers = decltype(makeContainer(std::make_index_sequence<static_cast<size_t>(Policy::Type::Count)>{}));

		Subscribers m_subscribers;

	public:

		ScopedEventSystem() = default;
		~ScopedEventSystem() = default;

		ScopedEventSystem(ScopedEventSystem&&) = default;
		ScopedEventSystem& operator=(ScopedEventSystem&&) = default;

		ScopedEventSystem(const ScopedEventSystem&) = delete;
		ScopedEventSystem& operator=(const ScopedEventSystem&) = delete;

		template<Policy::Type T>
		ScopedEventSystem& subscribe(Subscriber<T> callback)
		{
			std::get<static_cast<size_t>(T)>(m_subscribers).push_back(callback);
			return *this;
		}

		template<Policy::Type T, typename Handler>
		ScopedEventSystem& subscribe(auto callback, Handler& handler)
		{
			Subscriber<T> wrappedSubscriber = [&handler, callback](auto&&... args) {
				(handler.*callback)(std::forward<decltype(args)>(args)...);
				};
			std::get<static_cast<size_t>(T)>(m_subscribers).push_back(wrappedSubscriber);
			return *this;
		}

		template<Policy::Type T, typename... Args>
		const ScopedEventSystem& emit(Args&&... args) const
		{
			const auto& subscribers = std::get<static_cast<size_t>(T)>(m_subscribers);
			for (const auto& subscriber : subscribers)
			{
				try
				{
					subscriber(std::forward<Args>(args)...);
				}
				catch (...) {
					if constexpr (hasErrorHandler<Policy, T>()) {
						Policy::template handleError<T>(std::current_exception());
					}
				}
			}
			return *this;
		}
	};
}