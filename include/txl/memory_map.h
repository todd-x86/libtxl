#pragma once

#include <txl/buffer_ref.h>
#include <txl/handle_error.h>
#include <txl/system_error.h>
#include <txl/result.h>

#include <algorithm>
#include <optional>
#include <cstddef>

#include <sys/mman.h>
#include <linux/mman.h>

namespace txl
{
    class memory_map
    {
    private:
        void * map_ = nullptr;
        size_t size_ = 0;
    public:
        enum protection_flags : int
        {
            none = PROT_NONE,
            read = PROT_READ,
            write = PROT_WRITE,
            execute = PROT_EXEC
        };

        enum open_flags : int
        {
            lower_2gb = MAP_32BIT,
            anonymous = MAP_ANONYMOUS,
            fixed_or_replace = MAP_FIXED,
            fixed_or_fail = MAP_FIXED_NOREPLACE,
            stack = MAP_GROWSDOWN | MAP_STACK,
            huge_tlb = MAP_HUGETLB,
            huge_tlb_2mb = MAP_HUGE_2MB,
            huge_tlb_4mb = (22 << MAP_HUGE_SHIFT),
            huge_tlb_8mb = (23 << MAP_HUGE_SHIFT),
            huge_tlb_16mb = (24 << MAP_HUGE_SHIFT),
            huge_tlb_32mb = (25 << MAP_HUGE_SHIFT),
            huge_tlb_64mb = (26 << MAP_HUGE_SHIFT),
            huge_tlb_128mb = (27 << MAP_HUGE_SHIFT),
            huge_tlb_256mb = (28 << MAP_HUGE_SHIFT),
            huge_tlb_512mb = (29 << MAP_HUGE_SHIFT),
            huge_tlb_1gb = MAP_HUGE_1GB,
            locked = MAP_LOCKED,
            no_swap = MAP_NORESERVE,
            populate = MAP_POPULATE,
            populate_async = MAP_POPULATE | MAP_NONBLOCK,
            synchronized = MAP_SYNC,
            uninitialized = MAP_UNINITIALIZED,
        };

        memory_map() = default;
        memory_map(memory_map const &) = delete;
        memory_map(memory_map && m)
            : memory_map()
        {
            std::swap(map_, m.map_);
            std::swap(size_, m.size_);
        }

        virtual ~memory_map()
        {
            if (is_open())
            {
                // Ignore result
                close();
            }
        }
        
        auto operator=(memory_map const &) -> memory_map & = delete;
        auto operator=(memory_map && m) -> memory_map &
        {
            if (&m != this)
            {
                std::swap(map_, m.map_);
                std::swap(size_, m.size_);
            }
            return *this;
        }

        auto open(size_t size, protection_flags p_flags, bool shared = false, open_flags o_flags = static_cast<open_flags>(static_cast<int>(anonymous) | static_cast<int>(uninitialized)), std::optional<void *> addr_hint = std::nullopt, std::optional<int> fd = std::nullopt, std::optional<off_t> offset = std::nullopt) -> result<void>
        {
            if (is_open())
            {
                return {get_system_error(EBUSY)};
            }

            auto private_or_shared = shared ? MAP_SHARED : MAP_PRIVATE;
            map_ = ::mmap(addr_hint.value_or(nullptr), size, static_cast<int>(p_flags), private_or_shared | static_cast<int>(o_flags), fd.value_or(-1), offset.value_or(0));
            if (map_ == MAP_FAILED)
            {
                return {get_system_error()};
            }
            size_ = size;

            return {};
        }

        auto is_open() const -> bool { return map_ != nullptr and size_ != 0; }

        auto close() -> result<void>
        {
            auto res = handle_system_error(::munmap(map_, size_));
            if (res)
            {
                map_ = nullptr;
                size_ = 0;
            }
            return res;
        }

        auto data() const -> void const * { return map_; }
        auto data() -> void * { return map_; }
        auto size() const -> size_t { return size_; }

        auto memory() const -> buffer_ref
        {
            return buffer_ref{data(), size()};
        }
        
        auto sync(buffer_ref mem, bool async = false, bool invalidate = false) -> result<void>
        {
            int flags = (invalidate ? MS_INVALIDATE : 0);
            flags |= (async ? MS_ASYNC : MS_SYNC);
            auto res = ::msync(mem.data(), mem.size(), flags);
            return handle_system_error(res);
        }
    };

    inline auto operator|(txl::memory_map::protection_flags x, txl::memory_map::protection_flags y) -> txl::memory_map::protection_flags
    {
        return static_cast<txl::memory_map::protection_flags>(static_cast<int>(x) | static_cast<int>(y));
    }
    
    inline auto operator|(txl::memory_map::open_flags x, txl::memory_map::open_flags y) -> txl::memory_map::open_flags
    {
        return static_cast<txl::memory_map::open_flags>(static_cast<int>(x) | static_cast<int>(y));
    }
}
