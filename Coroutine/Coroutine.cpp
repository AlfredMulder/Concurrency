#include <coroutine>
#include <functional>
#include <iostream>
#include <thread>
#include <memory>
#include <random>
#include <vector>

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

// An Infinite Data Stream with Coroutines
template <typename T>
struct generator
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    explicit generator(handle_type h): coro(h)
    {
        std::cout << "        Generator::Generator" << '\n';
    }

    handle_type coro;

    ~generator()
    {
        std::cout << "        Generator::~Generator" << '\n';
        if (coro)
        {
            coro.destroy();
        }
    }

    generator(const generator&) = delete;
    generator& operator =(const generator&) = delete;

    generator(generator&& oth) noexcept: coro(oth.coro)
    {
        oth.coro = nullptr;
    }

    generator& operator =(generator&& oth) noexcept
    {
        coro = oth.coro;
        oth.coro = nullptr;
        return *this;
    }

    T get_next_value()
    {
        std::cout << "        Generator::getNextValue" << '\n';
        coro.resume(); // (13.3) 
        return coro.promise().current_value;
    }

    struct promise_type
    {
        promise_type()
        {
            // (2.3)
            std::cout << "            promise_type::promise_type" << '\n';
        }

        ~promise_type()
        {
            std::cout << "            promise_type::~promise_type" << '\n';
        }

        // static std::suspend_always initial_suspend()
        // {
        //     // (5.3)
        //     std::cout << "            promise_type::initial_suspend" << '\n';
        //     return {}; // (6.3)
        // }

        static std::suspend_never initial_suspend()
        {
            std::cout << "            promise_type::initial_suspend" << '\n';
            return {};
        }

        static std::suspend_always final_suspend() noexcept
        {
            std::cout << "            promise_type::final_suspend" << '\n';
            return {};
        }

        auto get_return_object()
        {
            // (3.3)
            std::cout << "            promise_type::get_return_object" << '\n';
            return generator{handle_type::from_promise(*this)}; // (4.3)
        }

        // std::suspend_always yield_value(int value)
        // {
        //     // (8.3)
        //     std::cout << "            promise_type::yield_value" << '\n';
        //     current_value = value; // (9.3)
        //     return {}; // (10.3)
        // }

        std::suspend_never yield_value(int value)
        {
            // (8.3)
            std::cout << "            promise_type::yield_value" << '\n';
            current_value = value; // (9.3)
            return {}; // (10.3)
        }

        static void return_void()
        {
        }

        static void unhandled_exception()
        {
            std::exit(1);
        }

        T current_value;
    };
};

generator<int> get_next(const int start = 10, const int step = 10)
{
    std::cout << "    getNext: start" << '\n';
    auto value = start;
    while (true)
    {
        // (11.3)
        std::cout << "    getNext: before co_yield" << '\n';
        co_yield value; // (7.3)
        std::cout << "    getNext: after co_yield" << '\n';
        value += step;
    }
}

template <typename T>
struct generator_2
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    explicit generator_2(handle_type h): coro(h)
    {
    }

    handle_type coro;

    ~generator_2()
    {
        if (coro)
        {
            coro.destroy();
        }
    }

    generator_2(const generator_2&) = delete;
    generator_2& operator =(const generator_2&) = delete;

    generator_2(generator_2&& oth) noexcept: coro(oth.coro)
    {
        oth.coro = nullptr;
    }

    generator_2& operator =(generator_2&& oth) noexcept
    {
        coro = oth.coro;
        oth.coro = nullptr;
        return *this;
    }

    T get_next_value()
    {
        coro.resume();
        return coro.promise().current_value;
    }

    struct promise_type
    {
        promise_type()
        {
        }

        ~promise_type()
        {
        }

        static std::suspend_always initial_suspend()
        {
            return {};
        }

        static std::suspend_always final_suspend() noexcept
        {
            return {};
        }

        auto get_return_object()
        {
            return generator_2{handle_type::from_promise(*this)};
        }

        std::suspend_always yield_value(const T value)
        {
            current_value = value;
            return {};
        }

        static void return_void()
        {
        }

        static void unhandled_exception()
        {
            std::exit(1);
        }

        T current_value;
    };
};

template <typename Cont>
generator_2<typename Cont::value_type> get_next(Cont cont)
{
    for (auto c : cont)
    {
        co_yield c;
    }
}

// Two predefined Awaitables

struct suspend_always
{
    static constexpr bool await_ready() noexcept { return false; }

    static constexpr void await_suspend(std::coroutine_handle<>) noexcept
    {
    }

    static constexpr void await_resume() noexcept
    {
    }
};

