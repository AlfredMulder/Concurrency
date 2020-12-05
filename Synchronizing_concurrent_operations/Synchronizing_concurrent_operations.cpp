#include <condition_variable>
#include <mutex>
#include <algorithm>
#include <cassert>
#include <charconv>
#include <chrono>
#include <future>
#include <iostream>
#include <list>
#include <optional>
#include <queue>
#include <string>
#include <type_traits>

std::condition_variable cv;
bool done;
std::mutex m;


// Waiting for data to process with std::condition_variable
// std::mutex mut;
// std::queue<int> data_queue;
// std::condition_variable data_cond;
//
// void data_preparation_thread()
// {
//     while (more_data_to_prepare())
//     {
//         data_chunk const data = prepare_data();
//         {
//             std::lock_guard<std::mutex> lk(mut);
//             data_queue.push(data);
//         }
//         data_cond.notify_one();
//     }
// }
//
// void data_processing_thread()
// {
//     while (true)
//     {
//         std::unique_lock<std::mutex> lk(mut);
//         data_cond.wait(
//             lk, [] { return !data_queue.empty(); });
//         data_chunk data = data_queue.front();
//         data_queue.pop();
//         lk.unlock();
//         process(data);
//         if (is_last_chunk(data))
//             break;
//     }
// }

// Fundamentally, std::condition_variable::wait is an optimization over a busy-wait.

template <typename Predicate>
void minimal_wait(std::unique_lock<std::mutex>& lk, Predicate pred)
{
    while (!pred())
    {
        lk.unlock();
        lk.lock();
    }
}

// Full class definition of a thread-safe queue using condition variables
template <typename T>
class thread_safe_queue
{
private:
    mutable std::mutex mut_;
    std::queue<T> data_queue_;
    std::condition_variable data_cond_;
public:
    thread_safe_queue()
    {
    }

    thread_safe_queue(thread_safe_queue const& other)
    {
        std::lock_guard<std::mutex> lk(other.mut_);
        data_queue_ = other.data_queue_;
    }

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut_);
        data_queue_.push(new_value);
        data_cond_.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut_);
        data_cond_.wait(lk, [this]
        {
            return !data_queue_.empty();
        });
        value = data_queue_.front();
        data_queue_.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut_);
        data_cond_.wait(lk, [this]
        {
            return !data_queue_.empty();
        });
        std::shared_ptr<T> res(std::make_shared<T>(data_queue_.front()));
        data_queue_.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut_);
        if (data_queue_.empty())
        {
            return false;
        }
        value = data_queue_.front();
        data_queue_.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut_);
        if (data_queue_.empty())
        {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res(std::make_shared<T>(data_queue_.front()));
        data_queue_.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut_);
        return data_queue_.empty();
    }
};

// Passing arguments to a function with std::async
struct X
{
    void foo(int, std::string const&);
    std::string bar(std::string const&);
};

// X x;
// auto f1 = std::async(&X::foo, &x, 42, "hello");
// auto f2 = std::async(&X::bar, x, "goodbye");

struct y
{
    double operator()(double);
};

// y y_1;
// auto f3 = std::async(y(), 3.141);
// auto f4 = std::async(std::ref(y_1), 2.718);
// X baz(X&);
// auto d = std::async(baz,std::ref(x));

class move_only
{
public:
    move_only();
    move_only(move_only&&) noexcept;
    move_only(move_only const&) = delete;
    move_only& operator=(move_only&&) noexcept;
    move_only& operator=(move_only const&) = delete;
    void operator()();
};

// auto f5 = std::async(move_only());

// std::packaged_task<> ties a future to a function or callable object.

// Partial class definition for a specialization of std::packaged_task< >
// template <>
// class packaged_task<std::string(std::vector<char>*, int)>
// {
// public:
//     template <typename Callable>
//     explicit packaged_task(Callable&& f);
//     std::future<std::string> get_future();
//     void operator()(std::vector<char>*, int);
// };

// PASSING TASKS BETWEEN THREADS
// Running code on a GUI thread using std::packaged_task

std::mutex m_2;
std::deque<std::packaged_task<void()>> tasks;

bool gui_shutdown_message_received()
{
    return true;
}

void get_and_process_gui_message()
{
}

void gui_thread()
{
    while (!gui_shutdown_message_received())
    {
        get_and_process_gui_message();
        std::packaged_task<void()> task;
        {
            std::lock_guard<std::mutex> lk(m_2);
            if (tasks.empty())
                continue;
            task = std::move(tasks.front());
            tasks.pop_front();
        }
        task();
    }
}

