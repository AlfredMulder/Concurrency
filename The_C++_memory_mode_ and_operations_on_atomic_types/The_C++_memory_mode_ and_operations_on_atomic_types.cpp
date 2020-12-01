#include <cassert>
#include <chrono>
#include <iostream>
#include <vector>
#include <thread>
#include <gsl/pointers>
#include <gsl/span>
#include <ranges>
#include <coroutine>

// You can also avoid the unde-
// fined behavior by using atomic operations to access the memory location involved
// in the race. This doesn’t prevent the race itself—which of the atomic operations
// touches the memory location first is still not specified—but it does bring the program
// back into the realm of defined behavior.

// is_lock_free() member function, which allows the user to deter-
// mine whether operations on a given type are done directly with atomic instructions
// (x.is_lock_free() returns true) or done by using a lock internal to the compiler
// and library (x.is_lock_free() returns false).

// The macros are ATOMIC_BOOL_LOCK_FREE, ATOMIC_CHAR_LOCK_FREE, ATOMIC_
// CHAR16_T_LOCK_FREE,
// ATOMIC_CHAR32_T_LOCK_FREE,
// ATOMIC_WCHAR_T_LOCK_FREE,
// ATOMIC_SHORT_LOCK_FREE, ATOMIC_INT_LOCK_FREE, ATOMIC_LONG_LOCK_FREE, ATOMIC
// _LLONG_LOCK_FREE, and ATOMIC_POINTER_LOCK_FREE. They specify the lock-free status
// of the corresponding atomic types for the specified built-in types and their unsigned
// counterparts
// The only type that doesn’t provide an is_lock_free() member function is
// std::atomic_flag

// The standard atomic types are not copyable or assignable in the conventional
// sense, in that they have no copy constructors or copy assignment operators. They do,
// however, support assignment from and implicit conversion to the corresponding
// built-in types as well as direct load() and store() member functions, exchange(),
// compare_exchange_weak(), and compare_exchange_strong(). They also support the
// compound assignment operators where appropriate: +=, -=, *=, |=, and so on, and
// the integral types and std::atomic<> specializations for ++ and -- pointers support.

// -Store operations, which can have memory_order_relaxed, memory_order_release,
// or memory_order_seq_cst ordering
// -Load operations, which can have memory_order_relaxed, memory_order_consume,
// memory_order_acquire, or memory_order_seq_cst ordering
// -Read-modify-write operations, which can have memory_order_relaxed, memory_
// order_consume, memory_order_acquire, memory_order_release, memory_order
// _acq_rel, or memory_order_seq_cst ordering

// To lock the mutex,
// loop on test_and_set() until the old value is false, indicating that this thread set the
// value to true. Unlocking the mutex is simply a matter of clearing the flag.

// Implementation of a spinlock mutex using std::atomic_flag
class spin_lock_mutex
{
    std::atomic_flag flag_;
public:
    explicit spin_lock_mutex() = default;

    auto lock() noexcept
    {
        while (flag_.test_and_set(std::memory_order_acquire));
    }

    auto unlock() noexcept
    {
        flag_.clear(std::memory_order_release);
    }
};

//  the assignment operators they support return values (of
// the corresponding non-atomic type) rather than references. If a reference to the
// atomic variable was returned, any code that depended on the result of the assignment
// would then have to explicitly load the value, potentially getting the result of a modifi-
// cation by another thread. By returning the result of the assignment as a non-atomic
// value, you can avoid this additional load, and you know that the value obtained is the
// value stored.

// compare_exchange_weak() and compare_exchange_strong() member functions - The operation is said to succeed if the
// store was done (because
// the values were equal), and fail otherwise; the return value is true for success, and
// false for failure
// For compare_exchange_weak(), the store might not be successful even if the origi-
// nal value was equal to the expected value, in which case the value of the variable is
// unchanged and the return value of compare_exchange_weak() is false.
// This is most
// likely to happen on machines that lack a single compare-and-exchange instruction, if
// the processor can’t guarantee that the operation has been done atomically—possibly
// because the thread performing the operation was switched out in the middle of the
// necessary sequence of instructions and another thread scheduled in its place by the
// operating system where there are more threads than processors. This is called a spuri-
// ous failure, because the reason for the failure is a function of timing rather than the
// values of the variables.
// On the other hand, compare_exchange_strong() is guaranteed to return false
// only if the value wasn’t equal to the expected value. This can eliminate the need for
// loops like the one shown where you want to know whether you successfully changed a
// variable or whether another thread got there first.

