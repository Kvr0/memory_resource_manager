#include <iostream>

#include <mrm/heap.hpp>
using namespace mrm;

int main() {

    heap_manager manager;
    {
        heap h1 = manager.allocate(sizeof(int) * 10);

        {
            for (int i = 0; i < 10; i++)
                ((int *)h1.data())[i] = i;
        }

        {
            int a = 1234;
            if(h1.write(a))
                std::cout << "a= " << a << std::endl;
        }

        {
            int b;
            if(h1.read(b))
                std::cout << "b= " << b << std::endl;
        }

        for(usize_t i = 0; i < 10; i++)
            std::cout << i << "=" << ((int *)h1.data())[i] << std::endl;

        // todo test read_arr/write_arr
        {
            int c[3] = {1, 2, 3};
            if(h1.write_arr(c, 1, 0))
                std::cout << "c= " << c[0] << ", " << c[1] << ", " << c[2]
                          << std::endl;
            else
                std::cout << "write_arr failed" << std::endl;
        }

        {
            int d[3] = {};
            if(h1.read_arr(d, 1, 0))
                std::cout << "d= " << d[0] << ", " << d[1] << ", " << d[2]
                          << std::endl;
            else
                std::cout << "read_arr failed" << std::endl;
        }

        for(usize_t i = 0; i < 10; i++)
            std::cout << i << "=" << ((int *)h1.data())[i] << std::endl;

        std::cout << manager.size() << std::endl;
        for(auto &h : manager)
            std::cout << h->size() << std::endl;
    }
    std::cout << manager.size() << std::endl;

    return 0;
}