std::thread gui_bg_thread(gui_thread);

template <typename Func>
std::future<void> post_task_for_gui_thread(Func f)
{
    std::packaged_task<void()> task(f);
    auto res = task.get_future();
    std::lock_guard<std::mutex> lk(m_2);
    tasks.push_back(std::move(task));
    return res;
}

bool wait_loop()
{
    auto const timeout = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(500);
    std::unique_lock<std::mutex> lk(m);
    while (!done)
    {
        if (cv.wait_until(lk, timeout) == std::cv_status::timeout)
            break;
    }
    return done;
}

// Saving an exception for the future
double square_root(double x)
{
    if (x < 0)
    {
        throw std::out_of_range("x < 0");
    }
    return sqrt(x);
}

std::future<double> f = std::async(std::launch::async, square_root, -1);
double y = f.get();

// extern std::promise<double> some_promise;
// try
// {
//     some_promise.set_value(calculate_value());
// }
//
// catch(...)
// {
//     some_promise.set_exception(std::current_exception());
// }

// Waiting from multiple threads
// std::promise<int> p;
// std::future<int> f_1(p.get_future());
// assert(f_1.valid());
// std::shared_future<int> sf(std::move(f_1));
// assert(!f_1.valid());
// assert(sf.valid());

// std::future<int> f_1=std::async(some_task);
// if(f_1.wait_for(std::chrono::milliseconds(35))==std::future_status::ready)
// do_something_with(f_1.get());

// Making (std::)promises
// Handling multiple connections from a single thread using promises
// void process_connections(connection_set& connections)
// {
//     while (!done(connections))
//     {
//         for (connection_iterator
//                  connection = connections.begin(), end = connections.end();
//              connection != end;
//              ++connection)
//         {
//             if (connection->has_incoming_data())
//             {
//                 data_packet data = connection->incoming();
//                 std::promise<payload_type>& p =
//                     connection->get_promise(data.id);
//                 p.set_value(data.payload);
//             }
//             if (connection->has_outgoing_data())
//             {
//                 outgoing_packet data =
//                     connection->top_of_outgoing_queue();
//                 connection->send(data.payload);
//                 data.promise.set_value(true);
//             }
//         }
//     }
// }

// Listing 4.12 A sequential implementation of Quicksort
template <typename T>
std::list<T> sequential_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result = {};
    result.splice(result.begin(), input, input.begin());
    auto pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
                                       [&](T const& t) { return t < pivot; });
    std::list<T> lower_part = {};
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

    auto new_lower(sequential_quick_sort(std::move(lower_part)));
    auto new_higher(sequential_quick_sort(std::move(input)));

    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower);
    return result;
}

// Parallel Quicksort using futures
template <typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;

    result.splice(result.begin(), input, input.begin());
    auto pivot = *result.begin();
    auto divide_point =
        std::partition(input.begin(), input.end(), [&](T const& t) { return t < pivot; });

    std::list<T> lower_part = {};

    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

    std::future<std::list<T>> new_lower(std::async(std::launch::async,
                                                   &parallel_quick_sort<T>, std::move(lower_part)));
    std::future<std::list<T>> new_higher(parallel_quick_sort<T>(std::move(input)));

    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());

    return result;
}

template <typename Func, typename Arg>
std::future<std::invoke_result<Func(Arg&&)>> spawn_task(Func&& func, Arg&& arg)
{
    using result_type = std::invoke_result<Func(Arg&&)>;
    std::packaged_task<result_type(Arg&&)> task(std::move(func));
    std::future<result_type> res(task.get_future());
    std::thread t(std::move(task), std::move(arg));
    t.detach();
    return res;
}

