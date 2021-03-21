#include <coroutine>
#include <iostream>
#include <thread>
#include <memory>

// The Promise Workflow
// {
//     Promise prom;                      // (1)
//     co_await prom.initial_suspend();   // (2)
//     try {                                         
//         <function body>                  // (3)
//       }
//     catch (...) {
//         prom.unhandled_exception();
//     }
//     FinalSuspend:
//       co_await prom.final_suspend();     // (4)
// }

template <typename T>
struct my_future
{
    std::shared_ptr<T> value;

    my_future(std::shared_ptr<T> p): value(p)
    {
        // (3)
        std::cout << "    MyFuture::MyFuture" << '\n';
    }

    ~my_future()
    {
        std::cout << "    MyFuture::~MyFuture" << '\n';
    }

    T get()
    {
        std::cout << "    MyFuture::get" << '\n';
        return *value;
    }

    struct promise_type
    {
        // (4)
        std::shared_ptr<T> ptr = std::make_shared<T>(); // (11)
        promise_type()
        {
            std::cout << "        promise_type::promise_type" << '\n';
        }

        ~promise_type()
        {
            std::cout << "        promise_type::~promise_type" << '\n';
        }

        my_future<T> get_return_object()
        {
            std::cout << "        promise_type::get_return_object" << '\n';
            return ptr;
        }

        static std::suspend_never initial_suspend()
        {
            // (6)
            std::cout << "        promise_type::initial_suspend" << '\n';
            return {};
        }

        static std::suspend_never final_suspend() noexcept
        {
            // (7)
            std::cout << "        promise_type::final_suspend" << '\n';
            return {};
        }

        void return_value(T v)
        {
            std::cout << "        promise_type::return_value" << '\n';
            *ptr = v;
        }

        // static void return_void()
        // {
        // }

        static void unhandled_exception()
        {
            std::exit(1);
        }
    }; // (5)
};

template <typename T>
struct my_lazy_future
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    handle_type coro; // (5.1)

    my_lazy_future(handle_type h): coro(h)
    {
        std::cout << "    MyFuture::MyFuture" << '\n';
    }

    ~my_lazy_future()
    {
        std::cout << "    MyFuture::~MyFuture" << '\n';
        if (coro)
        {
            coro.destroy(); // (8.1)
        }
    }

    T get()
    {
        std::cout << "    MyFuture::get" << '\n';
        coro.resume(); // (6.1)
        return coro.promise().result;
    }

    struct promise_type
    {
        T result;

        promise_type()
        {
            std::cout << "        promise_type::promise_type" << '\n';
        }

        ~promise_type()
        {
            std::cout << "        promise_type::~promise_type" << '\n';
        }

        auto get_return_object()
        {
            // (3.1)
            std::cout << "        promise_type::get_return_object" << '\n';
            return my_lazy_future{handle_type::from_promise(*this)};
        }

        void return_value(T v)
        {
            std::cout << "        promise_type::return_value" << '\n';
            result = v;
        }

        static std::suspend_always initial_suspend()
        {
            // (1.1)
            std::cout << "        promise_type::initial_suspend" << '\n';
            return {};
        }

        static std::suspend_always final_suspend() noexcept
        {
            // (2.1)
            std::cout << "        promise_type::final_suspend" << '\n';
            return {};
        }

        // static void return_void() {}

        static void unhandled_exception()
        {
            std::exit(1);
        }
    };
};

// Execution on Another Thread
template <typename T>
struct my_future_thread
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type coro;

    my_future_thread(handle_type h): coro(h)
    {
    }

    ~my_future_thread()
    {
        if (coro)
        {
            coro.destroy();
        }
    }

    T get()
    {
        // (1.2)
        std::cout << "    MyFuture::get:  "
            << "std::this_thread::get_id(): "
            << std::this_thread::get_id() << '\n';

        // std::thread t([this] { coro.resume(); }); 
        // t.join();
        // std::jthread([this] { coro.resume(); }); 
        {
            std::jthread t([this] { coro.resume(); }); // (2.2)
        }
        return coro.promise().result;
    }

    struct promise_type
    {
        promise_type()
        {
            std::cout << "        promise_type::promise_type:  "
                << "std::this_thread::get_id(): "
                << std::this_thread::get_id() << '\n';
        }

        ~promise_type()
        {
            std::cout << "        promise_type::~promise_type:  "
                << "std::this_thread::get_id(): "
                << std::this_thread::get_id() << '\n';
        }

        T result;

        auto get_return_object()
        {
            return my_future_thread{handle_type::from_promise(*this)};
        }

        void return_value(T v)
        {
            std::cout << "        promise_type::return_value:  "
                << "std::this_thread::get_id(): "
                << std::this_thread::get_id() << '\n';
            std::cout << v << std::endl;
            result = v;
        }

        static std::suspend_always initial_suspend()
        {
            return {};
        }

        static std::suspend_always final_suspend() noexcept
        {
            std::cout << "        promise_type::final_suspend:  "
                << "std::this_thread::get_id(): "
                << std::this_thread::get_id() << '\n';
            return {};
        }

        static void unhandled_exception()
        {
            std::exit(1);
        }
    };
};

my_future<int> create_future()
{
    // (2)
    std::cout << "createFuture" << '\n';
    co_return 2021;
}

my_lazy_future<int> create_lazy_future()
{
    std::cout << "createFuture" << '\n';
    co_return 2021;
}

my_future_thread<int> create_future_thread()
{
    co_return 2021;
}

int main()
{
    // std::cout << '\n';
    //
    // auto fut = create_future(); // (1)
    // const auto res = fut.get(); // (8)
    // std::cout << "res: " << res << '\n';

    // std::cout << '\n';
    //
    // auto lazy_fut = create_lazy_future(); // (4.1)
    // const auto lazy_res = lazy_fut.get();                                             // (7.1)
    // std::cout << "res: " << lazy_res << '\n';

    std::cout << '\n';

    // In the first program eagerFutureWithComments.cpp, I stored the coroutine result in a
    // std::shared_ptr. This is critical because the coroutine is eagerly executed.
    //
    // In the program lazyFuture.cpp, the call final_suspend suspends always (line 2):
    // std::suspend_always final_suspend(). Consequently, the promise outlives the client, and a
    // std::shared_ptr is not necessary anymore. Returning std::suspend_never from the function
    // final_suspend would cause, in this case, undefined behavior, because the client would outlive
    // the promise. Hence, the lifetime of the result ends bevor the client asks for it.

    std::cout << "main:  "
        << "std::this_thread::get_id(): "
        << std::this_thread::get_id() << '\n';

    auto fut_thread = create_future_thread();
    const auto res_thread = fut_thread.get();
    std::cout << "res: " << res_thread << '\n';

    std::cout << '\n';
    // @promise_type
    //     You may wonder that the coroutine such as MyFuture has always inner type promise_type.
    //     This name is required. Alternatively, you can specialize std::coroutines_traits on MyFuture
    //     and define a public promise_type in it. I mentioned this point explicitly because I know a
    //     few people including me which already fall into this trap.
    //
    //     Here is another trap I fall into on Windows.
    //
    // @return_void and return_value
        // The promise needs either the member function return_void or return_value.
        //
        // The promise needs a return_void member function if
        //     -the coroutine has no co_return statement.
        //     -the coroutine has a co_return statement without argument.
        //     -the coroutine has a co_return expression statement where expression has type void.
        // The promise needs a return_value member function if it returns co_return expression
        // statement where expression must not have the type void
}
