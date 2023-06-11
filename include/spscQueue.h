#ifndef INCLUDE_SPSC_QUEUE_H
#define INCLUDE_SPSC_QUEUE_H

#include <algorithm>
#include <vector>
#include <optional>

namespace spsc
{

    // We can rid of modulos in two ways (Need to investigate)
    // 1. Manually check if it hits capactiy (Modulo has to do extra work since it checks every multiple)
    // 2. Make capacity a power of two and use bit shifting

    // Re-think if I really need to worry about the whole "no-throw" thing
    // Probably a speed things tbh

    template <typename T, typename Alloc = std::allocator<std::byte>>
        requires std::is_nothrow_copy_constructible<T>::value &&
                 std::is_nothrow_destructible<T>::value
    class spscQueue_t
    {
    public:
        explicit spscQueue_t(size_t maxCapacity)
        {
            // In theory... maybe this has a use
            // But for not, just don't let anyone make this
            assert(maxCapacity > 0);

            // There's multiple ways to do this, but we need an extra element here
            // to differentiate between empty and full
            m_capacity = maxCapacity + 1;
            m_rawMemory.resize((m_capacity) * sizeof(T));
        }

        ~spscQueue_t()
        {
            while (front().has_value())
            {
                pop();
            }
        }

        // We don't want any copying or moving
        spscQueue_t(const spscQueue_t &) = delete;
        spscQueue_t(spscQueue_t &&) = delete;

        spscQueue_t &operator=(const spscQueue_t &) = delete;
        spscQueue_t &operator=(spscQueue_t &&) = delete;

        template <typename... Args>
            requires std::is_nothrow_constructible_v<T, Args...>
        inline auto emplace(Args &&...args) noexcept -> void
        {
            // Busy wait until we get space
            while (is_full())
            {
            }

            new (&m_rawMemory[m_writeIdx]) T(std::forward<Args>(args)...);
            progressIndex<indexType_e::WRITE>();
        }

        template <typename... Args>
        inline auto tryEmplace(Args &&...args) -> bool
            requires std::is_nothrow_constructible_v<T, Args...>
        {
            if (is_full())
                return false;

            new (&m_rawMemory[m_writeIdx]) T(std::forward<Args>(args)...);
            progressIndex<indexType_e::WRITE>();

            return true;
        }

        // TODO: r-value versions of these?
        auto push(const T &val) noexcept -> void
            requires std::is_nothrow_copy_constructible<T>::value
        {
            emplace(val);
        }

        auto tryPush(const T &val) -> bool
            requires std::is_nothrow_copy_constructible<T>::value
        {
            return tryEmplace(val);
        }

        auto front() noexcept -> std::optional<std::reference_wrapper<T>>
        {
            if (is_empty())
                return {};

            auto &valueToReturn = reinterpret_cast<T &>(m_rawMemory[m_readIdx]);
            progressIndex<indexType_e::READ>();

            return valueToReturn;
        }

        auto pop() noexcept -> void
        {
            assert(is_empty() == false);

            // If these do special work in their dtor(), make sure it's done
            reinterpret_cast<T &>(m_rawMemory[m_readIdx]).~T();
            progressIndex<indexType_e::READ>();
        }

        auto size() const noexcept -> size_t
        {
            return m_rawMemory.size() / sizeof(T);
        }

        auto is_empty() const noexcept -> bool
        {
            return m_writeIdx == m_readIdx;
        }

        auto is_full() const noexcept -> bool
        {
            return ((m_writeIdx + 1) % m_capacity) == m_readIdx;
        }

        auto capacity() const noexcept -> size_t
        {
            return m_capacity - 1;
        }

    private:
        enum class indexType_e
        {
            READ,
            WRITE
        };

        template <indexType_e INDEX_TYPE>
        constexpr inline auto whichIndex() noexcept -> size_t &
        {
            if constexpr (INDEX_TYPE == indexType_e::WRITE)
                return m_writeIdx;
            return m_readIdx;
        }

        template <indexType_e INDEX_TYPE>
        inline auto progressIndex() noexcept -> void
        {
            auto &indexToIncrement = whichIndex<INDEX_TYPE>();

            // Do it manually to prevent the cost of modulo
            if (indexToIncrement == m_capacity)
            {
                indexToIncrement = 0;
            }
            else
            {
                indexToIncrement++;
            }
        }

        // #if defined(__has_cpp_attribute) && __has_cpp_attribute(no_unique_address)
        //         Alloc allocator_ [[no_unique_address]];
        // #else
        //         Alloc allocator_;
        // #endif

#ifdef __cpp_lib_hardware_interference_size
        using cacheLineSize = std::hardware_destructive_interference_size;
#else
        // Apple Clang :(
        inline static constexpr std::size_t cacheLineSize = 64;
#endif
        size_t m_capacity;

        size_t m_writeIdx{0};
        size_t m_readIdx{0};

        std::vector<std::byte, Alloc> m_rawMemory;
    };
};

#endif // INCLUDE_SPSC_QUEUE_H