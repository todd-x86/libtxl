#pragma once

#include <mutex>
#include <shared_mutex>

#define _TXL_SHARED_LOCK_NAME(line) _shared_lock_line_ ## line
#define _TXL_SHARED_LOCK(line, m) std::shared_lock<std::shared_mutex> _TXL_SHARED_LOCK_NAME(line)(m)
#define SHARED_LOCK(m) _TXL_SHARED_LOCK(__LINE__, m)

#define _TXL_UNIQUE_LOCK_NAME(line) _unique_lock_line_ ## line
#define _TXL_UNIQUE_LOCK(line, m) std::unique_lock<std::shared_mutex> _TXL_UNIQUE_LOCK_NAME(line)(m)
#define UNIQUE_LOCK(m) _TXL_UNIQUE_LOCK(__LINE__, m)
