#ifndef _MRM_FILE_HPP_
#define _MRM_FILE_HPP_

#if defined(_WIN32) || defined(_WIN64)

#include <mrm/interface.hpp>
#define NOMINMAX
#include <windows.h>

namespace mrm {

    class fileview_manager;

    /// @brief A fileview resource
    class fileview : public resource_i<fileview> {
    public:
        using _Mybase = resource_i<fileview>;

        fileview() = default;
    protected:
        using _Mybase::_Mybase;
        friend class fileview_manager;
    };

    /// @brief A memory mapped file manager
    class fileview_manager : public fileview::manager_t {
    public:
        fileview_manager() = default;
        virtual ~fileview_manager(){
            close();
        }

        // movable
        fileview_manager(fileview_manager &&other) noexcept
            : fileview::manager_t(std::move(other)){
            hFile_ = std::exchange(other.hFile_, INVALID_HANDLE_VALUE);
            hMap_ = std::exchange(other.hMap_, (HANDLE)NULL);
            capacity_ = std::exchange(other.capacity_, 0);
        }
        fileview_manager &operator=(fileview_manager &&other) noexcept{
            if(this != &other){
                fileview::manager_t::operator=(std::move(other));
                hFile_ = std::exchange(other.hFile_, INVALID_HANDLE_VALUE);
                hMap_ = std::exchange(other.hMap_, (HANDLE)NULL);
                capacity_ = std::exchange(other.capacity_, 0);
            }
            return *this;
        }

        /// @brief Open a file
        /// @param filename The name of the file
        /// @param file_capacity The capacity of the file
        /// @return `bool` `true` if the file is opened, `false` otherwise
        /// @note If the file does not exist, it will be created
        /// @note If the file exists, the capacity of the file will be extended to `file_capacity`
        bool open_file(const char* filename, LONG file_capacity = 0){
            // close previous file
            close();

            // open file
            hFile_ = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if(hFile_ == INVALID_HANDLE_VALUE) return false;

            // get file size
            DWORD high;
            DWORD low = GetFileSize(hFile_, &high);
            auto file_size = static_cast<usize_t>(high) << 32 | low;

            // extend file size
            if(file_capacity > file_size){
                SetFilePointer(hFile_, file_capacity, nullptr, FILE_BEGIN);
                SetEndOfFile(hFile_);
                file_size = file_capacity;
            }

            // create file mapping
            hMap_ = CreateFileMappingA(hFile_, nullptr, PAGE_READWRITE, 0, 0, nullptr);
            if(hMap_ == NULL){
                CloseHandle(hFile_);
                hFile_ = NULL;
                return false;
            }

            capacity_ = file_size;
            return true;
        }

        /// @brief Open a file mapping
        /// @param name The name of the file mapping
        /// @param capacity The capacity of the file mapping
        bool open(const char* name, usize_t capacity){
            // close previous file mapping
            close();

            // create file mapping
            DWORD high = static_cast<DWORD>(capacity >> 32);
            DWORD low = static_cast<DWORD>(capacity & 0xFFFFFFFF);
            hMap_ = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, high, low, nullptr);
            
            // check if file mapping is created
            if(hMap_ == NULL){
                CloseHandle(hFile_);
                hFile_ = NULL;
                return false;
            }

            capacity_ = capacity;
            return true;
        }

        /// @brief Close the file mapping
        void close(){
            if(hMap_ != NULL){
                CloseHandle(hMap_);
                hMap_ = NULL;
            }
            if(hFile_ != INVALID_HANDLE_VALUE){
                CloseHandle(hFile_);
                hFile_ = INVALID_HANDLE_VALUE;
            }
            capacity_ = 0;
        }

        /// @brief Check if the manager is open
        /// @return `bool` `true` if the manager is open, `false` otherwise
        bool is_open() const {
            return hMap_ != NULL;
        }

        /// @brief Get the capacity of the manager
        /// @return `mrm::usize_t` The maximum capacity of the manager
        usize_t capacity() const override {
            return capacity_;
        }

        /// @brief Allocate fileview
        /// @param size The size of fileview to allocate
        /// @return `mrm::file` The allocated memory
        /// @note size must be less than or equal to the capacity of the manager and less than or equal to `std::numeric_limits<DWORD>::max()`
        resource_t allocate(usize_t size, usize_t offset = 0) override {
            // check if the manager is open and the size is valid
            if(hMap_ == NULL || offset + size > capacity_ || size > std::numeric_limits<DWORD>::max()) return resource_t{};
            
            // map view of file
            auto low = static_cast<DWORD>(offset & 0xFFFFFFFF);
            auto high = static_cast<DWORD>(offset >> 32);
            LPVOID data = MapViewOfFile(hMap_, FILE_MAP_ALL_ACCESS, high, low, size);

            if(data == NULL) return resource_t{};
            return resource_t{data, size, this};
        }

        /// @brief Deallocate fileview
        /// @param resource The fileview to deallocate
        /// @note `resource` must be allocated by this manager
        /// @return `bool` `true` if the memory is deallocated, `false` otherwise
        bool deallocate(resource_i &resource) override {
            if(resource.manager() != this || hMap_ == NULL) return false;
            UnmapViewOfFile(resource.data()); // unmap view of file
            detach_resource(resource); // remove from resources_
            _clear_resource(resource); // clear resource
            return true;
        }
    protected:
        HANDLE hFile_ = INVALID_HANDLE_VALUE;
        HANDLE hMap_ = NULL;
        usize_t capacity_ = 0;
    };

} // namespace mrm

#endif // defined(_WIN32) || defined(_WIN64)
#endif // _MRM_FILE_HPP_
