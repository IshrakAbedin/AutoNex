#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "EventDispatcher.hpp"

namespace anx {
	template<typename state_t>
	class StateMachine
	{
	public:
		using rule_t = std::function<bool()>;
		using rule_state_pair_t = std::pair<rule_t, state_t>;
		using transition_rule_list_t = std::vector<rule_state_pair_t>;
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
		
		// No special member functions due to the Rule of Zero

		state_t GetCurrentState() const;
		void ForceSetCurrentState(state_t newState);

		void AddTranstionRule(state_t from, state_t to, rule_t rule);
		void BindOnStateLeave(state_t from, transition_action_t action);
		void BindOnStateEntry(state_t into, transition_action_t action);

		bool Step();
	};

	template<typename state_t>
	inline state_t StateMachine<state_t>::GetCurrentState() const
	{
		return m_CurrentState;
	}

	template<typename state_t>
	inline void StateMachine<state_t>::ForceSetCurrentState(state_t newState)
	{
		m_CurrentState = newState;
	}

	template<typename state_t>
	inline void StateMachine<state_t>::AddTranstionRule(state_t from, state_t to, rule_t rule)
	{
		m_RuleRegistry[from].emplace_back(rule, to);
	}

	template<typename state_t>
	inline void StateMachine<state_t>::BindOnStateLeave(state_t from, transition_action_t action)
	{
		m_OnStateLeaveRegistry[from] << action;
	}

	template<typename state_t>
	inline void StateMachine<state_t>::BindOnStateEntry(state_t into, transition_action_t action)
	{
		m_OnStateEntryRegistry[into] << action;
	}

	template<typename state_t>
	inline bool StateMachine<state_t>::Step()
	{
		auto it = m_RuleRegistry.find(m_CurrentState);
		if (it != m_RuleRegistry.end())
		{
			for (const auto& [rule, newState] : it->second)
			{
				if (rule())
				{
					// Check and fire on leave events
					if (auto it_l = m_OnStateLeaveRegistry.find(m_CurrentState);
						it_l != m_OnStateLeaveRegistry.end())
					{
						it_l->second.Dispatch(newState);
					}

					// Remember and transit
					auto oldState = m_CurrentState;
					m_CurrentState = newState;

					// Check and fire on entry events
					if (auto it_e = m_OnStateEntryRegistry.find(m_CurrentState);
						it_e != m_OnStateEntryRegistry.end())
					{
						it_e->second.Dispatch(oldState);
					}

					return true;
				}
			}
		}
		return false;
	}
} // namespace anx
