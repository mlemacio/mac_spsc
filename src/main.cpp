#include <iostream>

#include "include/spscQueue.h"

auto main() -> int
{
    {
        static constexpr size_t CAPACITY = 10;

        spsc::spscQueue_t<std::vector<int>> q(CAPACITY);

        for (size_t i{}; i < 3; i++)
        {
            q.emplace(std::vector<int>{1, 2, 3});
        }

        std::cout << std::boolalpha;

        auto maybeVal = q.front();
        while (maybeVal.has_value())
        {
            for (const auto &v : maybeVal.value().get())
            {
                std::cout << v << std::endl;
            }
            q.pop();
            maybeVal = q.front();
        }
    }

    return 0;
}