// A simple implementation of an ATM logic class(appendix C) Actor model(there are several discrete actors in the system (each running on a separate
// thread), which send messages to each other to perform the task at hand, and there’s
// no shared state except that which is directly passed via messages.)
// struct card_inserted
// {
//     std::string account;
// };
//
// class atm
// {
//     messaging::receiver incoming;
//     messaging::sender bank;
//     messaging::sender interface_hardware;
//     void (atm::* state)() = nullptr;
//     std::string account;
//     std::string pin;
//
//     void waiting_for_card()
//     {
//         interface_hardware.send(display_enter_card());
//         incoming.wait()
//                 .handle<card_inserted>(
//                     [&](card_inserted const& msg)
//                     {
//                         account = msg.account;
//                         pin = "";
//                         interface_hardware.send(display_enter_pin());
//                         state = &atm::getting_pin;
//                     }
//                 );
//     }
//
//     void getting_pin()
//     {
//         incoming.wait()
//                 .handle<digit_pressed>(
//                     [&](digit_pressed const& msg)
//                     {
//                         unsigned const pin_length = 4;
//                         pin += msg.digit;
//                         if (pin.length() == pin_length)
//                         {
//                             bank.send(verify_pin(account, pin, incoming));
//                             state = &atm::verifying_pin;
//                         }
//                     }
//                 )
//                 .handle<clear_last_pressed>(
//                     [&](clear_last_pressed const& msg)
//                     {
//                         if (!pin.empty())
//                         {
//                             pin.resize(pin.length() - 1);
//                         }
//                     }
//                 )
//                 .handle<cancel_pressed>(
//                     [&](cancel_pressed const& msg)
//                     {
//                         state = &atm::done_processing;
//                     }
//                 );
//     }
//
// public:
//     void run()
//     {
//         state = &atm::waiting_for_card;
//         try
//         {
//             for (;;)
//             {
//                 (this->*state)();
//             }
//         }
//         catch (messaging::close_queue const&)
//         {
//         }
//     }
// };

// Continuation when it's done - .then()
// std::future<int> find_the_answer();
// auto fut = find_the_answer();
// auto fut2 = fut.then(find_the_question);
// static_assert(fut.valid(),"");
// static_assert(fut2.valid(),"");

// A simple equivalent to std::async for Concurrency TS futures
// template<typename Func>
// std::experimental::future<decltype(std::declval<Func>()())>
// spawn_async(Func&& func) {
//     std::experimental::promise<
//         decltype(std::declval<Func>()())> p;
//     auto res = p.get_future();
//     std::thread t(
//         [p = std::move(p), f = std::decay_t<Func>(func)]()
//         mutable{
//         try {
//             p.set_value_at_thread_exit(f());
//         }
//         catch (...) {
//             p.set_exception_at_thread_exit(std::current_exception());
//         }
//     });
//     t.detach();
//     return res;
// }

// A function to process user login with continuations
// std::experimental::future<void> process_login(
//     std::string const& username, std::string const& password)
// {
//     return spawn_async([=]()
//     {
//         return backend.authenticate_user(username, password);
//     }).then([](std::experimental::future<user_id> id)
//     {
//         return backend.request_current_info(id.get());
//     }).then([](std::experimental::future<user_data> info_to_display)
//     {
//         try
//         {
//             update_display(info_to_display.get());
//         }
//         catch (std::exception& e)
//         {
//             display_error(e);
//         }
//     });
// }

// A function to process user login with fully asynchronous operations
// std::experimental::future<void> process_login(
//     std::string const& username, std::string const& password)
// {
//     return backend.async_authenticate_user(username, password).then(
//         [](auto id)
//         {
//             return backend.async_request_current_info(id.get());
//         }).then([](auto info_to_display)
//     {
//         try
//         {
//             update_display(info_to_display.get());
//         }
//         catch (std::exception& e)
//         {
//             display_error(e);
//         }
//     });
// }

/// Waiting for more than one future

// Gathering results from futures using std::async
// std::future<FinalResult> process_data(std::vector<MyData>& vec)
// {
//     size_t const chunk_size = whatever;
//     std::vector<std::future<ChunkResult>> results;
//     for (auto begin = vec.begin(), end = vec.end(); beg != end;)
//     {
//         size_t const remaining_size = end - begin;
//         size_t const this_chunk_size = std::min(remaining_size, chunk_size);
//         results.push_back(
//             std::async(process_chunk, begin, begin + this_chunk_size));
//         begin += this_chunk_size;
//     }
//     return std::async([all_results=std::move(results)]()
//     {
//         std::vector<ChunkResult> v;
//         v.reserve(all_results.size());
//         for (auto& f : all_results)
//         {
//             v.push_back(f.get());
//         }
//         return gather_results(v);
//     });
// }

