#pragma once
#include "../Namespaces.h"

/**
 * @brief Policy class for defining event types and their callback signatures
 *
 * This policy defines the event types and their associated callback signatures for the event system.
 * It requires an enum class for event types and the number of events as template parameters.
 *
 * Requirements:
 * 1. EventEnum must be an enum class with sequential values starting from 0
 * 2. num must match the number of events defined in EventEnum
 * 3. Each event type must have a corresponding Traits specialization
 *
 * There are two ways to define your policy:
 *
 * 1. Direct specialization approach:
 * @code
 * enum class GameEvents {
 *     OnPlayerDeath = 0,  // Must start at 0
 *     OnScoreChange,      // = 1
 *     OnLevelComplete     // = 2
 * };
 *
 * template<>
 * struct EventPolicy<GameEvents, 3> {
 *     using EventEnum = GameEvents;
 *     static const size_t EVENT_NUM = 3;
 *
 *     template<EventEnum E>
 *     struct Traits;
 *
 *     template<>
 *     struct Traits<GameEvents::OnPlayerDeath> {
 *         using Signature = void(int playerId);
 *     };
 *     // ... other specializations
 * };
 * @endcode
 *
 * 2. Inheritance approach:
 * @code
 * enum class GameEvents {
 *     OnPlayerDeath = 0,
 *     OnScoreChange,
 *     OnLevelComplete
 * };
 *
 * struct GamePolicy : public EventPolicy<GameEvents, 3> {};
 *
 * template<>
 * template<>
 * struct GamePolicy::Traits<GameEvents::OnPlayerDeath> {
 *     using Signature = void(int playerId);
 * };
 *
 * template<>
 * template<>
 * struct GamePolicy::Traits<GameEvents::OnScoreChange> {
 *     using Signature = void(int playerId, int newScore);
 * };
 *
 * template<>
 * template<>
 * struct GamePolicy::Traits<GameEvents::OnLevelComplete> {
 *     using Signature = void(int levelId, float completionTime);
 * };
 * @endcode
 *
 * The inheritance approach can be cleaner when you have many events as it allows
 * separating the policy definition from its traits specializations.
 *
 * @tparam EventEnum An enum class defining the event types. Values MUST:
 *                   - Start from 0
 *                   - Be sequential (no gaps)
 *                   - Match the num parameter in count
 * @tparam num The total number of events defined in EventEnum
 */

namespace MultiThreading
{
	template<typename Enum, size_t num>
	struct EventPolicy
	{
		using EventEnum = Enum;
		static const size_t EVENT_NUM = num;

		/**
		* @brief Traits template that must be specialized for each event type
		*
		* Each specialization must define:
		* - Signature: A function type describing the callback signature for the event
		*
		* @tparam E The event type to define traits for
		*/
		template <EventEnum E>
		struct Traits;
	};
}