# TXL
## Todd's eXtension Library for C++

# #include <txl/action.h>

An experimental continuation library for chaining lambdas and capturing state.

# #include <txl/app.h>

Utility function for retrieving the running application's path as a `std::filesystem::path`.

# #include <txl/backoff.h>

Simple backoff mechanism that invokes a custom "sleep" function.

# #include <txl/bitwise.h>

Simple bitwise manipulation utilities.

# #include <txl/buffer_ref.h>

`std::string_view`-inspired view into a contiguous buffer of raw memory.

# #include <txl/copy.h>

Memory copy utilities that work with the `txl::reader` and `txl::writer` patterns.

# #include <txl/csv.h>

CSV row reader and splitter.

# #include <txl/event_poller.h>

epoll-backed generic event poller.

# #include <txl/event_timer.h>

Timer event that works in conjunction with a `txl::event_poller`.

# #include <txl/file_base.h>

Internal base class for file-descriptor-based I/O classes.

# #include <txl/file.h>

File class for reading/writing to a file system.

# #include <txl/fixed_string.h>
# #include <txl/flat_map.h>
# #include <txl/handle_error.h>
# #include <txl/io.h>
# #include <txl/io_reactor.h>
# #include <txl/is_one_of.h>
# #include <txl/iterators.h>
# #include <txl/iterator_view.h>
# #include <txl/make_unique.h>
# #include <txl/memory_map.h>
# #include <txl/memory_pool.h>
# #include <txl/object.h>
# #include <txl/opaque_ptr.h>
# #include <txl/overload.h>
# #include <txl/patterns.h>
# #include <txl/pipe.h>
# #include <txl/read_string.h>
# #include <txl/ref.h>
# #include <txl/resource_pool.h>
# #include <txl/result.h>
# #include <txl/ring_buffer_file.h>
# #include <txl/ring_buffer.h>
# #include <txl/size_policy.h>
# #include <txl/socket_address.h>
# #include <txl/socket.h>
# #include <txl/storage_union.h>
# #include <txl/stream_writer.h>
# #include <txl/system_error.h>
# #include <txl/threading.h>
# #include <txl/time.h>
# #include <txl/type_info.h>
# #include <txl/types.h>
# #include <txl/unit_test.h>
# #include <txl/virtual_ptr.h>
