#include <iostream>

#include <coroutine>

template<class T>
class BasicCoroutine {
public:
    struct Promise {
        BasicCoroutine get_return_object() { return BasicCoroutine {}; }

        void unhandled_exception() noexcept { }

        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        T return_value(T && value) { return 4; }
    };
    using promise_type = Promise;
    T value_ = 0;
};

BasicCoroutine<int> coro()
{
    co_return 42;
}

int main()
{
    std::cout << coro().value_ << std::endl;
}
