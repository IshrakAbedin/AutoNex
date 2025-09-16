#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "EventDispatcher.hpp"

enum class Dummy
{
	STATE1, STATE2, STATE3
};

namespace anx {
	template<typename state_t>
	class StateMachine
	{
	public:
		using rule_t = std::function<bool>;
		using rule_state_pair = std::pair<rule_t, state_t>;
		using transition_rule_list_t = std::vector<rule_state_pair>;
		using transition_registry_t = std::unordered_map<state_t, transition_rule_list_t>;
		
		using transition_action_t = std::function<void(state_t)>;
		using transition_event_dispatcher_t = EventDispatcher<state_t>;
		using transition_event_registry_t = std::unordered_map<state_t, transition_event_dispatcher_t>;

	private:
		state_t m_CurrentState;
		transition_registry_t m_RuleRegistry;
		transition_event_registry_t m_OnStateLeaveRegistry;
		transition_event_registry_t m_OnStateEntryRegistry;

	public:
		StateMachine(state_t initialState) : m_CurrentState{ initialState } {}
		// ToDo: Create move and copy constructors if required

		state_t GetCurrentState() const;
		void ForceSetCurrentState(state_t newState);

		void AddTranstionRule(state_t from, state_t to, rule_t rule);
		void BindOnStateLeave(state_t from, transition_action_t action);
		void BindOnStateEntry(state_t from, transition_action_t action);

		bool Step();
	};

	template<typename state_t>
	inline state_t StateMachine<state_t>::GetCurrentState() const
	{
		return state_t();
	}

	template<typename state_t>
	inline void StateMachine<state_t>::ForceSetCurrentState(state_t newState)
	{
	}

	template<typename state_t>
	inline void StateMachine<state_t>::AddTranstionRule(state_t from, state_t to, rule_t rule)
	{
	}

	template<typename state_t>
	inline void StateMachine<state_t>::BindOnStateLeave(state_t from, transition_action_t action)
	{
	}

	template<typename state_t>
	inline void StateMachine<state_t>::BindOnStateEntry(state_t from, transition_action_t action)
	{
	}

	template<typename state_t>
	inline bool StateMachine<state_t>::Step()
	{
		return false;
	}
}