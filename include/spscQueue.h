#ifndef INCLUDE_SPSC_QUEUE_H
#define INCLUDE_SPSC_QUEUE_H

namespace spsc
{
    template <typename T>
    class spscQueue_t
    {
    public:
        explicit spscQueue_t() {}

        ~spscQueue_t() {}

        // We don't want any copying or moving
        spscQueue_t(const spscQueue_t &) = delete;
        spscQueue_t(spscQueue_t &&) = delete;

        spscQueue_t &operator=(const spscQueue_t &) = delete;
        spscQueue_t &operator=(spscQueue_t &&) = delete;

        auto emplace() -> void
        {
        }

        auto tryEmplace() -> void
        {
        }

        auto push() -> void
        {
        }

        auto tryPush() -> void
        {
        }

        auto front() noexcept -> const T &
        {
            static T t;
            return t;
        }

        auto pop() noexcept -> void
        {
        }

        auto size() const noexcept -> size_t
        {
            return 0;
        }

        auto empty() const noexcept -> bool
        {
            return true;
        }

        auto capacity() const noexcept -> size_t
        {
            return 0;
        }
    };
};

#endif // INCLUDE_SPSC_QUEUE_H