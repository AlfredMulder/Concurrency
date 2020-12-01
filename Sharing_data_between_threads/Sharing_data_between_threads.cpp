// Sharing_data_between_threads.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <list>
#include <mutex>
#include <algorithm>
#include <deque>
#include <exception>
#include <map>
#include <memory>
#include <stack>
#include <shared_mutex>
#include <compare>

// Protecting a list with a mutex
std::list<int> some_list;
std::mutex some_mutex;

void add_to_list(const int new_value)
{
    std::lock_guard<std::mutex> guard(some_mutex);
    // std::scoped_lock guard(some_mutex);
    some_list.push_back(new_value);
}

bool list_contains(const int value_to_find)
{
    std::lock_guard<std::mutex> guard(some_mutex);
    // std::scoped_lock guard(some_mutex);
    return std::find(some_list.begin(), some_list.end(), value_to_find)
        != some_list.end();
}

// Accidentally passing out a reference to protected data
class some_data
{
    int a_ = 0;
    std::string b_;
public:
    static void do_something()
    {
    }
};

class data_wrapper
{
    some_data data_;
    std::mutex m_;
public:
    template <typename Function>
    void process_data(Function func)
    {
        std::lock_guard<std::mutex> l(m_);
        func(data_);
    }
};

some_data* unprotected;

void malicious_function(some_data& protected_data)
{
    unprotected = &protected_data;
}

data_wrapper x;

void foo()
{
    x.process_data(malicious_function);
    some_data::do_something();
}

// Don’t pass pointers and references to protected data outside the scope of the lock, whether by
// returning them from a function, storing them in externally visible memory, or passing them as
// arguments to user-supplied functions

// The interface to the std::stack container adapter
template <typename T, typename Container=std::deque<T>>
class stack
{
public:
    explicit stack(const Container&);
    explicit stack(Container&& = Container());
    template <class Alloc>
    explicit stack(const Alloc&);
    template <class Alloc>
    stack(const Container&, const Alloc&);
    template <class Alloc>
    stack(Container&&, const Alloc&);
    template <class Alloc>
    stack(stack&&, const Alloc&);
    bool empty() const;
    size_t size() const;
    T& top();
    T const& top() const;
    void push(T const&);
    void push(T&&);
    void pop();
    void swap(stack&&);

    template <class... Args>
    void emplace(Args&&... args);
};

// A fleshed-out class definition for a thread-safe stack
struct empty_stack : std::exception
{
    const char* what() const throw() override;
};

template <typename T>
class thread_safe_stack
{
    std::stack<T> data_;
    mutable std::mutex m_;
public:
    thread_safe_stack()
    {
    }

    thread_safe_stack(const thread_safe_stack& other)
    {
        std::lock_guard<std::mutex> lock(other.m_);
        data_ = other.data_;
    }

    thread_safe_stack& operator=(const thread_safe_stack&) = delete;

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m_);
        data_.push(std::move(new_value));
    }

    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock(m_);
        if (data_.empty()) throw empty_stack();
        std::shared_ptr<T> const res(std::make_shared<T>(data_.top()));
        data_.pop();
        return res;
    }

    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m_);
        if (data_.empty()) throw empty_stack();
        value = data_.top();
        data_.pop();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_);
        return data_.empty();
    }
};

// Using std::lock() and std::lock_guard in a swap operation to awoid DEADLOCK
class some_big_object
{
};

void swap(some_big_object& lhs, some_big_object& rhs);

class x
{
private:
    some_big_object some_detail_;
    std::mutex m_;
public:
    explicit x(some_big_object const& sd) noexcept: some_detail_(sd)
    {
    }

    friend void swap(x& lhs, x& rhs)
    {
        if (&lhs == &rhs)
        {
            return;
        }
        std::lock(lhs.m_, rhs.m_);
        std::lock_guard<std::mutex> lock_a(lhs.m_, std::adopt_lock);
        std::lock_guard<std::mutex> lock_b(rhs.m_, std::adopt_lock);
        swap(lhs.some_detail_, rhs.some_detail_);
    }

    // friend void swap(X& lhs, X& rhs)
    // {
    //     if (&lhs == &rhs)
    //         return;
    //     std::scoped_lock guard(lhs.m, rhs.m);
    //     swap(lhs.some_detail, rhs.some_detail);
    // }
};

