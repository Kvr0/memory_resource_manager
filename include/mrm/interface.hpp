#ifndef _MRM_INTERFACE_HPP_
#define _MRM_INTERFACE_HPP_

#include <cstring>
#include <set>

namespace mrm {

using ptr_t = void *;
using cptr_t = const void *;
using usize_t = unsigned long long;

/// @brief A constant view of memory
class mem_cview {
public:
    mem_cview() = default;
    mem_cview(cptr_t data, usize_t size) : data_(data), size_(size) {}

    usize_t size() const noexcept { return size_; }
    cptr_t data() const noexcept { return data_; }

    bool operator==(const mem_cview &other) const noexcept {
        return size_ == other.size_ && data_ == other.data_;
    }
    bool operator!=(const mem_cview &other) const noexcept {
        return !(*this == other);
    }

    mem_cview sub(usize_t offset, usize_t size) const noexcept {
        if(offset + size > size_)
            return {};
        return {static_cast<const char *>(data()) + offset, size};
    }

    bool read(void* dst, usize_t size, usize_t offset = 0) const {
        if(offset + size > size_)
            return false;
        std::memcpy(dst, static_cast<const char *>(data()) + offset,
                    size);
        return true;
    }

protected:
    cptr_t data_ = nullptr;
    usize_t size_ = 0;
};

/// @brief A mutable view of memory
class mem_view : public mem_cview {
public:
    mem_view() = default;
    mem_view(ptr_t data, usize_t size) : mem_cview(data, size) {}

    ptr_t data() noexcept { return const_cast<ptr_t>(mem_cview::data()); }

    mem_cview cview() const noexcept { return *this; }

    mem_view sub(usize_t offset, usize_t size) noexcept {
        auto cview = mem_cview::sub(offset, size);
        return {const_cast<ptr_t>(cview.data()), cview.size()};
    }

    bool write(const void* src, usize_t size, usize_t offset = 0) {
        if(offset + size > size_)
            return false;
        std::memcpy(static_cast<char *>(data()) + offset, src,
                    size);
        return true;
    }
};

template <typename _Resource>
class resource_i;

/// @brief A resource manager interface
template <typename _Resource>
class resource_manager_i {
public:
    using resource_t = _Resource;
    using resource_i = resource_i<resource_t>;
    static_assert(std::is_base_of_v<resource_i, resource_t>,
                  "resource_t must be derived from resource_i");

    resource_manager_i() = default;
    virtual ~resource_manager_i() = default;

    // uncopyable
    resource_manager_i(const resource_manager_i &) = delete;
    resource_manager_i &operator=(const resource_manager_i &) = delete;

    // movable
    resource_manager_i(resource_manager_i &&) noexcept = default;
    resource_manager_i &operator=(resource_manager_i &&) noexcept = default;

    /// @brief Get the size of allocated memory
    /// @return `mrm::usize_t` The size of allocated memory
    usize_t size() const noexcept { return resources_.size(); }

    /// @brief Get the used size of the manager
    /// @return `mrm::usize_t` The used size of the manager
    usize_t used_size() const noexcept { return used_size_; }

    auto begin() const noexcept { return resources_.begin(); }
    auto end() const noexcept { return resources_.end(); }

    /// @brief Get the maximum capacity of the manager
    /// @return `mrm::usize_t` The maximum capacity of the manager
    virtual usize_t capacity() const = 0;

    /// @brief Allocate memory
    /// @param size The size of memory to allocate
    /// @return `resource_t` The allocated memory
    virtual resource_t allocate(usize_t size) = 0;

    /// @brief Deallocate memory
    /// @param resource The memory to deallocate
    /// @note `resource` must be allocated by this manager
    /// @return `bool` `true` if the memory is deallocated, `false` otherwise
    virtual bool deallocate(resource_i &resource) = 0;

    /// @brief Deallocate all memory
    virtual void deallocate_all() {
        for(auto &resource : resources_)
            deallocate(*resource);
    }

    /// @brief Detach resource
    /// @param resource The resource to detach
    /// @note `resource` must be allocated by this manager
    /// @return `bool` `true` if the resource is detached, `false` otherwise
    virtual bool detach_resource(resource_i &resource) {
        if(resource.manager_ != this)
            return false;
        resources_.erase(&resource);
        used_size_ -= resource.size();
        resource.manager_ = nullptr;
        return true;
    }

    /// @brief Add resource
    /// @param resource The resource to add
    /// @note `resource.manager()` must be `nullptr`
    virtual bool add_resource(resource_i &resource) {
        if(resource.manager())
            return false;
        resources_.insert(&resource);
        used_size_ += resource.size();
        resource.manager_ = this;
        return true;
    }

    /// @brief Swap two resources
    /// @param a The first resource
    /// @param b The second resource
    /// @note `a` and `b` must be allocated by this manager
    /// @return `bool` `true` if the resources are swapped, `false` otherwise
    virtual bool swap_resource(resource_i &a, resource_i &b) {
        if(&a == &b)
            return false;
        if(a.manager() != this || b.manager() != this)
            return false;
        std::swap(a.data_, b.data_);
        std::swap(a.size_, b.size_);
        return true;
    }

protected:
    usize_t used_size_ = 0;
    std::set<resource_i *> resources_;

    virtual void _clear_resource(resource_i &resource) {
        resource._clear();
    }
};

/// @brief A resource interface
template <typename _Resource>
class resource_i : public mem_view {
public:
    using manager_t = resource_manager_i<_Resource>;

    resource_i() = default;

    virtual ~resource_i() {
        if(valid())
            release();
    }

    // uncopyable
    resource_i(const resource_i &) = delete;
    resource_i &operator=(const resource_i &) = delete;

    // movable
    resource_i(resource_i &&other) noexcept
        : resource_i(nullptr, 0, other.manager()) {
        if(manager_) {
            manager_->swap_resource(other, *this);
            other.release();
        }
    }
    resource_i &operator=(resource_i &&) noexcept {
        if(this != &other) {
            if(other.manager()) {
                other.manager()->swap_resource(other, *this); // swap resources
                other.release();                              // release other
            } else {
                release(); // release this if other is invalid
            }
        }
        return *this;
    }

    manager_t *manager() const noexcept { return manager_; }

    bool valid() const noexcept { return manager_ != nullptr; }

    virtual void release() {
        if(manager()) {
            manager()->deallocate(*this); // deallocate memory by manager
        }
    }

protected:
    resource_i(ptr_t data, usize_t size, manager_t *manager)
        : mem_view(data, size) {
        if(manager)
            manager->add_resource(*this);
    }
    friend class manager_t;

    void _clear(){
        data_ = nullptr;
        size_ = 0;
        manager_ = nullptr;
    }

    manager_t *manager_ = nullptr;
};

} // namespace mrm

#endif // _MRM_INTERFACE_HPP_
