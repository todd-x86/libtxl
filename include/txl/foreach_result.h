#pragma once

#include <txl/result.h>
#include <txl/storage.h>

#include <functional>

#define FOREACH_RESULT(expr, res) for (auto const & res : ::txl::foreach_result([&]() { return expr; }))

namespace txl
{
    template<class T, class E>
    class foreach_result_view
    {
    private:
        std::function<result<T, E>()> iter_;
    public:
        class const_iterator
        {
        private:
            std::function<result<T, E>()> * iter_ = nullptr;
            safe_storage<T> current_{};
        public:
            const_iterator() = default;
            const_iterator(std::function<result<T, E>()> & iter)
                : iter_{&iter}
            {
                // Emplace first value
                ++(*this);
            }
            
            auto operator*() const -> T const &
            {
                return *current_;
            }

            auto operator->() const -> T const *
            {
                return &(*current_);
            }

            auto operator==(const_iterator const & it) const -> bool
            {
                return current_.empty() and it.current_.empty();
            }

            auto operator!=(const_iterator const & it) const -> bool
            {
                return not (*this == it);
            }

            auto operator++() -> const_iterator &
            {
                // Erase existing value
                current_.erase();
                
                if (iter_ == nullptr)
                {
                    // Unassigned
                    return *this;
                }

                auto res = (*iter_)();
                if (not res.is_assigned())
                {
                    // Empty
                    return *this;
                }

                // Emplace or throw
                current_.emplace(res.or_throw());
                return *this;
            }
        };

        foreach_result_view(std::function<result<T, E>()> iter)
            : iter_{iter}
        {
        }

        auto begin() -> const_iterator
        {
            return {iter_};
        }

        auto end() -> const_iterator
        {
            return {};
        }
    };

    template<class ResultFunc, class T = typename std::invoke_result_t<ResultFunc>::value_type, class E = typename std::invoke_result_t<ResultFunc>::error_context_type>
    inline auto foreach_result(ResultFunc && generator) -> foreach_result_view<T, E>
    {
        std::function<result<T, E>()> res = generator;
        return {res};
    }
}