// Further guidelines for avoiding deadlock
// Deadlock doesn’t only occur with locks, although that’s the most frequent cause; you
// can create deadlock with two threads and no locks by having each thread call join()
// on the std::thread object for the other. In this case, neither thread can make prog-
// ress because it’s waiting for the other to finish, like the children fighting over their toy.
// This simple cycle can occur anywhere that a thread can wait for another thread to per-
// form some action if the other thread can simultaneously be waiting for the first
// thread, and it isn’t limited to two threads: a cycle of three or more threads will still
// cause deadlock. The guidelines for avoiding deadlock all boil down to one idea: don’t
// wait for another thread if there’s a chance it’s waiting for you. The individual guide-
// lines provide ways of identifying and eliminating the possibility that the other thread
// is waiting for you.
// AVOID NESTED LOCKS
// The first idea is the simplest: don’t acquire a lock if you already hold one. If you stick
// to this guideline, it’s impossible to get a deadlock from the lock usage alone because
// each thread only ever holds a single lock. You could still get deadlock from other
// things (like the threads waiting for each other), but mutex locks are probably the
// most common cause of deadlock. If you need to acquire multiple locks, do it as a sin-
// gle action with std::lock in order to acquire them without deadlock.
// AVOID CALLING USER-SUPPLIED CODE WHILE HOLDING A LOCK
// This is a simple follow-on from the previous guideline. Because the code is user-
// supplied, you have no idea what it could do; it could do anything, including acquiring
// a lock. If you call user-supplied code while holding a lock, and that code acquires a
// lock, you’ve violated the guideline on avoiding nested locks and could get deadlock.
// Sometimes this is unavoidable; if you’re writing generic code, such as the stack in
// section 3.2.3, every operation on the parameter type or types is user-supplied code. In
// this case, you need a new guideline.
// ACQUIRE LOCKS IN A FIXED ORDER
// If you absolutely must acquire two or more locks, and you can’t acquire them as a sin-
// gle operation with std::lock, the next best thing is to acquire them in the same order
// in every thread. I touched on this in section 3.2.4 as one way of avoiding deadlock
// when acquiring two mutexes: the key is to define the order in a way that’s consistent
// between threads. In some cases, this is relatively easy. For example, look at the stack
// from section 3.2.3—the mutex is internal to each stack instance, but the operations
// on the data items stored in a stack require calling user-supplied code. You can, how-
// ever, add the constraint that none of the operations on the data items stored in the
// stack should perform any operation on the stack itself. This puts the burden on the
// user of the stack, but it’s rather uncommon for the data stored in a container to access
// that container, and it’s quite apparent when this is happening, so it’s not a particularly
// difficult burden to carry.
// In other cases, this isn’t so straightforward, as you discovered with the swap opera-
// tion in section 3.2.4. At least in that case you could lock the mutexes simultaneously,
// but that’s not always possible. If you look back at the linked list example from sec-
// tion 3.1, you’ll see that one possibility for protecting the list is to have a mutex per
// node. Then, in order to access the list, threads must acquire a lock on every node
// they’re interested in. For a thread to delete an item, it must then acquire the lock on
// three nodes: the node being deleted and the nodes on either side, because they’re all
// being modified in some way. Likewise, to traverse the list, a thread must keep hold of
// the lock on the current node while it acquires the lock on the next one in the
// sequence, in order to ensure that the next pointer isn’t modified in the meantime.
// Once the lock on the next node has been acquired, the lock on the first can be
// released because it’s no longer necessary.
// This hand-over-hand locking style allows multiple threads to access the list, pro-
// vided each is accessing a different node. But in order to prevent deadlock, the
// nodes must always be locked in the same order: if two threads tried to traverse the
// list in opposite orders using hand-over-hand locking, they could deadlock with
// each other in the middle of the list. If nodes A and B are adjacent in the list, the
// thread going one way will try to hold the lock on node A and try to acquire the lock
// on node B. A thread going the other way would be holding the lock on node B and
// trying to acquire the lock on node A—a classic scenario for deadlock, as shown in
// figure 3.2.
// Likewise, when deleting node B that lies between nodes A and C, if that thread
// acquires the lock on B before the locks on A and C, it has the potential to deadlock
// with a thread traversing the list. Such a thread would try to lock either A or C first
// (depending on the direction of traversal) but would then find that it couldn’t obtain a
// lock on B because the thread doing the deleting was holding the lock on B and trying
// to acquire the locks on A and C.
// USE A LOCK HIERARCHY
// Although this is a particular case of defining lock ordering, a lock hierarchy can pro-
// vide a means of checking that the convention is adhered to at runtime. The idea is that
// you divide your application into layers and identify all the mutexes that may be locked
// in any given layer. When code tries to lock a mutex, it isn’t permitted to lock that mutex
// if it already holds a lock from a lower layer. You can check this at runtime by assigning
// layer numbers to each mutex and keeping a record of which mutexes are locked by
// each thread. This is a common pattern, but the C++ Standard Library does not provide
// direct support for it, so you will need to write a custom hierarchical_mutex mutex
// type, the code for which is shown in listing 3.8.

