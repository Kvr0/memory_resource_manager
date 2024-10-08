#include <iostream>

#include <mrm/heap.hpp>
using namespace mrm;

int main() {

    heap_manager manager;
    {
        heap h1 = manager.allocate(10);

        std::cout << manager.size() << std::endl;
        for (auto &h : manager)
            std::cout << h->size() << std::endl;
    }
    std::cout << manager.size() << std::endl;

    return 0;
}
