#include <iostream>

#include "include/spscQueue.h"

auto main() -> int
{
    static constexpr size_t CAPACITY = 10;

    spsc::spscQueue_t<int> q(CAPACITY);

    q.emplace(12);

    std::cout << q.front().value() << std::endl;

    return 0;
}