// A simple hierarchical mutex
class hierarchical_mutex
{
    std::mutex internal_mutex_;
    unsigned long const hierarchy_value_;
    unsigned long previous_hierarchy_value_;
    static thread_local unsigned long this_thread_hierarchy_value_;

    void check_for_hierarchy_violation() const
    {
        if (this_thread_hierarchy_value_ <= hierarchy_value_)
        {
            throw std::logic_error("mutex hierarchy violated");
        }
    }

    void update_hierarchy_value()
    {
        previous_hierarchy_value_ = this_thread_hierarchy_value_;
        this_thread_hierarchy_value_ = hierarchy_value_;
    }

public:
    explicit hierarchical_mutex(const unsigned long value) noexcept:
        hierarchy_value_(value),
        previous_hierarchy_value_(0)
    {
    }

    void lock()
    {
        check_for_hierarchy_violation();
        internal_mutex_.lock();
        update_hierarchy_value();
    }

    void unlock()
    {
        if (this_thread_hierarchy_value_ != hierarchy_value_)
        {
            throw std::logic_error("mutex hierarchy violated");
        }
        this_thread_hierarchy_value_ = previous_hierarchy_value_;
        internal_mutex_.unlock();
    }

    bool try_lock()
    {
        check_for_hierarchy_violation();
        if (!internal_mutex_.try_lock())
        {
            return false;
        }
        update_hierarchy_value();
        return true;
    }
};

thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value_(ULONG_MAX);

// Using a lock hierarchy to prevent deadlock
hierarchical_mutex high_level_mutex(10000);
hierarchical_mutex low_level_mutex(5000);
hierarchical_mutex other_mutex(6000);

int do_low_level_stuff()
{
    return 1;
}

int low_level_func()
{
    std::lock_guard<hierarchical_mutex> lk(low_level_mutex);
    return do_low_level_stuff();
}

void high_level_stuff(int some_param)
{
    some_param = 0;
    std::cout << some_param << '\n';
}

void high_level_func()
{
    std::lock_guard<hierarchical_mutex> lk(high_level_mutex);
    high_level_stuff(low_level_func());
}

void thread_a()
{
    high_level_func();
}

void do_other_stuff()
{
}

void other_stuff()
{
    high_level_func();
    do_other_stuff();
}

void thread_b()
{
    std::lock_guard<hierarchical_mutex> lk(other_mutex);
    other_stuff();
}

// Once you’ve designed your code to avoid deadlock, std::lock() and std::
// lock_guard cover most of the cases of simple locking, but sometimes more flexibility
// is required. For those cases, the Standard Library provides the std::unique_lock
// template. Like std::lock_guard, this is a class template parameterized on the mutex
// type, and it also provides the same RAII-style lock management as std::lock_guard,
// but with a bit more flexibility.

// Using std::lock() and std::unique_lock in a swap operation
class some_big_object;
void swap(some_big_object& lhs, some_big_object& rhs);

class X
{
private:
    some_big_object some_detail_;
    std::mutex m_;
public:
    explicit X(some_big_object const& sd) noexcept: some_detail_(sd)
    {
    }

    friend void swap(X& lhs, X& rhs)
    {
        if (&lhs == &rhs)
        {
            return;
        }
        std::unique_lock<std::mutex> lock_a(lhs.m_, std::defer_lock);
        std::unique_lock<std::mutex> lock_b(rhs.m_, std::defer_lock);
        std::lock(lock_a, lock_b);
        swap(lhs.some_detail_, rhs.some_detail_);
    }
};

