#pragma once

// `unsafe_storage` is essentially a `std::optional`-like replacement that shaves off the extra bytes incurred from storing a flag to represent whether a value is contained within it

#include <txl/buffer_ref.h>

#include <array>
#include <algorithm>

namespace txl
{
    namespace detail
    {
        /**
         * Base container for unsafe storage.
         *
         * \tparam Value storage value type
         */
        template<class Value>
        class storage_base
        {
        private:
            /**
             * Underlying value stored as a series of bytes 
             */
            alignas(Value) std::array<std::byte, sizeof(Value)> val_;
        protected:
            /**
             * Casts the underlying storage value as a buffer_ref.
             *
             * \return buffer_ref to underlying storage
             */
            auto to_buffer_ref() -> buffer_ref { return {val_}; }
            /**
             * Casts the underlying storage value as a buffer_ref.
             *
             * \return buffer_ref to underlying storage
             */
            auto to_buffer_ref() const -> buffer_ref { return {val_}; }
            /**
             * Determines if the underlying storage is completely zeroed out 
             *
             * \tparam 
             * \param 
             * \return 
             */
            auto is_zero() const -> bool { return to_buffer_ref().is_zero(); }
            /**
             * Zeros out the underlying storage 
             *
             * \tparam 
             * \param 
             * \return 
             */
            auto fill_zero() -> void { to_buffer_ref().fill(std::byte{0}); }

            /**
             * Retrieves a typed pointer to the underlying storage
             *
             * \tparam 
             * \param 
             * \return 
             */
            auto ptr() -> Value * { return reinterpret_cast<Value *>(&val_[0]); }
            /**
             * Retrieves a typed value to the underlying storage 
             *
             * \tparam 
             * \param 
             * \return 
             */
            auto val() -> Value & { return *ptr(); }
            /**
             * Retrieves a typed pointer to the underlying storage
             *
             * \tparam 
             * \param 
             * \return 
             */
            auto ptr() const -> Value const * { return reinterpret_cast<Value const *>(&val_[0]); }
            /**
             * Retrieves a typed value to the underlying storage 
             *
             * \tparam 
             * \param 
             * \return 
             */
            auto val() const -> Value const & { return *ptr(); }
        public:
            /**
             * Indirection operator which returns a typed reference to the underlying storage.
             *
             * \tparam 
             * \param 
             * \return 
             */
            auto operator*() -> Value & { return val(); }
            /**
             * Indirection operator which returns a typed reference to the underlying storage.
             *
             * \tparam 
             * \param 
             * \return 
             */
            auto operator*() const -> Value const & { return val(); }
            /**
             * Structure dereference operator which returns a typed pointer to the underlying storage.
             *
             * \tparam 
             * \param 
             * \return 
             */
            auto operator->() -> Value * { return ptr(); }
            /**
             * Structure dereference operator which returns a typed pointer to the underlying storage.
             *
             * \tparam 
             * \param 
             * \return 
             */
            auto operator->() const -> Value const * { return ptr(); }
        };
    }

    /**
     * Represents a typed value as a series of bytes. The purpose of this container is to provide a `std::optional<T>`-like container for typed values with managing memory being the responsibility of the class that owns the `unsafe_storage<T>`. This helps eliminate the overhead from storing extra bytes to represent whether a value is contained within the underlying storage by making it the responsibility of the class that owns it.
     *
     * \tparam 
     * \param 
     * \return 
     */
    template<class Value>
    struct unsafe_storage : detail::storage_base<Value>
    {
        /**
         *
         *
         * \tparam 
         * \param 
         * \return 
         */
        unsafe_storage() = default;

        /**
         *
         *
         * \tparam 
         * \param 
         * \return 
         */
        unsafe_storage(unsafe_storage const & s)
        {
            new (this->ptr()) Value( s.val() );
        }

        /**
         *
         *
         * \tparam 
         * \param 
         * \return 
         */
        unsafe_storage(unsafe_storage && s)
        {
            new (this->ptr()) Value(std::move(s.val()));
        }
        
        /**
         *
         *
         * \tparam 
         * \param 
         * \return 
         */
        template<class... Args>
        auto emplace(Args && ... args) -> void
        {
            new (this->ptr()) Value(std::forward<Args>(args)...);
        }

        /**
         *
         *
         * \tparam 
         * \param 
         * \return 
         */
        auto erase() -> void
        {
            this->val().~Value();
        }

        /**
         *
         *
         * \tparam 
         * \param 
         * \return 
         */
        auto operator=(unsafe_storage const & s) -> unsafe_storage &
        {
            if (&s != this)
            {
                new (this->ptr()) Value(s.val());
            }
            return *this;
        }

        /**
         *
         *
         * \tparam 
         * \param 
         * \return 
         */
        auto operator=(unsafe_storage && s) -> unsafe_storage &
        {
            if (&s != this)
            {
                new (this->ptr()) Value(std::move(s.val()));
            }
            return *this;
        }
    };

    /**
     *
     *
     * \tparam 
     * \param 
     * \return 
     */
    template<class Value, size_t Size>
    using unsafe_storage_array = std::array<unsafe_storage<Value>, Size>;
}
