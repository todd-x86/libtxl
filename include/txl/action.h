#pragma once

#include <memory>
#include <functional>
#include <optional>

namespace txl
{
    template<class... Args>
    class action
    {
    private:
        std::function<void(Args...)> body_;
        std::unique_ptr<action<Args...>> next_;
    public:
        action() = default;
        
        template<class Func>
        action(Func && func)
            : body_(std::move(func))
        {
        }

        auto empty() const -> bool { return body_ == nullptr; }
        
        template<class Func>
        auto then(Func && func) -> action<Args...>
        {
            auto body = body_;
            return {[body=std::move(body), func=std::move(func)](Args && ... args) {
                body(args...);
                func(args...);
            }};
        }

        auto operator()(Args && ... args) -> void
        {
            auto body = body_;
            if (body)
            {
                body(std::forward<Args>(args)...);
            }
        }
    };

    template<class ReturnType, class... Args>
    class func
    {
    };
}