struct suspend_never
{
    static constexpr bool await_ready() noexcept { return true; }

    static constexpr void await_suspend(std::coroutine_handle<>) noexcept
    {
    }

    static constexpr void await_resume() noexcept
    {
    }
};

// awaiter workflow

// awaitable.await_ready() returns false:                   // (1)
//     
//     suspend coroutine
// 	
//     awaitable.await_suspend(coroutineHandle) returns:    // (3)
// 	
//         void:                                            // (4)
//             awaitable.await_suspend(coroutineHandle);
//             coroutine keeps suspended
//             return to caller
//
//         bool:                                            // (5)
//             bool result = awaitable.await_suspend(coroutineHandle);
//             if result: 
//                 coroutine keep suspended
//                 return to caller
//             else: 
//                 go to resumptionPoint
//
//         another coroutine handle:	                 // (6)
//             auto anotherCoroutineHandle = awaitable.await_suspend(coroutineHandle);
//             anotherCoroutineHandle.resume();
//             return to caller
// 	
// resumptionPoint:
//
// return awaitable.await_resume();                         // (2)

// Starting a Job on Request
struct job
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type coro;

    explicit job(const handle_type h): coro(h)
    {
    }

    ~job()
    {
        if (coro) coro.destroy();
    }

    void start() const
    {
        coro.resume(); // (6.5) 
    }


    struct promise_type
    {
        auto get_return_object()
        {
            return job{handle_type::from_promise(*this)};
        }

        static std::suspend_always initial_suspend()
        {
            // (4.5)
            std::cout << "    Preparing job" << '\n';
            return {};
        }

        static std::suspend_always final_suspend() noexcept // (7.5)
        {
            std::cout << "    Performing job" << '\n';
            return {};
        }

        static void return_void()
        {
        }

        static void unhandled_exception()
        {
        }
    };
};

job prepare_job() // (1.5)
{
    co_await std::suspend_never(); // (2.5)
}

// The Transparent Awaiter Workflow
struct my_suspend_always // (1.6)
{
    static bool await_ready() noexcept
    {
        std::cout << "        MySuspendAlways::await_ready" << '\n';
        return false;
    }

    static void await_suspend(std::coroutine_handle<>) noexcept
    {
        std::cout << "        MySuspendAlways::await_suspend" << '\n';
    }

    static void await_resume() noexcept
    {
        std::cout << "        MySuspendAlways::await_resume" << '\n';
    }
};

struct my_suspend_never // (2.6)
{
    static bool await_ready() noexcept
    {
        std::cout << "        MySuspendNever::await_ready" << '\n';
        return true;
    }

    static void await_suspend(std::coroutine_handle<>) noexcept
    {
        std::cout << "        MySuspendNever::await_suspend" << '\n';
    }

    static void await_resume() noexcept
    {
        std::cout << "        MySuspendNever::await_resume" << '\n';
    }
};

struct job_with_my_suspend
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type coro;

    explicit job_with_my_suspend(const handle_type h): coro(h)
    {
    }

    ~job_with_my_suspend()
    {
        if (coro)
        {
            coro.destroy();
        }
    }

    void start() const
    {
        coro.resume();
    }


    struct promise_type
    {
        auto get_return_object()
        {
            return job_with_my_suspend{handle_type::from_promise(*this)};
        }

        static my_suspend_always initial_suspend()
        {
            // (3.6)
            std::cout << "    Job prepared" << '\n';
            return {};
        }

        static my_suspend_always final_suspend() noexcept
        {
            // (4.6)
            std::cout << "    Job finished" << '\n';
            return {};
        }

        static void return_void()
        {
        }

        static void unhandled_exception()
        {
        }
    };
};

job_with_my_suspend prepare_job_with_my_suspend()
{
    co_await my_suspend_never(); // (5.6)
}

std::random_device seed;
auto gen = std::bind_front(std::uniform_int_distribution(0, 1), // (1.7)
                           std::default_random_engine(seed()));

struct my_suspend_always_for_auto // (3.7)
{
    static bool await_ready() noexcept
    {
        std::cout << "        MySuspendAlways::await_ready" << '\n';
        return gen();
    }

    static bool await_suspend(const std::coroutine_handle<> handle) noexcept
    // (5.7)
    {
        std::cout << "        MySuspendAlways::await_suspend" << '\n';
        handle.resume(); // (6.7)
        return true;
    }

    static void await_resume() noexcept // (4.7)
    {
        std::cout << "        MySuspendAlways::await_resume" << '\n';
    }
};