// Gathering results from futures using std::experimental::when_all
// std::experimental::future<FinalResult> process_data(
//     std::vector<MyData>& vec)
// {
//     size_t const chunk_size = whatever;
//     std::vector<std::experimental::future<ChunkResult>> results;
//     for (auto begin = vec.begin(), end = vec.end(); beg != end;)
//     {
//         size_t const remaining_size = end - begin;
//         size_t const this_chunk_size = std::min(remaining_size, chunk_size);
//         results.push_back(
//             spawn_async(
//                 process_chunk, begin, begin + this_chunk_size));
//         begin += this_chunk_size;
//     }
//     return std::experimental::when_all(
//         results.begin(), results.end()).then(
//         [](std::future<std::vector<
//         std::experimental::future<ChunkResult>>> ready_results)
//         {
//             std::vector<std::experimental::future<ChunkResult>>
//                 all_results = ready_results.get();
//             std::vector<ChunkResult> v;
//             v.reserve(all_results.size());
//             for (auto& f : all_results)
//             {
//                 v.push_back(f.get());
//             }
//             return gather_results(v);
//         });
// }

// Using std::experimental::when_any to process the first value found
// std::experimental::future<FinalResult>
// find_and_process_value(std::vector<MyData>& data)
// {
//     unsigned const concurrency = std::thread::hardware_concurrency();
//     unsigned const num_tasks = (concurrency > 0) ? concurrency : 2;
//     std::vector<std::experimental::future<MyData*>> results;
//     auto const chunk_size = (data.size() + num_tasks - 1) / num_tasks;
//     auto chunk_begin = data.begin();
//     std::shared_ptr<std::atomic<bool>> done_flag =
//         std::make_shared<std::atomic<bool>>(false);
//     for (unsigned i = 0; i < num_tasks; ++i)
//     {
//         auto chunk_end =
//             (i < (num_tasks - 1)) ? chunk_begin + chunk_size : data.end();
//         results.push_back(spawn_async([=]
//         {
//             for (auto entry = chunk_begin;
//                  !*done_flag && (entry != chunk_end);
//                  ++entry)
//             {
//                 if (matches_find_criteria(*entry))
//                 {
//                     *done_flag = true;
//                     return &*entry;
//                 }
//             }
//             return static_cast<MyData*>(nullptr);
//         }));
//         chunk_begin = chunk_end;
//     }
//     std::shared_ptr<std::experimental::promise<FinalResult>> final_result =
//         std::make_shared<std::experimental::promise<FinalResult>>();
//     struct DoneCheck
//     {
//         std::shared_ptr<std::experimental::promise<FinalResult>>
//         final_result;
//
//         DoneCheck(
//             std::shared_ptr<std::experimental::promise<FinalResult>>
//             final_result_)
//             : final_result(std::move(final_result_))
//         {
//         }
//
//         void operator()(
//             std::experimental::future<std::experimental::when_any_result<
//                 std::vector<std::experimental::future<MyData*>>>>
//             results_param)
//         {
//             auto results = results_param.get();
//             MyData* const ready_result =
//                 results.futures[results.index].get();
//             if (ready_result)
//                 final_result->set_value(
//                     process_found_value(*ready_result));
//             else
//             {
//                 results.futures.erase(
//                     results.futures.begin() + results.index);
//                 if (!results.futures.empty())
//                 {
//                     std::experimental::when_any(
//                             results.futures.begin(), results.futures.end())
//                         .then(std::move(*this));
//                 }
//                 else
//                 {
//                     final_result->set_exception(
//                         std::make_exception_ptr(
//                             std::runtime_error(“Not found”)));
//                 }
//             }
//         };

// LATCH - A latch is a syn-
// chronization object that becomes ready when its counter is decremented to zero. Its
// name comes from the fact that it latches the output—once it is ready, it stays ready
// until it is destroyed. A latch is thus a lightweight facility for waiting for a series of
// events to occur.

// BARRIER is a reusable synchronization component used for
// internal synchronization between a set of threads. Whereas a latch doesn’t care which
// threads decrement the counter—the same thread can decrement the counter multi-
// ple times, or multiple threads can each decrement the counter once, or some combi-
// nation of the two—with barriers, each thread can only arrive at the barrier once per
// cycle. When threads arrive at the barrier, they block until all of the threads involved
// have arrived at the barrier, at which point they are all released. The barrier can then
// be reused—the threads can then arrive at the barrier again to wait for all the threads
// for the next cycle.

// Waiting for events with std::experimental::latch
// void foo()
// {
//     unsigned const thread_count = ...;
//     latch done(thread_count);
//     my_data data[thread_count];
//     std::vector<std::future<void>> threads;
//     for (unsigned i = 0; i < thread_count; ++i)
//         threads.push_back(std::async(std::launch::async, [&,i]
//         {
//             data[i] = make_data(i);
//             done.count_down();
//             do_more_stuff();
//         }));
//     done.wait();
//     process_data(data, thread_count);
// }

