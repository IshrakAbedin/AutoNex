#pragma once

#include <array>
#include <atomic>

namespace anx
{
	// A latest-value strict SPSC triple buffer
	template <typename T>
	class TripleBuffer
	{
	private:
		std::array<T, 3> m_Buffers;

		unsigned m_WriteIndex{ 0u };
		unsigned m_ReadIndex{ 1u };
		std::atomic<unsigned> m_MiddleIndex{ 2u };
		std::atomic_bool m_HasData{ false };

	public:
		TripleBuffer() = default;
		TripleBuffer(const TripleBuffer& other) = delete;
		TripleBuffer& operator=(const TripleBuffer& other) = delete;

		T& GetWriteBuffer()
		{
			return m_Buffers[m_WriteIndex];
		}

		void Publish()
		{
			m_WriteIndex = m_MiddleIndex.exchange(m_WriteIndex, std::memory_order_release);
			m_HasData.store(true, std::memory_order_relaxed);
		}

		bool CheckNew()
		{
			if (m_HasData.load(std::memory_order_relaxed))
			{
				m_HasData.store(false, std::memory_order_relaxed);
				m_ReadIndex = m_MiddleIndex.exchange(m_ReadIndex, std::memory_order_acquire);
				return true;
			}
			else
				return false;
		}

		const T& GetReadBuffer() const
		{
			return m_Buffers[m_ReadIndex];
		}
	};
} // namespace anx