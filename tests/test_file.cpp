#include <iostream>

#include <mrm/file.hpp>

int main() {

    mrm::fileview_manager manager;
    manager.open_file("test.dat", sizeof(int) * 30);

    {
        mrm::fileview file = manager.allocate(sizeof(int) * 10);

        int data[10] = {0};
        if(file.read_arr(data)) {
            std::cout << "Read data successfully" << std::endl;
            for(int i = 0; i < 10; ++i)
                std::cout << data[i] << ' ';
            std::cout << std::endl;
        } else {
            std::cout << "Read data failed" << std::endl;
        }

        for (int i = 0; i < 10; ++i) {
            data[i] += 1;
        }

        if(file.write_arr(data)) {
            std::cout << "Write data successfully" << std::endl;
        } else {
            std::cout << "Write data failed" << std::endl;
        }
    }

    return 0;
}
