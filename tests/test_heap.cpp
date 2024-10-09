#include <iostream>

#include <mrm/heap.hpp>
using namespace mrm;

void print_array(const int* x, usize_t n) {
    usize_t i=0;
    while(i<n){
        std::cout << x[i];
        if(i%10==9) std::cout << "\n";
        else std::cout << " ";
        i++;
    }
    std::cout << "\n";
}

int main() {

    heap_manager manager;
    {
        heap h1 = manager.allocate(sizeof(int) * 100);

        h1.sub(sizeof(int)*5, sizeof(int) * 10).write_arr<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});

        print_array((int*)h1.data(), 100);
    }

    return 0;
}
