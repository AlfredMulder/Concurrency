// Managing_threads.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <algorithm>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>
#include <gsl/gsl>

// A function that returns while a thread still has access to local variables
struct func
{
    int& i;

    explicit func(int& i): i(i)
    {
    }

    void operator()() const
    {
        for (unsigned j = 0; j < 1000000; ++j)
        {
            std::cout << i;
        }
    }
};

void oops()
{
    auto some_local_state = 0;
    func my_func(some_local_state);
    std::thread my_thread(my_func);
    my_thread.detach();
}

// Waiting for a thread to finish
struct func;

void f()
{
    auto some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_func);
    try
    {
        // do_something_in_current_thread();
    }
    catch (...)
    {
        t.join();
        throw;
    }
    t.join();
}

// Using RAII to wait for a thread to complete
class thread_guard
{
public:
    thread_guard(thread_guard&& other) noexcept
        : t_(other.t_),
          thread_gua_(std::move(other.thread_gua_))
    {
    }

    thread_guard& operator=(const thread_guard& other)
    {
        using std::swap;
        swap(*this, other.thread_gua_);
        return *this;
    }

    thread_guard& operator=(thread_guard&& other) noexcept
    {
        if (other.t_.joinable())
        {
            other.t_.join();
        }
        t_ = std::move(other.t_);
        return *this;
    }

private:
    std::thread& t_;
    thread_guard&& thread_gua_;

public:
    thread_guard() = delete;

    ~thread_guard()
    {
        if (t_.joinable())
        {
            t_.join();
        }
    }

    auto operator<=>(const thread_guard&) const
    {
    }
};

struct func;

void f_1()
{
    auto some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_func);
    // thread_guard g(t);
    // do_something_in_current_thread();
}

// Detaching a thread to handle other documents
// void edit_document(std::string const& filename)
// {
//     open_document_and_display_gui(filename);
//     while (!done_editing())
//     {
//         user_command cmd = get_user_input();
//         if (cmd.type == open_new_document)
//         {
//             std::string const new_name = get_filename_from_user();
//             std::thread t(edit_document, new_name);
//             t.detach();
//         }
//         else
//         {
//             process_user_input(cmd);
//         }
//     }
// }

// scoped_thread and example usage
class scoped_thread
{
public:
    explicit scoped_thread(std::thread t):
        t_(std::move(t))
    {
        if (!t.joinable())
        {
            throw std::logic_error("No thread");
        }
    }

    scoped_thread& operator=(scoped_thread other)
    {
        if (other.t_.joinable())
        {
            other.t_.join();
        }
        t_ = std::move(other.t_);
        return *this;
    }

    scoped_thread& operator=(scoped_thread&& other) noexcept
    {
        if (other.t_.joinable())
        {
            other.t_.join();
        }
        t_ = std::move(other.t_);
        return *this;
    }

private:
    std::thread t_;
public:


    ~scoped_thread()
    {
        t_.join();
    }

    scoped_thread(scoped_thread const&)
    {
    }

    auto operator<=>(scoped_thread const&) const
    {
    }
};

struct func;

void f_2()
{
    int some_local_state;
    scoped_thread t{std::thread(func(some_local_state))};
    // do_something_in_current_thread();
}

int main()
{
    std::cout << "Hello World!\n";
}

// A joining_thread class
class joining_thread
{
    std::thread t_;
public:

    template <typename Callable, typename ... Args>
    explicit joining_thread(Callable&& func, Args&& ... args):
        t_(std::forward<Callable>(func), std::forward<Args>(args)...)
    {
    }

    explicit joining_thread(std::thread t) noexcept:
        t_(std::move(t))
    {
    }

    joining_thread(joining_thread&& other) noexcept:
        t_(std::move(other.t_))
    {
    }

    joining_thread()
    = default;

    joining_thread& operator=(joining_thread other) noexcept
    {
        if (joinable())
        {
            join();
        }
        t_ = std::move(other.t_);
        return *this;
    }

    joining_thread& operator=(joining_thread&& other) noexcept
    {
        if (joinable())
        {
            join();
        }
        t_ = std::move(other.t_);
        return *this;
    }

    joining_thread& operator=(std::thread other) noexcept
    {
        if (joinable())
        {
            join();
        }
        t_ = std::move(other);
        return *this;
    }

    ~joining_thread() noexcept
    {
        if (joinable())
        {
            join();
        }
    }

    void swap(joining_thread& other) noexcept
    {
        t_.swap(other.t_);
    }

    [[nodiscard]]
    std::thread::id get_id() const noexcept
    {
        return t_.get_id();
    }

    [[nodiscard]]
    bool joinable() const noexcept
    {
        return t_.joinable();
    }

    void join()
    {
        t_.join();
    }

    void detach()
    {
        t_.detach();
    }

    std::thread& as_thread() noexcept
    {
        return t_;
    }

    [[nodiscard]]
    const std::thread& as_thread() const noexcept
    {
        return t_;
    }
};

// Spawns some threads and waits for them to finish
void do_work(const unsigned id)
{
    // id = 0u;
    std::cout << id << '\n';
};

// A naïve parallel version of std::accumulate
template <typename Iterator, typename T>
struct accumulate_block
{
    void operator()(Iterator first, Iterator last, T& result)
    {
        result = std::accumulate(first, last, result);
    }
};

template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
    auto const length = std::distance(first, last);
    if (!length)
    {
        return init;
    }
    auto const min_per_thread = 25;
    auto const max_threads = (length + min_per_thread - 1) / min_per_thread;
    auto const hardware_threads = gsl::narrow_cast<int>(std::thread::hardware_concurrency());
    auto const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    auto const block_size = length / num_threads;
    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads - 1);
    Iterator block_start = first;
    for (unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        threads[i] = std::thread(
            accumulate_block<Iterator, T>(),
            block_start, block_end, std::ref(results[i]));
        block_start = block_end;
    }
    accumulate_block<Iterator, T>()(
        block_start, last, results[num_threads - 1]);
    for (auto& entry : threads)
    {
        entry.join();
    }
    return std::accumulate(results.begin(), results.end(), init);
}

std::thread::id master_thread;

void some_core_part_of_algorithm()
{
    if (std::this_thread::get_id() == master_thread)
    {
        // do_master_thread_work();
        std::cout << std::this_thread::get_id();
    }
    // do_common_work();
}

void f_3()
{
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 20; ++i)
    {
        threads.emplace_back(do_work, i);
    }
    for (auto& entry : threads)
    {
        entry.join();
    }
}