// Using std::experimental::barrier
// result_chunk process(data_chunk);
// std::vector<data_chunk>
// divide_into_chunks(data_block data, unsigned num_threads);
//
// void process_data(data_source& source, data_sink& sink)
// {
//     unsigned const concurrency = std::thread::hardware_concurrency();
//     unsigned const num_threads = (concurrency > 0) ? concurrency : 2;
//     std::experimental::barrier sync(num_threads);
//     std::vector<joining_thread> threads(num_threads);
//     std::vector<data_chunk> chunks;
//     result_block result;
//     for (unsigned i = 0; i < num_threads; ++i)
//     {
//         threads[i] = joining_thread([&, i]
//         {
//             while (!source.done())
//             {
//                 if (!i)
//                 {
//                     data_block current_block =
//                         source.get_next_data_block();
//                     chunks = divide_into_chunks(
//                         current_block, num_threads);
//                 }
//                 sync.arrive_and_wait();
//                 result.set_chunk(i, num_threads, process(chunks[i]));
//                 sync.arrive_and_wait();
//                 if (!i)
//                 {
//                     sink.write_data(std::move(result));
//                 }
//             }
//         });
//     }
// }

// The interface to std::experimental::flex_barrier differs from that of std::
// experimental::barrier in only one way: there is an additional constructor that takes
// a completion function, as well as a thread count. This function is run on exactly one
// of the threads that arrived at the barrier, once all the threads have arrived at the bar-
// rier.

// Using std::flex_barrier to provide a serial region
// void process_data(data_source& source, data_sink& sink)
// {
//     unsigned const concurrency = std::thread::hardware_concurrency();
//     unsigned const num_threads = (concurrency > 0) ? concurrency : 2;
//     std::vector<data_chunk> chunks;
//     auto split_source = [&]
//     {
//         if (!source.done())
//         {
//             data_block current_block = source.get_next_data_block();
//             chunks = divide_into_chunks(current_block, num_threads);
//         }
//     };
//     split_source();
//     result_block result;
//     std::experimental::flex_barrier sync(num_threads, [&]
//     {
//         sink.write_data(std::move(result));
//         split_source();
//         return -1;
//     });
//     std::vector<joining_thread> threads(num_threads);
//     for (unsigned i = 0; i < num_threads; ++i)
//     {
//         threads[i] = joining_thread([&, i]
//         {
//             while (!source.done())
//             {
//                 result.set_chunk(i, num_threads, process(chunks[i]));
//                 sync.arrive_and_wait();
//             }
//         });
//     }
// }

std::ostream& operator<<(std::ostream& ostr, const std::list<int>& list)
{
    for (const auto& i : list)
    {
        ostr << " " << i;
    }
    return ostr;
}

std::optional<std::string> create(bool i)
{
    if (i)
    {
        return "Godzilla";
    }
    return {};
}

std::optional<int> convert(const std::string& arg)
{
    try
    {
        return std::stoi(arg);
    }
    catch (...)
    {
        std::cerr << "Can't convert " << arg << '\n';
    }
    return std::nullopt;
}

int main(int argc, char* argv[])
{
    // const auto b = sequential_quick_sort(i);
    //
    // std::cout << b << '\n';
    std::list<int> i = {1, 2, 3, 3, 2, 1, 5, 4, 5, 67, 3, 21, 3, 325, 346, 6};
    std::list<int> i_2 = {8, 9, 10, 11, 12};
    auto iter = i.begin();
    std::advance(iter, 2);
    i.splice(iter, i_2);
    std::cout << "i: " << i << "\n";
    std::cout << "i_2: " << i_2 << "\n";
    i_2.splice(i_2.begin(), i, iter, i.end());
    std::cout << "i: " << i << "\n";
    std::cout << "i_2: " << i_2 << "\n";

    // std::list<int> list1 = {1, 2, 3, 4, 5};
    // std::list<int> list2 = {10, 20, 30, 40, 50};
    auto it = i.begin();
    std::advance(it, 3);
    i.splice(it, i_2);
    std::cout << "list1: " << i << "\n";
    std::cout << "list2: " << i_2 << "\n";
    i_2.splice(i_2.begin(), i, it, i.end());
    std::cout << "list1: " << i << "\n";
    std::cout << "list2: " << i_2 << "\n";

    std::cout << create(false).value_or("empty") << '\n';

    auto first = convert("abc");
    auto second = convert("2");
    std::cout << *first + *second;
}
