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

    template <typename T, typename Alloc = std::allocator<T>>
    class spscQueue_t
    {
    public:
        explicit spscQueue_t(size_t maxCapacity) : m_capacity{},
                                                   m_writeIdx{},
                                                   m_readIdx{},
                                                   m_rawMemory{}
        {
            assert(maxCapacity > 0);

            // There's multiple ways to do this, but we need an extra element here
            // to differentiate between empty and full
            m_capacity = maxCapacity + 1;
            m_rawMemory.resize(m_capacity);
        }

        ~spscQueue_t()
        {
            while (front())
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
            progress_index<indexType_e::WRITE>();
        }

        // The throwing version
        template <typename... Args>
            requires(std::is_constructible_v<T, Args...> && std::is_nothrow_constructible_v<T, Args...> == false)
        inline auto emplace(Args &&...args) -> void
        {
            std::cout << "Lets maybe throw" << std::endl;

            // Busy wait until we get space
            while (is_full())
            {
            }

            new (&m_rawMemory[m_writeIdx]) T(std::forward<Args>(args)...);
            progress_index<indexType_e::WRITE>();
        }

        template <typename... Args>
        inline auto try_emplace(Args &&...args) -> bool
            requires std::is_nothrow_constructible_v<T, Args...>
        {
            if (is_full())
                return false;

            new (&m_rawMemory[m_writeIdx]) T(std::forward<Args>(args)...);
            progress_index<indexType_e::WRITE>();

            return true;
        }

        auto push(const T &val) noexcept -> void
        {
            emplace(val);
        }

        auto try_push(const T &val) -> bool
        {
            return tryEmplace(val);
        }

        auto take_and_pop() noexcept -> std::optional<T>
        {
            if (is_empty())
                return {};

            // We HAVE to make a copy here since we use a circular buffer
            // and the data will be invalidated on the next right

            // If you really wanted to not copy, in theory... use unique ptr as the type
            T valCopy = front().value();
            pop();

            return valCopy;
        }

        auto front() noexcept -> std::optional<std::reference_wrapper<T>>
        {
            if (is_empty())
                return {};

            return m_rawMemory[m_readIdx];
        }

        auto pop() noexcept -> void
        {
            assert(is_empty() == false);

            // If these do special work in their dtor(), make sure it's done
            m_rawMemory[m_readIdx].~T();
            progress_index<indexType_e::READ>();
        }

        auto size() const noexcept -> size_t
        {
            // Write index should always be "ahead or equal" relative to read index
            std::ptrdiff_t diff = m_writeIdx - m_readIdx;
            if (diff < 0)
                diff += m_capacity;

            return static_cast<size_t>(diff);
        }

        auto is_empty() const noexcept -> bool
        {
            return m_writeIdx == m_readIdx;
        }

        // We do it the long way to avoid the modulo
        auto is_full() const noexcept -> bool
        {
            return size() == m_capacity;
        }

        auto capacity() const noexcept -> size_t
        {
            return m_capacity - 1;
        }

    private:
        enum class indexType_e : bool
        {
            READ,
            WRITE
        };

        template <indexType_e INDEX_TYPE>
        constexpr inline auto which_index() noexcept -> size_t &
        {
            if constexpr (INDEX_TYPE == indexType_e::WRITE)
                return m_writeIdx;
            return m_readIdx;
        }

        template <indexType_e INDEX_TYPE>
        inline auto progress_index() noexcept -> void
        {
            auto &indexToIncrement = which_index<INDEX_TYPE>();

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
        size_t m_capacity; // How much memory we're currently holding
                           // Probably could be simplified into grabbing the m_rawMemory size

        size_t m_writeIdx; // What location to write to next
        size_t m_readIdx;  // What location to read from next

        std::vector<T, Alloc> m_rawMemory; // The underlying memory
    };
};

#endif // INCLUDE_SPSC_QUEUE_H