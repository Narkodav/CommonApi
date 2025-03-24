#pragma once
#include "../Namespaces.h"

#include <vector>
#include <functional>
#include <type_traits>
#include <stdexcept>
#include <memory>
#include <shared_mutex>

namespace MultiThreading
{
	template <typename Policy>
	class EventSystem
	{
	private:
		//used as a unique identifier for each subscription
		class SubscriptionBase : public std::enable_shared_from_this<SubscriptionBase> //exists only in a form of shared pointers
		{
		private:
			EventSystem* m_owner;

		public:
			SubscriptionBase(EventSystem* owner) : m_owner(owner) {};

			SubscriptionBase(const SubscriptionBase&) = delete;
			SubscriptionBase& operator=(const SubscriptionBase&) = delete;

			SubscriptionBase(SubscriptionBase&&) noexcept = delete;
			SubscriptionBase& operator=(SubscriptionBase&&) noexcept = delete;

			void unsubscribe()
			{
				std::unique_lock lock(m_owner->m_mutex);
				m_owner->storage.erase(this->shared_from_this());
				m_owner = nullptr;
			}

			void migrate(EventSystem* newOwner)
			{
				m_owner = newOwner;
			}

			friend class EventSystem;
		};

	public:
		using EventEnum = typename Policy::EventEnum;

		template<EventEnum E>
		struct Event
		{
			using Signature = typename Policy::template Traits<E>::Signature;
			using Callback = std::function<Signature>;
			using Subscribers = std::vector<std::pair<std::shared_ptr<SubscriptionBase>, Callback>>;
			Subscribers subs;

			Event() : subs() {};

			Event(const Event&) = delete;
			Event& operator=(const Event&) = delete;

			Event(Event&&) noexcept = default;
			Event& operator=(Event&&) noexcept = default;
		};

		// Helper tuple type that contains vectors for each enum value
		template<EventEnum... Es>
		struct EventStorage {
			std::tuple<Event<Es>...> events;
			static const size_t size = Policy::EVENT_NUM;

			// Helper method to get the vector for a specific event
			template<EventEnum E>
			Event<E>& getEvent() {
				return std::get<Event<E>>(events);
			}

			template<EventEnum E>
			const Event<E>& getEvent() const {
				return std::get<Event<E>>(events);
			}

			template<typename Func>
			void iterate(Func&& callback) {
				[&] <size_t... I>(std::index_sequence<I...>) {
					//will short circuit on false return
					(... && callback(std::get<Event<static_cast<EventEnum>(I)>>(events)));
				}(std::make_index_sequence<size>{});
			}

			template<typename Func>
			void iterate(Func&& callback) const {
				[&] <size_t... I>(std::index_sequence<I...>) {
					//will short circuit on false return
					(... && callback(std::get<Event<static_cast<EventEnum>(I)>>(events)));
				}(std::make_index_sequence<size>{});
			}

			void erase(std::shared_ptr<SubscriptionBase> subscription)
			{
				iterate([&](auto& event) -> bool {
					auto it = std::find_if(event.subs.begin(), event.subs.end(),
						[&](const auto& sub) {
							return subscription.get() == sub.first.get();
						});
					if (it != event.subs.end()) {
						event.subs.erase(it);
						return false;  // Stop iteration
					}
					return true;  // Continue to next event
					});
			}

			EventStorage() : events{ Event<Es>{}... } {};

			EventStorage(const EventStorage&) = delete;
			EventStorage& operator=(const EventStorage&) = delete;

			EventStorage(EventStorage&&) noexcept = default;
			EventStorage& operator=(EventStorage&&) noexcept = default;
		};

		// Create sequence of enum values
		template<std::size_t... Is>
		static constexpr auto makeEnumSequence(std::index_sequence<Is...>) {
			return EventStorage<static_cast<EventEnum>(Is)...>{};
		}

		using makeEventStorage = decltype(makeEnumSequence(std::make_index_sequence<Policy::EVENT_NUM>{}));

	private:
		// Storage for all event vectors
		makeEventStorage storage;
		mutable std::shared_mutex m_mutex;

	public:

		class Subscription
		{
		private:
			std::weak_ptr<SubscriptionBase> m_base;

		public:
			Subscription() : m_base(std::weak_ptr<SubscriptionBase>()) {};
			Subscription(std::shared_ptr<SubscriptionBase>& base) : m_base(base) {};

			Subscription(const Subscription&) = delete;
			Subscription& operator=(const Subscription&) = delete;

			Subscription(Subscription&&) = default;

			Subscription& operator=(Subscription&& other)
			{
				if (this == &other)
					return *this;
				unsubscribe();
				m_base = std::exchange(other.m_base, std::weak_ptr<SubscriptionBase>());

				return *this;
			}

			~Subscription() { unsubscribe(); }

			void unsubscribe() {
				auto lock = m_base.lock();
				if (lock == nullptr)
					return;
				lock->unsubscribe();
				m_base.reset();
			}

			inline bool isValid() const { return !m_base.expired(); };
		};

		EventSystem() : storage() {};

		EventSystem(const EventSystem&) = delete;
		EventSystem& operator=(const EventSystem&) = delete;

		EventSystem(EventSystem&& other) noexcept
		{
			std::scoped_lock locks(m_mutex, other.m_mutex);
			storage = std::exchange(other.storage, makeEventStorage{});
			storage.iterate([this](auto& event) {
				for (auto& sub : event.subs)
				{
					sub.first->migrate(this);
				}
				return true;
				});
		};

		EventSystem& operator=(EventSystem&& other) noexcept
		{
			if (this == &other)
				return *this;

			std::scoped_lock locks(m_mutex, other.m_mutex);
			storage = std::exchange(other.storage, makeEventStorage{});
			storage.iterate([this](auto& event) {
				for (auto& sub : event.subs)
				{
					sub.first->migrate(this);
				}
				return true;
				});

			return *this;
		};

		template<EventEnum E>
		Subscription subscribe(typename Event<E>::Callback callback) {
			std::unique_lock lock(m_mutex);
			auto base = std::make_shared<SubscriptionBase>(this);
			Subscription sub(base);
			storage.template getEvent<E>().subs.push_back(
				std::make_pair(std::move(base), std::move(callback)));
			return sub;
		}

		//emissing the same event simultaneously is legal, callback should manage their thread safety internally
		template<EventEnum E, typename... Args>
		void emit(Args&&... args) const {
			std::shared_lock lock(m_mutex);
			static_assert(std::is_invocable_v<typename Event<E>::Signature, Args...>,
				"Parameter types don't match event signature");

			const auto& subs = storage.template getEvent<E>().subs;
			for (const auto& sub : subs) {
				sub.second(std::forward<Args>(args)...);
			}
		}

		bool hasSubscribers() const {
			std::shared_lock lock(m_mutex);
			bool validator = false;
			storage.iterate([&validator](auto& event) {
				if (!event.subs.empty())
				{
					validator = true;
					return false;
				}
				return true;
				});
			return validator;
		}

		template<EventEnum E>
		bool hasSubscribers() const {
			std::shared_lock lock(m_mutex);
			return !storage.template getEvent<E>().subs.empty();
		}

		void clear() {
			std::unique_lock lock(m_mutex);
			storage.iterate([](auto& event) {
				event.subs.clear();
				return true;
				});
		}

		template<EventEnum E>
		void clear() {
			std::unique_lock lock(m_mutex);
			storage.template getEvent<E>().subs.clear();
		}
	};
}