#pragma once

#include <vector>
#include <deque>
#include <optional>
#include <functional>
#include <cstddef> 
#include <cstdint>
#include <new>
#include <type_traits>

namespace anx {
	enum class PoolAllocationStrategy
	{
		STATIC, DYNAMIC
	};

	template <typename T, PoolAllocationStrategy Strategy = PoolAllocationStrategy::STATIC>
	class ObjectPool {
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
		std::vector<uint32_t> m_FreeList;

	public:
		struct Handle
		{
			uint32_t Index;
			uint64_t Generation;
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
			for (size_t i = 0; i < m_Storage.size(); ++i) {
				if (m_Alive[i]) {
					reinterpret_cast<T*>(&m_Storage[i].Data)->~T();
				}
			}
		}
	}

	template<typename T, PoolAllocationStrategy Strategy>
	template<typename ...Args>
	inline ObjectPool<T, Strategy>::Handle ObjectPool<T, Strategy>::Create(Args && ...args)
	{
		uint32_t Index;
		if (!m_FreeList.empty())
		{
			Index = m_FreeList.back();
			m_FreeList.pop_back();
			m_Generations[Index]++; // invalidate old handles
		}
		else
		{
			Index = static_cast<decltype(Index)>(m_Storage.size());
			m_Storage.emplace_back();  // raw storage for T
			m_Generations.push_back(0);
			m_Alive.push_back(0);
		}

		void* ptr = &m_Storage[Index].Data;
		new (ptr) T(std::forward<Args>(args)...); // placement new
		m_Alive[Index] = 1;

		return Handle{ Index, m_Generations[Index] };
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
		return *reinterpret_cast<T*>(&m_Storage[h.Index].Data);
	}

	template<typename T, PoolAllocationStrategy Strategy>
	inline std::optional<std::reference_wrapper<const T>> ObjectPool<T, Strategy>::Get(Handle h) const
	{
		if (!Validate(h)) return std::nullopt;
		return *reinterpret_cast<const T*>(&m_Storage[h.Index].Data);
	}

	template<typename T, PoolAllocationStrategy Strategy>
	inline size_t ObjectPool<T, Strategy>::Size() const noexcept
	{
		return m_Storage.size() - m_FreeList.size();
	}
} // namespace anx