// is_lock_free()

// If your UDT is the same size as (or smaller than) an int or a void*, most common
// platforms will be able to use atomic instructions for std::atomic<UDT>. Some plat-
// forms will also be able to use atomic instructions for user-defined types that are twice
// the size of an int or void*. These platforms are typically those that support a so-called
// double-word-compare-and-swap (DWCAS) instruction corresponding to the compare_
// exchange_xxx functions

// when instantiated with a user-defined type T, the interface
// of std::atomic<T> is limited to the set of operations available for std::atomic<bool>:
// load(), store(), exchange(), compare_exchange_weak(), compare_exchange_strong(),
// and assignment from and conversion to an instance of type T.

std::atomic<std::shared_ptr<int>> p;

auto process_global_data() noexcept
{
    auto local = std::atomic_load(&p);
    // process_global_data(local);
}

auto update_global_data()
{
    const auto local(std::make_shared<int>());
    std::atomic_store(&p, local);
}

// Reading and writing variables from different threads
static std::vector<int> data{0,};
std::atomic<bool> data_ready(false);

auto reader_thread()
{
    while (!data_ready.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // std::cout << "The answer = " << data[0] << "\n";
    std::cout << "The answer = " << data.at(0) << "\n";
}

auto writer_thread()
{
    data.emplace_back(42);
    data_ready = true;
}

// Order of evaluation of arguments to a function call is unspecified
auto foo(const int a, const int b)
{
    std::cout << a << "," << b << std::endl;
}

auto get_num() noexcept
{
    static auto i = 0;
    return ++i;
}

// SEQUENTIALLY CONSISTENT ORDERING - If all oper-
// ations on instances of atomic types are sequentially consistent, the behavior of a
// multithreaded program is as if all these operations were performed in some particular
// sequence by a single thread. operations can’t be reordered

// Sequential consistency implies a total ordering
std::atomic<bool> x, y;
std::atomic<int> z;

auto write_x() noexcept
{
    x.store(true, std::memory_order_seq_cst);
}

auto write_y() noexcept
{
    y.store(true, std::memory_order_seq_cst);
}

auto read_x_then_y() noexcept
{
    while (!x.load(std::memory_order_seq_cst));

    if (y.load(std::memory_order_seq_cst))
    {
        ++z;
    }
}

auto read_y_then_x() noexcept
{
    while (!y.load(std::memory_order_seq_cst));

    if (x.load(std::memory_order_seq_cst))
    {
        ++z;
    }
}

// Sequential consistency is the most straightforward and intuitive ordering, but it’s also
// the most expensive memory ordering because it requires global synchronization
// between all threads.

// RELAXED ORDERING - Operations on atomic types performed with relaxed ordering don’t participate in
// synchronizes-with relationships. Operations on the same variable within a single thread
// still obey happens-before relationships, but there’s almost no requirement on order-
// ing relative to other threads. The only requirement is that accesses to a single atomic
// variable from the same thread can’t be reordered; once a given thread has seen a par-
// ticular value of an atomic variable, a subsequent read by that thread can’t retrieve
// an earlier value of the variable.

// Relaxed operations have few ordering requirements
std::atomic<bool> x_1, y_1;
std::atomic<int> z_1;

auto write_x_then_y() noexcept
{
    x_1.store(true, std::memory_order_relaxed);
    y_1.store(true, std::memory_order_relaxed);
}

auto read_y_then_x_1() noexcept
{
    while (!y_1.load(std::memory_order_relaxed));

    if (x_1.load(std::memory_order_relaxed))
    {
        ++z_1;
    }
}

// Relaxed operations on multiple threads
std::atomic<int> x_2(0), y_2(0), z_2(0);
std::atomic<bool> go(false);
unsigned constexpr loop_count = 10;

struct read_values
{
    int x_2, y_2, z_2;
};

read_values values1[loop_count];
read_values values2[loop_count];
read_values values3[loop_count];
read_values values4[loop_count];
read_values values5[loop_count];

auto increment(const gsl::not_null<std::atomic<int>*>& var_to_inc, const gsl::span<read_values>& values) noexcept
{
    while (!go)
    {
        std::this_thread::yield();
    }
    for (unsigned i = 0; i < loop_count; ++i)
    {
        // values[i].x_2 = x.load(std::memory_order_relaxed);
        // values[i].y_2 = y.load(std::memory_order_relaxed);
        // values[i].z_2 = z.load(std::memory_order_relaxed);
        values.data()->x_2 = x_2.load(std::memory_order_relaxed);
        values.data()->y_2 = y_2.load(std::memory_order_relaxed);
        values.data()->z_2 = z_2.load(std::memory_order_relaxed);
        var_to_inc->store(i + 1, std::memory_order_relaxed);
        std::this_thread::yield();
    }
}

auto read_value(const gsl::span<read_values>& values) noexcept
{
    while (!go)
    {
        std::this_thread::yield();
    }
    for (unsigned i = 0; i < loop_count; ++i)
    {
        values.data()->x_2 = x_2.load(std::memory_order_relaxed);
        values.data()->y_2 = y_2.load(std::memory_order_relaxed);
        values.data()->z_2 = z_2.load(std::memory_order_relaxed);
        // values[i].x_2 = x_2.load(std::memory_order_relaxed);
        // values[i].y_2 = y_2.load(std::memory_order_relaxed);
        // values[i].z_2 = z_2.load(std::memory_order_relaxed);
        std::this_thread::yield();
    }
}

auto print(const gsl::span<read_values>& v)
{
    for (unsigned i = 0; i < loop_count; ++i)
    {
        if (i)
        {
            std::cout << ",";
        }
        // std::cout << "(" << v[i].x_2 << "," << v[i].y_2 << "," << v[i].z_2 << ")";
        std::cout << "(" << v.data()->x_2 << "," << v.data()->y_2 << "," << v.data()->z_2 << ")";
    }
    std::cout << std::endl;
}

// ACQUIRE-RELEASE ORDERING -  is a step up from relaxed ordering; there’s still no total order
// of operations, but it does introduce some synchronization. Under this ordering model,
// atomic loads are acquire operations (memory_order_acquire), atomic stores are release
// operations (memory_order_release), and atomic read-modify-write operations (such as fetch_add() or 
//     exchange()) are either acquire, release, or both (memory_order_
// acq_rel). Synchronization is pairwise between the thread that does the release and
// the thread that does the acquire. A release operation synchronizes-with an acquire operation
// that reads the value written. This means that different threads can still see different
// orderings, but these orderings are restricted. 

// Acquire-release doesn’t imply a total ordering
std::atomic<bool> x_3, y_3;
std::atomic<int> z_3;

auto write_x_ar() noexcept
{
    x_3.store(true, std::memory_order_release);
}

auto write_y_ar() noexcept
{
    y_3.store(true, std::memory_order_release);
}

auto read_x_then_y_ar() noexcept
{
    while (!x_3.load(std::memory_order_acquire));
    if (y_3.load(std::memory_order_acquire))
    {
        ++z_3;
    }
}

auto read_y_then_x_ar() noexcept
{
    while (!y_3.load(std::memory_order_acquire));
    if (x_3.load(std::memory_order_acquire))
    {
        ++z_3;
    }
}

// Acquire-release operations can impose ordering on relaxed operations
std::atomic<bool> x_4, y_4;
std::atomic<int> z_4;

auto write_x_then_y_a() noexcept
{
    x_4.store(true, std::memory_order_relaxed);
    y_4.store(true, std::memory_order_release);
}

auto read_y_then_x_a() noexcept
{
    while (!y_4.load(std::memory_order_acquire));
    if (x_4.load(std::memory_order_relaxed))
    {
        ++z_4;
    }
}

// You can still think about acquire-release ordering in terms of our men with note-
// pads in their cubicles, but you have to add more to the model. First, imagine that
// every store that’s done is part of some batch of updates, so when you call a man to tell
// him to write down a number, you also tell him which batch this update is part of:
// “Please write down 99, as part of batch 423.” For the last store in a batch, you tell this
// to the man too: “Please write down 147, which is the last store in batch 423.” The
// man in the cubicle will then duly write down this information, along with who gave
// him the value. This models a store-release operation. The next time you tell some-
// one to write down a value, you increase the batch number: “Please write down 41, as
// part of batch 424.”

// If you look at the definition of inter-thread happens-before back in section 5.3.2,
// one of the important properties is that it’s transitive: if A inter-thread happens before B
// and B inter-thread happens before C, then A inter-thread happens before C. This means that
// acquire-release ordering can be used to synchronize data across several threads, even
// when the “intermediate” threads haven’t touched the data.

// Transitive synchronization using acquire and release ordering
std::atomic<int> data_1[5];
std::atomic<bool> sync1(false), sync2(false);

auto thread_1() noexcept
{
    data_1[0].store(42, std::memory_order_relaxed);
    data_1[1].store(97, std::memory_order_relaxed);
    data_1[2].store(17, std::memory_order_relaxed);
    data_1[3].store(-141, std::memory_order_relaxed);
    data_1[4].store(2003, std::memory_order_relaxed);
    sync1.store(true, std::memory_order_release);
}

auto thread_2()noexcept
{
    while (!sync1.load(std::memory_order_acquire));
    sync2.store(true, std::memory_order_release);
}

auto thread_3()noexcept
{
    while (!sync2.load(std::memory_order_acquire));
    assert(data_1[0].load(std::memory_order_relaxed)==42);
    assert(data_1[1].load(std::memory_order_relaxed)==97);
    assert(data_1[2].load(std::memory_order_relaxed)==17);
    assert(data_1[3].load(std::memory_order_relaxed)==-141);
    assert(data_1[4].load(std::memory_order_relaxed)==2003);
}

// combine sync1 and sync2 in sync to use memory_order_acq_rel
std::atomic<bool> sync(false);
auto thread_1_1()noexcept
{
    data_1[0].store(42, std::memory_order_relaxed);
    data_1[1].store(97, std::memory_order_relaxed);
    data_1[2].store(17, std::memory_order_relaxed);
    data_1[3].store(-141, std::memory_order_relaxed);
    data_1[4].store(2003, std::memory_order_relaxed);
    sync.store(true, std::memory_order_release);
}

auto thread_2_1()noexcept
{
    auto expected=true;
    while (!sync.compare_exchange_strong(expected, 2, std::memory_order_acq_rel))
    {
        expected=true;
    }
}

auto thread_3_1()noexcept
{
    while(sync.load(std::memory_order_acquire) < 2);
    assert(data_1[0].load(std::memory_order_relaxed)==42);
    assert(data_1[1].load(std::memory_order_relaxed)==97);
    assert(data_1[2].load(std::memory_order_relaxed)==17);
    assert(data_1[3].load(std::memory_order_relaxed)==-141);
    assert(data_1[4].load(std::memory_order_relaxed)==2003);
}

// The concept of a data dependency is relatively straightforward: there is a data
// dependency between two operations if the second one operates on the result of the
// first. There are two new relations that deal with data dependencies: dependency-orderedbefore
// and carries-a-dependency-to.
// memory_order_consume - use when the atomic operation
// loads a pointer to some data

// Using std::memory_order_consume to synchronize data
struct X
{
    int i = 0;
    std::string s;
};
std::atomic<X*> p_x;
std::atomic<std::shared_ptr<X>> v;
std::atomic<int> a;

auto create_x()
{
    auto ptr = std::shared_ptr<X>();
    ptr->i = 42;
    ptr->s = "hello";
    
    a.store(99, std::memory_order_relaxed);
    v.store(ptr, std::memory_order_release);
}

auto create_x_raw_ptr()
{
    auto* ptr = new X;
    ptr->i = 42;
    ptr->s = "hello";
    
    a.store(99, std::memory_order_relaxed);
    p_x.store(ptr, std::memory_order_release);
}

auto use_x()
{
    auto ptr = std::make_shared<X>();
    while(!((ptr = v.load(std::memory_order_consume))))
    {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    assert(ptr->i==42);
    assert(ptr->s=="hello");
    assert(a.load(std::memory_order_relaxed)==99);
}

// int global_data[]={ … };
// std::atomic<int> index;
// void f()
// {
//     int i=index.load(std::memory_order_consume);
//     do_something_with(global_data[std::kill_dependency(i)]);
// }
// In real code, you should always use memory_order_acquire where you might be
// tempted to use memory_order_consume, and std::kill_dependency is unnecessary.

// Reading values from a queue with atomic operations
std::vector<int> queue_data;
std::atomic<int> count;
auto populate_queue()
{
    unsigned constexpr number_of_items = 20;
    queue_data.clear();
    for (unsigned i = 0; i < number_of_items; ++i)
    {
        queue_data.emplace_back(i);
    }
    count.store(number_of_items, std::memory_order_release);
}

auto consume_queue_items() noexcept
{
    while (true)
    {
        auto item_index = 0;
        if ((item_index=count.fetch_sub(1,std::memory_order_acquire) <= 0))
        {
            // wait_for_more_items();
            continue;
        }
        // process(queue_data[item_index-1]);
    }
}

// int main()
// {
//     std::thread a(populate_queue);
//     std::thread b(consume_queue_items);
//     std::thread c(consume_queue_items);
//     a.join();
//     b.join();
//     c.join();
// }

// fences - These are
// operations that enforce memory-ordering constraints without modifying any data and
// are typically combined with atomic operations that use the memory_order_relaxed
// ordering constraints.Fences are global operations and affect the ordering of other
// atomic operations in the thread that executed the fence. Fences are also commonly
// called memory barriers, and they get their name because they put a line in the code that
// certain operations can’t cross.

// Relaxed operations can be ordered with fences
std::atomic<bool> x_5,y_5;
std::atomic<int> z_5;
auto write_x_then_y_5() noexcept
{
    x_5.store(true,std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    y_5.store(true,std::memory_order_relaxed);
}
auto read_y_then_x_5() noexcept
{
    while(!y_5.load(std::memory_order_relaxed));
    std::atomic_thread_fence(std::memory_order_acquire);
    if(x_5.load(std::memory_order_relaxed))
    {
        ++z_5;
    }
}
// if an acquire operation sees the result of
// a store that takes place after a release fence, the fence synchronizes with that acquire
// operation; and if a load that takes place before an acquire fence sees the result of a
// release operation, the release operation synchronizes with the acquire fence.

// auto write_x_then_y()
// {
//     std::atomic_thread_fence(std::memory_order_release);
//     x.store(true,std::memory_order_relaxed);
//     y.store(true,std::memory_order_relaxed);
// }
//
// These two operations are no longer separated by the fence and so are no longer
// ordered. It’s only when the fence comes between the store to x and the store to y that
// it imposes an ordering. The presence or absence of a fence doesn’t affect any
// enforced orderings on happens-before relationships that exist because of other
// atomic operations.

// Enforcing ordering on non-atomic operations
bool x_6=false; //!!!!!
std::atomic<bool> y_6;
std::atomic<int> z_6;
auto write_x_then_y_6() noexcept
{
    x_6=true;
    std::atomic_thread_fence(std::memory_order_release);
    y_6.store(true,std::memory_order_relaxed);
}
auto read_y_then_x_6() noexcept
{
    while(!y_6.load(std::memory_order_relaxed));
    std::atomic_thread_fence(std::memory_order_acquire);
    if(x_6)
        ++z_6;
}

// lock() is an acquire operation on an internal memory
// location, and unlock() is a release operation on that same memory location.

// The following are the synchronization relationships provided by these facilities:

// -std::thread
//  The completion of the std::thread constructor synchronizes with the invocation
// of the supplied function or callable object on the new thread.
//  The completion of a thread synchronizes with the return from a successful call
// to join on the std::thread object that owns that thread.
//
// -std::mutex, std::timed_mutex, std::recursive_mutex, std::recursive_timed_mutex
//  All calls to lock and unlock, and successful calls to try_lock, try_lock_for, or
// try_lock_until, on a given mutex object form a single total order: the lock order
// of the mutex.
//  A call to unlock on a given mutex object synchronizes with a subsequent call to
// lock, or a subsequent successful call to try_lock, try_lock_for, or try_
// lock_until, on that object in the lock order of the mutex.
//  Failed calls to try_lock, try_lock_for, or try_lock_until do not participate
// in any synchronization relationships.
//
// -std::shared_mutex, std::shared_timed_mutex
//  All calls to lock, unlock, lock_shared, and unlock_shared, and successful calls
// to try_lock, try_lock_for, try_lock_until, try_lock_shared, try_lock_
// shared_for, or try_lock_shared_until, on a given mutex object form a single
// total order: the lock order of the mutex.
//  A call to unlock on a given mutex object synchronizes with a subsequent call to
// lock or shared_lock, or a successful call to try_lock, try_lock_for, try_
// lock_until, try_lock_shared, try_lock_shared_for, or try_lock_shared
// _until, on that object in the lock order of the mutex.
//  Failed calls to try_lock, try_lock_for, try_lock_until, try_lock_shared,
// try_lock_shared_for, or try_lock_shared_until do not participate in any
// synchronization relationships.
//
// -std::promise, std::future AND std::shared_future
//  The successful completion of a call to set_value or set_exception on a given
// std::promise object synchronizes with a successful return from a call to wait
// or get, or a call to wait_for or wait_until that returns std::future_status::
// ready on a future that shares the same asynchronous state as the promise.
// The destructor of a given std::promise object that stores an std::future_error
// exception in the shared asynchronous state associated with the promise synchronizes
// with a successful return from a call to wait or get, or a call to wait_for or
// wait_until that returns std::future_status::ready on a future that shares the
// same asynchronous state as the promise.
//
// -std::packaged_task, std::future AND std::shared_future
//  The successful completion of a call to the function call operator of a given
// std::packaged_task object synchronizes with a successful return from a call to
// wait or get, or a call to wait_for or wait_until that returns std::future
// _status::ready on a future that shares the same asynchronous state as the
// packaged task.
//  The destructor of a given std::packaged_task object that stores an std::
// future_error exception in the shared asynchronous state associated with the
// packaged task synchronizes with a successful return from a call to wait or get,
// or a call to wait_for or wait_until that returns std::future_status::ready
// on a future that shares the same asynchronous state as the packaged task.
//
// -std::async, std::future AND std::shared_future
//  The completion of the thread running a task launched via a call to std::async
// with a policy of std::launch::async synchronizes with a successful return from
// a call to wait or get, or a call to wait_for or wait_until that returns
// std::future_status::ready on a future that shares the same asynchronous
// state as the spawned task.
//  The completion of a task launched via a call to std::async with a policy of
// std::launch::deferred synchronizes with a successful return from a call to wait
// or get, or a call to wait_for or wait_until that returns std::future_status
// ::ready on a future that shares the same asynchronous state as the promise.
//
// -std::experimental::future, std::experimental::shared_future AND CONTINUATIONS
//  The event that causes an asynchronous shared state to become ready synchronizes
// with the invocation of a continuation function scheduled on that
// shared state.
//  The completion of a continuation function synchronizes with a successful
// return from a call to wait or get, or a call to wait_for or wait_until that
// returns std::future_status::ready on a future that shares the same asynchronous
// state as the future returned from the call to then that scheduled the
// continuation, or the invocation of any continuation scheduled on that future.
//
// -std::experimental::latch
//  The invocation of each call to count_down or count_down_and_wait on a given
// instance of std::experimental::latch synchronizes with the completion of
// each successful call to wait or count_down_and_wait on that latch.
//
// -std::experimental::barrier
//  The invocation of each call to arrive_and_wait or arrive_and_drop on a
// given instance of std::experimental::barrier synchronizes with the completion
// of each subsequent successful call to arrive_and_wait on that barrier.
//
// -std::experimental::flex_barrier
//  The invocation of each call to arrive_and_wait or arrive_and_drop on a given
// instance of std::experimental::flex_barrier synchronizes with the completion
// of each subsequent successful call to arrive_and_wait on that barrier.
//  The invocation of each call to arrive_and_wait or arrive_and_drop on a
// given instance of std::experimental::flex_barrier synchronizes with the
// subsequent invocation of the completion function on that barrier.
//  The return from the completion function on a given instance of std::
// experimental::flex_barrier synchronizes with the completion of each call to
// arrive_and_wait on that barrier that was blocked waiting for that barrier when
// the completion function was invoked.
//
// -std::condition_variable AND std::condition_variable_any
//  Condition variables do not provide any synchronization relationships. They are
// optimizations over busy-wait loops, and all the synchronization is provided by
// the operations on the associated mutex.

int main()
{
    std::atomic<bool> b;

    [[maybe_unused]]
        auto const xf = b.load(std::memory_order_acquire);

    b.store(true);
    x = b.exchange(false, std::memory_order_acq_rel);

    std::cout << "Hello World!\n";

    [[maybe_unused]]
        const std::atomic<bool> b_2;
    bool expected;
    b.compare_exchange_weak(expected, true,
                            std::memory_order::memory_order_acq_rel,
                            std::memory_order::memory_order_acquire);
    b.compare_exchange_weak(expected, true, std::memory_order_acq_rel);

    foo(get_num(), get_num());
    //--------------------------------------------------------------------//
    x = false;
    y = false;
    z = 0;
    std::thread a1(write_x);
    std::thread b_1(write_y);
    std::thread c(read_x_then_y);
    std::thread d(read_y_then_x);
    a1.join();
    b_1.join();
    c.join();
    d.join();
    // static_assert(z.load() != 0, "");
    //--------------------------------------------------------------------//
    x_1 = false;
    y_1 = false;
    z_1 = 0;
    std::thread a_1(write_x_then_y);
    std::thread bs(read_y_then_x);
    a_1.join();
    bs.join();
    assert(z_1.load()!=0);
    //--------------------------------------------------------------------//
    // std::thread t1(increment, &x, values1);
    // std::thread t2(increment, &y, values2);
    // std::thread t3(increment, &z, values3);
    // std::thread t4(read_value, values4);
    // std::thread t5(read_value, values5);
    // go = true;
    // t5.join();
    // t4.join();
    // t3.join();
    // t2.join();
    // t1.join();
    // print(values1);
    // print(values2);
    // print(values3);
    // print(values4);
    // print(values5);
    //--------------------------------------------------------------------//
    x_3 = false;
    y_3 = false;
    z_3 = 0;
    std::thread a_ar(write_x_ar);
    std::thread b_ar(write_y_ar);
    std::thread c_ar(read_x_then_y_ar);
    std::thread d_ar(read_y_then_x_ar);
    a_ar.join();
    b_ar.join();
    c_ar.join();
    d_ar.join();
    assert(z_3.load()!=0);
    //--------------------------------------------------------------------//
    x_4 = false;
    y_4 = false;
    z_4 = 0;
    std::thread a_a(write_x_then_y);
    std::thread b_a(read_y_then_x);
    a_a.join();
    b_a.join();
    assert(z_4.load()!=0);
    //--------------------------------------------------------------------//
    std::thread t1(create_x);
    std::thread t2(use_x);
    t1.join();
    t2.join();
    //--------------------------------------------------------------------//
    x_5=false;
    y_5=false;
    z_5=0;
    std::thread a_5(write_x_then_y);
    std::thread b_5(read_y_then_x);
    a_5.join();
    b_5.join();
    assert(z_5.load()!=0);
}
