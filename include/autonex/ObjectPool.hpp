#pragma once

#include <vector>
#include <deque>
#include <optional>
#include <functional>
#include <cstddef> 
#include <cstdint>
#include <new>
#include <type_traits>
#include <limits>
#include <stdexcept>

namespace anx {
	enum class PoolAllocationStrategy
	{
		STATIC, DYNAMIC
	};

	template<typename T, PoolAllocationStrategy Strategy = PoolAllocationStrategy::STATIC>
	class ObjectPool
	{
	private:
		struct alignas(T) Storage
		{
			std::byte Data[sizeof(T)];
		};

		using container_t = std::conditional_t<
			Strategy == PoolAllocationStrategy::STATIC,
			std::vector<Storage>,
			std::deque<Storage>>;

		container_t m_Storage;
		std::vector<uint64_t> m_Generations;
		std::vector<uint8_t> m_Alive;

		using index_t = uint32_t;
		std::vector<index_t> m_FreeList;

	public:
		struct Handle
		{
			friend class ObjectPool<T, Strategy>;

		private:
			index_t Index;
			uint64_t Generation;

			Handle(index_t index, uint64_t generation)
				: Index{ index }, Generation{ generation }
			{
			}

		public:
			bool operator==(const Handle& other) const noexcept
			{
				return Index == other.Index && Generation == other.Generation;
			}

			bool operator!=(const Handle& other) const noexcept
			{
				return !(*this == other);
			}

			struct Hash
			{
				size_t operator()(const Handle& h) const noexcept
				{
					size_t seed = std::hash<index_t>{}(h.Index);
					seed ^= std::hash<uint64_t>{}(h.Generation) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
					return seed;
				}
			};
		};

		template<PoolAllocationStrategy S = Strategy>
			requires (S == PoolAllocationStrategy::STATIC)
		explicit ObjectPool(size_t reserveSize);

		template<PoolAllocationStrategy S = Strategy>
			requires (S == PoolAllocationStrategy::DYNAMIC)
		ObjectPool();

		ObjectPool(const ObjectPool&) = delete;
		ObjectPool& operator=(const ObjectPool&) = delete;
		ObjectPool(ObjectPool&&) noexcept = default;
		ObjectPool& operator=(ObjectPool&&) noexcept = default;
		~ObjectPool();

		template <typename... Args>
		Handle Create(Args&&... args);
		bool Destroy(Handle h);

		bool Validate(Handle h) const;

		std::optional<std::reference_wrapper<T>> Get(Handle h);
		std::optional<std::reference_wrapper<const T>> Get(Handle h) const;

		size_t Size() const noexcept;
		size_t Capacity() const noexcept;
		size_t FreeSlotCount() const noexcept;
	};

	template<typename T, PoolAllocationStrategy Strategy>
	template<PoolAllocationStrategy S>
		requires (S == PoolAllocationStrategy::STATIC)
	inline ObjectPool<T, Strategy>::ObjectPool(size_t reserveSize)
	{
		if (reserveSize > 0)
		{
			m_Storage.reserve(reserveSize);
			m_Generations.reserve(reserveSize);
			m_Alive.reserve(reserveSize);
		}
	}

	template<typename T, PoolAllocationStrategy Strategy>
	template<PoolAllocationStrategy S>
		requires (S == PoolAllocationStrategy::DYNAMIC)
	inline ObjectPool<T, Strategy>::ObjectPool()
	{
		// Empty as no reservation for std::deque
	}

	template<typename T, PoolAllocationStrategy Strategy>
	inline ObjectPool<T, Strategy>::~ObjectPool()
	{
		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			for (size_t i = 0; i < m_Storage.size(); ++i)
			{
				if (m_Alive[i])
				{
					reinterpret_cast<T*>(&m_Storage[i].Data)->~T();
				}
			}
		}
	}

	template<typename T, PoolAllocationStrategy Strategy>
	template<typename ...Args>
	inline ObjectPool<T, Strategy>::Handle ObjectPool<T, Strategy>::Create(Args && ...args)
	{
		index_t Index;
		if (!m_FreeList.empty())
		{
			Index = m_FreeList.back();
			m_FreeList.pop_back();
		}
		else
		{
			auto current_size = m_Storage.size();
			if (current_size >= std::numeric_limits<index_t>::max())
			{
				throw std::overflow_error("ObjectPool: Maximum capacity reached (index overflow)");
			}

			Index = static_cast<index_t>(current_size);
			m_Storage.emplace_back(); // raw storage for T
			m_Generations.push_back(0);
			m_Alive.push_back(0);
		}

		void* ptr = &m_Storage[Index].Data;
		
		try {
			new (ptr) T(std::forward<Args>(args)...);
			m_Alive[Index] = 1;

			return Handle{ Index, m_Generations[Index] };
		}
		catch (...) {
			// Rollback: return index to free list for future reuse
			m_FreeList.push_back(Index);
			throw; // Re-throw the exception
		}
	}

	template<typename T, PoolAllocationStrategy Strategy>
	inline bool ObjectPool<T, Strategy>::Destroy(Handle h)
	{
		if (!Validate(h)) return false;

		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			T* obj = reinterpret_cast<T*>(&m_Storage[h.Index].Data);
			obj->~T();
		}

		m_Alive[h.Index] = 0;
		m_FreeList.push_back(h.Index);
		m_Generations[h.Index]++;
		return true;
	}

	template<typename T, PoolAllocationStrategy Strategy>
	inline bool ObjectPool<T, Strategy>::Validate(Handle h) const
	{
		return h.Index < m_Storage.size() &&
			m_Alive[h.Index] &&
			m_Generations[h.Index] == h.Generation;
	}

	template<typename T, PoolAllocationStrategy Strategy>
	inline std::optional<std::reference_wrapper<T>> ObjectPool<T, Strategy>::Get(Handle h)
	{
		if (!Validate(h)) return std::nullopt;
		return std::ref(*reinterpret_cast<T*>(&m_Storage[h.Index].Data));
	}

	template<typename T, PoolAllocationStrategy Strategy>
	inline std::optional<std::reference_wrapper<const T>> ObjectPool<T, Strategy>::Get(Handle h) const
	{
		if (!Validate(h)) return std::nullopt;
		return std::cref(*reinterpret_cast<const T*>(&m_Storage[h.Index].Data));
	}

	template<typename T, PoolAllocationStrategy Strategy>
	inline size_t ObjectPool<T, Strategy>::Size() const noexcept
	{
		return m_Storage.size() - m_FreeList.size();
	}

	template<typename T, PoolAllocationStrategy Strategy>
	inline size_t ObjectPool<T, Strategy>::Capacity() const noexcept
	{
		return m_Storage.size();
	}

	template<typename T, PoolAllocationStrategy Strategy>
	inline size_t ObjectPool<T, Strategy>::FreeSlotCount() const noexcept
	{
		return m_FreeList.size();
	}
} // namespace anx
