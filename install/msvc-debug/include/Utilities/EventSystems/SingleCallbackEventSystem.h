#pragma once
#include "Namespaces.h"
#include "Utilities/EventSystems/EventSystemConcept.h"

#include <functional>
#include <vector>
#include <array>
#include <tuple>
#include <utility>

namespace Utilities {

	template<EventSystemPolicy Policy>
	class SingleCallbackEventSystem
	{
	public:
		template <Policy::Type T>
		using Trait = typename Policy::template Trait<T>;

		template<Policy::Type T>
		using Signature = Trait<T>::Signature;

		template<Policy::Type T>
		using Callback = std::function<Signature<T>>;

		static inline const EventSystemType s_eventSystemType = EventSystemType::SingleCallback;

	private:

		template<typename P = Policy, P::Type T = static_cast<P::Type>(0)>
		static constexpr bool hasErrorHandler() {
			return requires { P::template handleError<T>(std::declval<std::exception_ptr>()); };
		}

		template<size_t... Is>
		static constexpr auto makeContainer(std::index_sequence<Is...>) {
			return std::tuple<Callback<static_cast<Policy::Type>(Is)>...>{};
		}

		using Callbacks = decltype(makeContainer(std::make_index_sequence<static_cast<size_t>(Policy::Type::Count)>{}));

		Callbacks m_callbacks;

	public:

		SingleCallbackEventSystem()
		{
			std::apply([](auto&... callbacks) {
				((callbacks = [](auto&&...) {}), ...);
				}, m_callbacks);
		}
		~SingleCallbackEventSystem() = default;

		SingleCallbackEventSystem(SingleCallbackEventSystem&&) = default;
		SingleCallbackEventSystem& operator=(SingleCallbackEventSystem&&) = default;

		SingleCallbackEventSystem(const SingleCallbackEventSystem&) = delete;
		SingleCallbackEventSystem& operator=(const SingleCallbackEventSystem&) = delete;

		template<Policy::Type T, typename... Args>
		const SingleCallbackEventSystem& emit(Args&&... args) const {
			try {
				std::get<static_cast<size_t>(T)>(m_callbacks)(std::forward<Args>(args)...);
			}
			catch (...) {
				if constexpr (hasErrorHandler<Policy, T>()) {
					Policy::template handleError<T>(std::current_exception());
				}
			}
			return *this;
		}

		template<Policy::Type T, typename CallbackT>
		SingleCallbackEventSystem& set(CallbackT&& callback)
		{
			std::get<static_cast<size_t>(T)>(m_callbacks) = std::forward<CallbackT>(callback);
			return *this;
		}

		template<Policy::Type T, typename Handler, typename CallbackT>
		SingleCallbackEventSystem& set(CallbackT&& callback, Handler& handler)
		{
			Callback<T> wrappedCallback = [&handler, callbackInt = std::forward<CallbackT>(callback)](auto&&... args) {
				(handler.*callbackInt)(std::forward<decltype(args)>(args)...);
				};
			std::get<static_cast<size_t>(T)>(m_callbacks) = wrappedCallback;
			return *this;
		}

		template<Policy::Type T>
		SingleCallbackEventSystem& clear()
		{
			std::get<static_cast<size_t>(T)>(m_callbacks) = [](auto&&...) {};
			return *this;
		}
	};
}
