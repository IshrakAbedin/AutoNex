#pragma once

#include <vector>
#include <functional>

namespace anx {
	template <typename... params>
	class EventDispatcher
	{
	private:
		std::vector<std::function<void(params...)>> m_EventList;

	public:
		EventDispatcher() = default;
		~EventDispatcher() = default;
		EventDispatcher(const EventDispatcher& other) = delete;
		EventDispatcher operator=(const EventDispatcher& other) = delete;
		bool operator==(const EventDispatcher& other) = delete;

		// Subscribe to event, operator overload +=
		EventDispatcher& operator+=(const std::function<void(params...)> subscriber);

		// Subscribe to event, operator overload <<
		EventDispatcher& operator<<(const std::function<void(params...)> subscriber);

		// Unsubscribe from event, operator overload -=
		EventDispatcher& operator-=(const std::function<void(params...)> subscriber);

		// Unsubscribe from event, operator overload >>
		EventDispatcher& operator>>(const std::function<void(params...)> subscriber);

		// Subscribe to event
		EventDispatcher& Subscribe(const std::function<void(params...)> subscriber);

		// Unsubscribe from event
		EventDispatcher& Unsubscribe(const std::function<void(params...)> subscriber);

		// Unsubscribe all subscribers
		void UnsubscribeAll() noexcept;

		// Dispatch event
		void Dispatch(params... args) const;

		// Get Subscriber Count
		size_t GetSubscriberCount() const noexcept;
	};

	template <typename... params>
	inline EventDispatcher<params...>& EventDispatcher<params...>::operator+=(const std::function<void(params...)> subscriber)
	{
		return Subscribe(subscriber);
	}

	template <typename... params>
	inline EventDispatcher<params...>& EventDispatcher<params...>::operator<<(const std::function<void(params...)> subscriber)
	{
		return Subscribe(subscriber);
	}

	template <typename... params>
	inline EventDispatcher<params...>& EventDispatcher<params...>::operator-=(const std::function<void(params...)> subscriber)
	{
		return Unsubscribe(subscriber);
	}

	template <typename... params>
	inline EventDispatcher<params...>& EventDispatcher<params...>::operator>>(const std::function<void(params...)> subscriber)
	{
		return Unsubscribe(subscriber);
	}

	template <typename... params>
	inline EventDispatcher<params...>& EventDispatcher<params...>::Subscribe(const std::function<void(params...)> subscriber)
	{
		m_EventList.push_back(subscriber);
		return *this;
	}

	template <typename... params>
	EventDispatcher<params...>& EventDispatcher<params...>::Unsubscribe(const std::function<void(params...)> subscriber)
	{
		for (int i = 0; i < m_EventList.size(); i++)
		{
			if (m_EventList[i].target_type() == subscriber.target_type())
			{
				m_EventList.erase(m_EventList.begin() + i);
				break;
			}
		}
		return *this;
	}

	template <typename... params>
	inline void EventDispatcher<params...>::UnsubscribeAll() noexcept
	{
		m_EventList.clear();
	}

	template <typename... params>
	void EventDispatcher<params...>::Dispatch(params... args) const
	{
		for (auto& subscriber : m_EventList)
		{
			subscriber(args...);
		}
	}

	template <typename... params>
	inline size_t EventDispatcher<params...>::GetSubscriberCount() const noexcept
	{
		return m_EventList.size();
	}
} // namespace anx