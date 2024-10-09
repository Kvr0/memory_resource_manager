#ifndef _MRM_NEW_ALLOCATE_HPP_
#define _MRM_NEW_ALLOCATE_HPP_

#include <mrm/interface.hpp>
#include <limits>

namespace mrm{

    class heap_manager;

    class heap : public resource_i<heap>{
    public:
        using _Mybase = resource_i<heap>;

        heap() = default;
    protected:
        using _Mybase::_Mybase;
        friend class heap_manager;
    };

    class heap_manager : public heap::manager_t{
    public:
        heap_manager() = default;

        /// @brief Get the maximum capacity of the manager
        /// @return `mrm::usize_t` The maximum capacity of the manager
        usize_t capacity() const override {
            return std::numeric_limits<usize_t>::max();
        }

        /// @brief Allocate memory
        /// @param size The size of memory to allocate
        /// @return `mrm::heap` The allocated memory
        resource_t allocate(usize_t size, usize_t offset = 0) override {
            return resource_t{::operator new(size), size, this};
        }

        /// @brief Deallocate memory
        /// @param resource The memory to deallocate
        /// @note `resource` must be allocated by this manager
        /// @return `bool` `true` if the memory is deallocated, `false` otherwise
        bool deallocate(resource_i& resource) override {
            if(resource.manager() != this) return false;
            ::operator delete(resource.data(), resource.size()); // deallocate memory
            detach_resource(resource); // remove from resources_
            _clear_resource(resource); // clear resource
            return true;
        }
    };

}

#endif // _MRM_NEW_ALLOCATE_HPP_