// Automatically Resuming the Awaiter
struct job_auto
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type coro;

    explicit job_auto(const handle_type h): coro(h)
    {
    }

    ~job_auto()
    {
        if (coro)
        {
            coro.destroy();
        }
    }

    struct promise_type
    {
        auto get_return_object()
        {
            return job_auto{handle_type::from_promise(*this)};
        }

        static my_suspend_always_for_auto initial_suspend() // (2.7)
        {
            std::cout << "    Job prepared" << '\n';
            return {};
        }

        static std::suspend_always final_suspend() noexcept
        {
            std::cout << "    Job finished" << '\n';
            return {};
        }

        static void return_void()
        {
        }

        static void unhandled_exception()
        {
        }
    };
};

job_auto perform_job()
{
    co_await std::suspend_never();
}

// Automatically Resuming the Awaiter on a Separate Thread
struct my_awaitable
{
    std::jthread& outer_thread;

    [[nodiscard]] static bool await_ready() noexcept
    {
        const auto res = gen();
        if (res) std::cout << " (executed)" << '\n';
        else std::cout << " (suspended)" << '\n';
        return res; // (6.8)   
    }

    void await_suspend(std::coroutine_handle<> h) const
    {
        // (7.8)
        outer_thread = std::jthread([h] { h.resume(); }); // (8.8)
    }

    static void await_resume()
    {
    }
};


struct job_auto_resume_separate_thread
{
    static inline int job_counter{1};

    job_auto_resume_separate_thread()
    {
        ++job_counter;
    }

    struct promise_type
    {
        int job_number{job_counter};

        static job_auto_resume_separate_thread get_return_object()
        {
            return {};
        }

        [[nodiscard]] std::suspend_never initial_suspend() const
        {
            // (2.8)
            std::cout << "    Job " << job_number << " prepared on thread "
                << std::this_thread::get_id();
            return {};
        }

        [[nodiscard]] std::suspend_never final_suspend() const noexcept
        {
            // (3.8)
            std::cout << "    Job " << job_number << " finished on thread "
                << std::this_thread::get_id() << '\n';
            return {};
        }

        static void return_void()
        {
        }

        static void unhandled_exception()
        {
        }
    };
};

job_auto_resume_separate_thread perform_job(std::jthread& out)
{
    co_await my_awaitable{out}; // (1.8)
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

    // std::cout << "main:  "
    //     << "std::this_thread::get_id(): "
    //     << std::this_thread::get_id() << '\n';
    //
    // auto fut_thread = create_future_thread();
    // const auto res_thread = fut_thread.get();
    // std::cout << "res: " << res_thread << '\n';
    //
    // std::cout << '\n';
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

    // auto gen = get_next(); // (1.3)
    // for (int i = 0; i <= 2; ++i)
    // {
    //     const auto val = gen.get_next_value(); // (12.3)
    //     std::cout << "main: " << val << '\n'; // (14.3)
    // }
    //
    // std::cout << '\n';

    // const std::string hello_world = "Hello world";
    // std::cout << "dick" << '\n';
    // auto gen = get_next(hello_world); // (1.4)
    // for (unsigned long long i = 0; i < hello_world.size(); ++i)
    // {
    //     std::cout << gen.get_next_value() << " "; // (4.4)
    // }
    //
    // std::cout << "\n\n";
    //
    // auto gen2 = get_next(hello_world); // (2.4)
    // for (int i = 0; i < 5; ++i)
    // {
    //     // (5.4)
    //     std::cout << gen2.get_next_value() << " ";
    // }
    //
    // std::cout << "\n\n";
    //
    // const std::vector my_vec{1, 2, 3, 4, 5};
    // auto gen3 = get_next(my_vec); // (3.4)
    // for (unsigned long long i = 0; i < my_vec.size(); ++i)
    // {
    //     // (6.4)
    //     std::cout << gen3.get_next_value() << " ";
    // }
    //
    // std::cout << '\n';

    // std::cout << "Before job" << '\n';
    //
    // const auto job = prepare_job();                                                                        // (3.5)                       
    // job.start();                                                                                         // (5.5)  
    //
    // std::cout << "After job" << '\n';


    // std::cout << "Before job" << '\n';
    //
    // const auto job_with_my_suspend = prepare_job_with_my_suspend();                                       // (6.6)
    // job_with_my_suspend.start();                                                                         // (7.6)
    //
    // std::cout << "After job" << '\n';

    // std::cout << "Before jobs" << '\n';
    //
    // perform_job();
    // perform_job();
    // perform_job();
    // perform_job();
    //
    // std::cout << "After jobs" << '\n';

    for (std::vector<std::jthread> threads(8); auto& thr : threads)
        // (4.8), (5.8)
    {
        perform_job(thr);
    }
}