// Transferring mutex ownership between scopes
// std::unique_lock<std::mutex> get_lock()
// {
//     extern std::mutex some_mutex;
//     std::unique_lock<std::mutex> lk(some_mutex);
//     prepare_data();
//     return lk;
// }
//
// void process_data()
// {
//     std::unique_lock<std::mutex> lk(get_lock());
//     do_something();
// }

// In general, a lock should be held for only the mini-
// mum possible time needed to perform the required operations. This also means that time-
// consuming operations such as acquiring another lock (even if you know it won’t
// deadlock) or waiting for I/O to complete shouldn’t be done while holding a lock
// unless absolutely necessary.

// Locking one mutex at a time in a comparison operator
class y
{
    int some_detail_;
    mutable std::mutex m_;

    int get_detail() const
    {
        std::lock_guard<std::mutex> lock_a(m_);
        return some_detail_;
    }

public:
    explicit y(const int sd) noexcept: some_detail_(sd)
    {
    }

    friend bool operator==(y const& lhs, y const& rhs)
    {
        if (&lhs == &rhs)
        {
            return true;
        }
        auto const lhs_value = lhs.get_detail();
        auto const rhs_value = rhs.get_detail();
        return lhs_value == rhs_value;
    }
};

// Thread-safe lazy initialization using a mutex
std::shared_ptr<int> resource_ptr;
std::mutex resource_mutex;

void foo_1()
{
    std::unique_lock<std::mutex> lk(resource_mutex);
    if (!resource_ptr)
    {
        resource_ptr.reset();
    }
    lk.unlock();
    // resource_ptr->do_something();
}

void undefined_behaviour_with_double_checked_locking()
{
    if (!resource_ptr)
    {
        std::lock_guard<std::mutex> lk(resource_mutex);
        if (!resource_ptr)
        {
            resource_ptr.reset();
        }
    }
    // resource_ptr->do_something();
}

// Thread-safe lazy initialization of a class member using std::call_once

// std::call_once - Rather than locking a mutex and explicitly checking the pointer,
// every thread can use std::call_once, safe in the knowledge that the pointer will
// have been initialized by some thread (in a properly synchronized fashion) by the
// time std::call_once returns. The necessary synchronization data is stored in the
// std::once_flag instance; each instance of std::once_flag corresponds to a different
// initialization. Use of std::call_once will typically have a lower overhead than using a
// mutex explicitly, especially when the initialization has already been done, so it should be
// used in preference where it matches the required functionality. 
// class z
// {
//     connection_info connection_details;
//     connection_handle connection;
//     std::once_flag connection_init_flag;
//
//     void open_connection()
//     {
//         connection = connection_manager.open(connection_details);
//     }
//
// public:
//     explicit z(connection_info const& connection_details_) noexcept:
//         connection_details(connection_details_)
//     {
//     }
//
//     void send_data(data_packet const& data)
//     {
//         std::call_once(connection_init_flag, &X::open_connection, this);
//         connection.send_data(data);
//     }
//
//     data_packet receive_data()
//     {
//         std::call_once(connection_init_flag, &X::open_connection, this);
//         return connection.receive_data();
//     }
// };


// Multiple threads can then call get_my_class_instance() safely, without having to
// worry about race conditions on the initialization.
class my_class
{
};

my_class& get_my_class_instance()
{
    static my_class instance; //! static
    return instance;
}

// Protecting a data structure with std::shared_mutex
class dns_entry
{
};

class dns_cache
{
    std::map<std::string, dns_entry> entries_;
    mutable std::shared_mutex entry_mutex_;
public:
    dns_entry find_entry(std::string const& domain) const
    {
        std::shared_lock<std::shared_mutex> lk(entry_mutex_);
        auto const it =
            entries_.find(domain);
        return (it == entries_.end()) ? dns_entry() : it->second;
    }

    void update_or_add_entry(std::string const& domain,
                             dns_entry const& dns_details)
    {
        std::lock_guard<std::shared_mutex> lk(entry_mutex_);
        entries_[domain] = dns_details;
    }
};

// std::recursive_mutex - With std::mutex, it’s an error for a thread to try to lock a mutex it already owns, and
// attempting to do so will result in undefined behavior. But in some circumstances it
// would be desirable for a thread to reacquire the same mutex several times without
// having first released it.

int main()
{
    std::cout << "Hello World!\n";
}
