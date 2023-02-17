#pragma once

#include "types.h"
#include <mutex>
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace txl
{
    namespace detail
    {
        // Memory chunk (rented)
        static constexpr uint16_t MEM_CHUNK = 1;
        // Free chunk (used, available for rent)
        static constexpr uint16_t FREE_CHUNK = 2;

        // "No chunk" identifier
        static constexpr uint32_t NO_CHUNK = 0xFFFFFFFF;

        // Memory pool chunk administrative header
        struct mempool_chunk_header
        {
            union
            {
                // Free chunk: next chunk identifier (singly-linked list)
                uint32_t next_chunk;
                // Memory chunk: number of concurrent handles open to chunk
                uint32_t num_references;
            };
            // Number of pages allocated in chunk
            uint32_t num_pages;
            // Chunk type (see constants above)
            uint16_t type;
        };
    }

    // Forward declarations
    class memory_pool_handle;
    class memory_pool_chunk;

    /**
     * Memory pool implementation with chunks allocated by fixed-size pages
     * and a LIFO-based free list.
     *
     * Layout:
     *
     *      Page
     *       |
     *   ----+----          +----------------+
     *  {         }         |                v
     *   +-+------+-+------+|+--------------+-+------+--------------------
     *   |H|-mem--|H|-mem--|*|-free---------|*|-free-| - - - - - - - - - -
     *   +-+------+-+------+-+--------------+-+------+--------------------
     *                      ^                        ^
     * Free List: ----------+                        |
     * Next Alloc: ----------------------------------+
     */
    class memory_pool
    {
        friend class memory_pool_chunk;
    private:
        // Allocation mutex
        std::mutex mutex_;
        // Raw memory pool
        byte_vector data_;
        // Number of pages allocated in memory pool
        size_t num_pages_;
        // Number of bytes per page
        size_t bytes_per_page_;
        // Allocation region
        void * next_alloc_;
        // Free list page identifier
        uint32_t free_list_ = detail::NO_CHUNK;

        detail::mempool_chunk_header * get_page(uint32_t id)
        {
            return reinterpret_cast<detail::mempool_chunk_header *>(reinterpret_cast<uint8_t *>(data_.data()) + (bytes_per_page_ * id));
        }

        void * find_free_list_chunk(size_t size_pages)
        {
            uint32_t * p_id = &free_list_;
            while (*p_id != detail::NO_CHUNK)
            {
                auto p = get_page(*p_id);
                if (p->num_pages >= size_pages)
                {
                    return reinterpret_cast<void *>(p);
                }
                p_id = &p->next_chunk;
            }

            return nullptr;
        }

        uint32_t page_id(void const * p) const
        {
            return (reinterpret_cast<uint8_t const *>(p) - reinterpret_cast<uint8_t const *>(data_.data())) / bytes_per_page_;
        }

        bool dec_ref(void * p)
        {
            auto h = reinterpret_cast<detail::mempool_chunk_header *>(p)-1;
            if (h->type != detail::MEM_CHUNK)
            {
                return false;
            }
            
            std::unique_lock<std::mutex> lock(mutex_);
            
            h->num_references--;
            if (h->num_references == 0)
            {
                h->type = detail::FREE_CHUNK;
                h->next_chunk = free_list_;
                free_list_ = page_id(reinterpret_cast<void const *>(h));
            }
            return true;
        }

        bool inc_ref(void * p)
        {
            auto h = reinterpret_cast<detail::mempool_chunk_header *>(p)-1;
            if (h->type != detail::MEM_CHUNK)
            {
                return false;
            }
            
            std::unique_lock<std::mutex> lock(mutex_);
            
            h->num_references++;
            return true;
        }

        void const * end_ptr() const
        {
            return reinterpret_cast<void const *>(reinterpret_cast<uint8_t const *>(data_.data()) + (num_pages_ * bytes_per_page_));
        }

        void const * next_alloc(size_t num_pages) const
        {
            return reinterpret_cast<void const *>(reinterpret_cast<uint8_t const *>(next_alloc_) + (bytes_per_page_ * num_pages));
        }

        void * get_free_chunk(size_t num_bytes)
        {
            std::unique_lock<std::mutex> lock(mutex_);

            // Find a suitable chunk in the free list
            auto num_pages = static_cast<uint32_t>(std::ceil((float)(num_bytes+sizeof(detail::mempool_chunk_header)) / bytes_per_page_));
            auto c = find_free_list_chunk(num_pages);
            if (c)
            {
                auto h = reinterpret_cast<detail::mempool_chunk_header *>(c);
                h->type = detail::MEM_CHUNK;
                h->num_references = 1;
                return reinterpret_cast<void *>(h + 1);
            }

            if (next_alloc(num_pages) > end_ptr())
            {
                // Out of memory
                return nullptr;
            }

            auto h = reinterpret_cast<detail::mempool_chunk_header *>(next_alloc_);
            h->type = detail::MEM_CHUNK;
            h->num_pages = num_pages;
            h->num_references = 1;
            next_alloc_ = reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(next_alloc_) + (bytes_per_page_ * num_pages));
            return reinterpret_cast<void *>(h + 1);
        }
    public:
        memory_pool(size_t num_pages, size_t bytes_per_page)
            : data_(num_pages * bytes_per_page)
            , num_pages_(num_pages)
            , bytes_per_page_(bytes_per_page)
            , next_alloc_(data_.data())
        {
        }

        memory_pool(memory_pool const &) = delete;
        memory_pool(memory_pool &&) = delete;  // TODO: can we std::move() safely?

        memory_pool & operator=(memory_pool const &) = delete;
        memory_pool & operator=(memory_pool &&) = delete;

        inline memory_pool_chunk allocate(size_t num_bytes);
        inline memory_pool_handle rent(size_t num_bytes);
        inline void release(memory_pool_handle & h);
    };

    class memory_pool_handle final : detail::memory_pool_handle_base
    {
        friend class memory_pool;
    private:
        void * data_ = nullptr;
        size_t size_ = 0;

        memory_pool_handle(void * data, size_t size)
            : data_(data)
            , size_(size)
        {
        }
    public:
        memory_pool_handle() = default;
        memory_pool_handle(memory_pool_handle const & p) = delete;
        memory_pool_handle(memory_pool_handle && p)
            : memory_pool_handle()
        {
            std::swap(data_, p.data_);
            std::swap(size_, p.size_);
        }

        memory_pool_handle & operator=(memory_pool_handle const & h) = delete;

        memory_pool_handle & operator=(memory_pool_handle && h)
        {
            if (this != &h)
            {
                std::swap(data_, h.data_);
                std::swap(size_, h.size_);
            }
            return *this;
        }

        bool empty() const { return data_ == nullptr && size_ == 0; }

        void * data() { return data_; }
        void const * data() const { return data_; }
        size_t size() const { return size_; }
    };
    
    class memory_pool_chunk final
    {
        friend class memory_pool;
    private:
        memory_pool * pool_ = nullptr;
        void * data_ = nullptr;
        size_t size_ = 0;

        memory_pool_chunk(memory_pool * pool, void * data, size_t size)
            : pool_(pool)
            , data_(data)
            , size_(size)
        {
        }
    public:
        memory_pool_chunk() = default;
        memory_pool_chunk(memory_pool_chunk const & p)
            : memory_pool_chunk(p.pool_, p.data_, p.size_)
        {
            if (pool_)
            {
                pool_->inc_ref(data_);
            }
        }
        memory_pool_chunk(memory_pool_chunk && p)
            : memory_pool_chunk()
        {
            std::swap(pool_, p.pool_);
            std::swap(data_, p.data_);
            std::swap(size_, p.size_);
        }
        ~memory_pool_chunk()
        {
            close();
        }
        bool empty() const { return pool_ == nullptr && data_ == nullptr && size_ == 0; }
        void close()
        {
            if (pool_)
            {
                pool_->dec_ref(data_);
                pool_ = nullptr;
                data_ = nullptr;
                size_ = 0;
            }
        }
        void * data() { return data_; }
        void const * data() const { return data_; }
        size_t size() const { return size_; }
    };
    
    memory_pool_handle memory_pool::rent(size_t num_bytes)
    {
        auto h = get_free_chunk(num_bytes);
        if (h != nullptr)
        {
            return memory_pool_handle(h, num_bytes);
        }
        else
        {
            return memory_pool_handle();
        }
    }

    memory_pool_chunk memory_pool::allocate(size_t num_bytes)
    {
        auto h = get_free_chunk(num_bytes);
        if (h != nullptr)
        {
            return memory_pool_chunk(this, h, num_bytes);
        }
        else
        {
            return memory_pool_chunk();
        }
    }

    void memory_pool::release(memory_pool_handle & h)
    {
        if (!h.empty())
        {
            dec_ref(h.data_);
            h.data_ = nullptr;
            h.size_ = 0;
        }
    }
}
