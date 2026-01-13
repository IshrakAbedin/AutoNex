#pragma once

#include <chrono>
#include <functional>
#include <concepts>
#include <type_traits>
#include <queue>

#include "autonex/ObjectPool.hpp"

namespace anx {
	using timerclock = std::chrono::steady_clock;
	using nstimepoint_t = std::chrono::time_point<timerclock, std::chrono::nanoseconds>;
	//using nsduration_t = std::chrono::duration<std::chrono::nanoseconds>;
	using nsduration_t = std::chrono::nanoseconds;
	using namespace std::chrono_literals;

	struct TimerProperty
	{
		bool IsRepeating;
		bool IsEnabled;
		nsduration_t Interval;
		std::function<void()> OnTimeout;
	};

	template<PoolAllocationStrategy Strategy = PoolAllocationStrategy::STATIC>
	class Timer
	{
		using timerpropertyhandle_t = ObjectPool<TimerProperty, Strategy>::Handle;
	private:
		nstimepoint_t m_When;
		timerpropertyhandle_t m_PropertyHandle;

	public:
		/*Timer(std::convertible_to<std::chrono::nanoseconds> auto interval, timerpropertyhandle_t handle)
			: m_When{ timerclock::now() + interval }, m_PropertyHandle{ handle }
		{
		}*/

		Timer(nstimepoint_t when, timerpropertyhandle_t handle)
			: m_When{ when }, m_PropertyHandle{ handle }
		{
		}

		bool operator>(const Timer& other) const
		{
			return this->m_When > other.m_When;
		}

		inline nstimepoint_t When() const
		{
			return m_When;
		}

		inline timerpropertyhandle_t GetHandle() const
		{
			return m_PropertyHandle;
		}
	};

	template<PoolAllocationStrategy Strategy = PoolAllocationStrategy::STATIC>
	class TimerManager
	{
		using timerpropertypool_t = ObjectPool<TimerProperty, Strategy>;
		using timerpropertyhandle_t = ObjectPool<TimerProperty, Strategy>::Handle;
	private:
		std::priority_queue<Timer<Strategy>, std::vector<Timer<Strategy>>, std::greater<Timer<Strategy>>> m_TimerQueue;
		ObjectPool<TimerProperty, Strategy> m_TimerPropertyPool;

	public:
		class TimerHandler;
		friend class TimerHandler;

		template<PoolAllocationStrategy S = Strategy>
			requires (S == PoolAllocationStrategy::STATIC)
		explicit TimerManager(size_t reserveSize)
			: m_TimerPropertyPool{ reserveSize }
		{
		}

		template<PoolAllocationStrategy S = Strategy>
			requires (S == PoolAllocationStrategy::DYNAMIC)
		TimerManager() {}

		inline TimerHandler CreateTimer(std::convertible_to<std::chrono::nanoseconds> auto interval, bool isRepeating, std::function<void()> onTimeout, bool isEnabled = true)
		{
			auto propertyHandle = m_TimerPropertyPool.Create(isRepeating, isEnabled, interval, onTimeout);
			//m_TimerQueue.emplace(interval, propertyHandle);
			auto executionTime = timerclock::now() + interval;
			m_TimerQueue.emplace(executionTime, propertyHandle);
			return TimerHandler{ propertyHandle, *this };
		}

		inline void RequeueTimer(timerpropertyhandle_t handle, nstimepoint_t previousScheduledTime)
		{
			if (auto opt = m_TimerPropertyPool.Get(handle))
			{
				m_TimerQueue.emplace(previousScheduledTime + opt->get().Interval, handle);
			}
		}

		inline void RequeueTimer(timerpropertyhandle_t handle)
		{
			if (auto opt = m_TimerPropertyPool.Get(handle))
			{
				m_TimerQueue.emplace(timerclock::now() + opt->get().Interval, handle);
			}
		}

		inline void Tick()
		{
			auto now = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
			while (!m_TimerQueue.empty() && now >= m_TimerQueue.top().When())
			{
				Timer<Strategy> timer = m_TimerQueue.top();
				auto propertyHandle = timer.GetHandle();
				auto scheduledTime = timer.When();
				m_TimerQueue.pop();

				if (auto opt = m_TimerPropertyPool.Get(propertyHandle))
				{
					if (opt->get().IsEnabled)
					{
						opt->get().OnTimeout(); // This callback may add new timer, potentially invalidating reference
					}
				}
				if (auto opt = m_TimerPropertyPool.Get(propertyHandle))
				{
					if (opt->get().IsRepeating)
					{
						RequeueTimer(propertyHandle, scheduledTime);
					}
					else
					{
						m_TimerPropertyPool.Destroy(propertyHandle);
					}
				}
			}
		}

		class TimerHandler
		{
		private:
			timerpropertyhandle_t m_Handle;
			TimerManager<Strategy>& m_Manager;
		public:
			// Must not outlive the manager
			TimerHandler(timerpropertyhandle_t handle, TimerManager<Strategy>& manager)
				: m_Handle{ handle }, m_Manager{ manager }
			{
			}

			inline bool IsValid() const
			{
				return m_Manager.m_TimerPropertyPool.Get(m_Handle).has_value();
			}

			inline void Disable()
			{
				if (std::optional<std::reference_wrapper<TimerProperty>> opt = m_Manager.m_TimerPropertyPool.Get(m_Handle))
				{
					opt->get().IsEnabled = false;
				}
			}

			inline void Enable()
			{
				if (std::optional<std::reference_wrapper<TimerProperty>> opt = m_Manager.m_TimerPropertyPool.Get(m_Handle))
				{
					opt->get().IsEnabled = true;
				}
			}

			inline bool IsEnabled() const
			{
				if (std::optional<std::reference_wrapper<TimerProperty>> opt = m_Manager.m_TimerPropertyPool.Get(m_Handle))
				{
					return opt->get().IsEnabled;
				}
				return false;
			}

			inline void SetRepeating()
			{
				if (std::optional<std::reference_wrapper<TimerProperty>> opt = m_Manager.m_TimerPropertyPool.Get(m_Handle))
				{
					opt->get().IsRepeating = true;
				}
			}

			inline void UnsetRepeating()
			{
				if (std::optional<std::reference_wrapper<TimerProperty>> opt = m_Manager.m_TimerPropertyPool.Get(m_Handle))
				{
					opt->get().IsRepeating = false;
				}
			}

			inline bool IsRepeating() const
			{
				if (std::optional<std::reference_wrapper<TimerProperty>> opt = m_Manager.m_TimerPropertyPool.Get(m_Handle))
				{
					return opt->get().IsRepeating;
				}
				return false;
			}

			inline void SetInterval(nsduration_t interval)
			{
				if (std::optional<std::reference_wrapper<TimerProperty>> opt = m_Manager.m_TimerPropertyPool.Get(m_Handle))
				{
					opt->get().Interval = interval;
				}
			}

			inline std::optional<nsduration_t> GetInterval() const
			{
				if (std::optional<std::reference_wrapper<TimerProperty>> opt = m_Manager.m_TimerPropertyPool.Get(m_Handle))
				{
					return opt->get().Interval;
				}
				return std::nullopt;
			}

			inline void SetOnTimeout(std::function<void()> onTimeout)
			{
				if (std::optional<std::reference_wrapper<TimerProperty>> opt = m_Manager.m_TimerPropertyPool.Get(m_Handle))
				{
					opt->get().OnTimeout = onTimeout;
				}
			}

			inline void QueueDestroy()
			{
				Disable();
				UnsetRepeating();
			}
		};
	};
}