// AndroidOS - Loopers as a message queue and one or more Handler types, depending on the specific message. 

// Qt Framework - Also as message queue upon which the signal and slot mechanism is built to signal across thread
// boundaries.

// *Windowing system**s with a UI-Thread and event-callbacks.

// Most Game-Loops in game engines (even though they might not be reusable components), which attach to the main thread
// and hook into operating system specific event systems - the classic WINAPI-hooks

// threads are not for free.
//
// There will at least be a stack allocated for the thread. There is the management of all threads to be done
// with respect to the governing process in kernel space and the operating system implementation. Also, when having a
// large number of threads, scaleability, will almost certainly become a critical factor, regarding the huge amount of
// permutations of target systems.
//
// And even worse, the specific expression of a thread is dependent on the operation system and the threading library
// used. 

// Finally, we hardly have any control about the threads and its execution.
//
//     Are things executed in proper order?
//     Who maintains the threads?
//     How to receive results from asynchronous execution?
//     What about task priorities or delayed insertions?
//     Maybe even event-driven dispatching?

// Loopers

// Loopers, in its core, are objects, which contain or are attached to a thread with a conditional infinite loop, which
// runs as long as the abort-criteria is unmet. Within this loop, arbitrary actions can be performed.
// Usually, a methodology like start, run and stop are provided.

#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <ostream>
#include <queue>
#include <stdexcept>
#include <thread>

class clooper
{
public: // Ctor/Dtor

    using runnable = std::function<void()>;

    // Imagine the dispatcher to be a bouncer in front of the looper.
    // It will accept a task but will manage insertion into the working-queue.
    // This way, some fancy usage scenarios can be enabled, e.g. delayed execution or immediate posting.
    class c_dispatcher
    {
        friend class clooper; // Allow the looper to access the private constructor.

    public:
        // Yet to be defined method, which will post the runnable 
        // into the looper-queue.
        bool post(runnable&& runnable) const
        {
            return m_assigned_looper_.post(std::move(runnable));
        }

    private: // construction, since we want the looper to expose it's dispatcher exclusively!
        explicit c_dispatcher(clooper& a_looper)
            : m_assigned_looper_(a_looper)
        {
        }

    private:
        // Store a reference to the attached looper in order to 
        // emplace tasks into the queue.
        clooper& m_assigned_looper_;
    };

    clooper()
        : m_running_(false)
          , m_abort_requested_(false)
          , m_runnables_mutex_()
          , m_runnables_()
          , m_dispatcher_(std::make_shared<c_dispatcher>(c_dispatcher(*this)))
    {
    } // Copy defined, Move to be implemented

    ~clooper()
    {
        abort_and_join();
    }

public: // Methods

    bool running() const
    {
        return m_running_.load();
    }

    // To be called from, once the looper should start looping through.
    bool run()
    {
        try
        {
            m_thread_ = std::thread(&clooper::run_func, this);
        }
        catch (...)
        {
            return false;
        }
        return true;
    }

    void stop()
    {
        abort_and_join();
    }

    std::shared_ptr<c_dispatcher> get_dispatcher() const
    {
        return m_dispatcher_;
    }

private: // Methods

    // Conditionally-infinite loop doing sth. iteratively
    void run_func()
    {
        m_running_.store(true);

        while (false == m_abort_requested_.load())
        {
            try
            {
                using namespace std::chrono_literals;
                auto r = next();
                if (nullptr != r)
                {
                    r();
                }
                else
                {
                    std::this_thread::sleep_for(1ms);
                }
            }
            catch (std::runtime_error& e)
            {
                // Some more specific
                std::cerr << e.what() << std::endl;
            }
            catch (...)
            {
                // Make sure that nothing leaves the thread for now...
            }
        }

        m_running_.store(false);
    }

    // Shared implementation of exiting the loop-function and joining 
    // to the main thread.
    void abort_and_join()
    {
        m_abort_requested_.store(true);
        if (m_thread_.joinable())
        {
            m_thread_.join();
        }
    }

    using runnable = std::function<void()>;

    runnable next()
    {
        std::lock_guard guard{m_runnables_mutex_}; // CTAD, C++17 with {} braces

        if (m_runnables_.empty())
        {
            return nullptr;
        }

        auto runnable = m_runnables_.front();
        m_runnables_.pop();

        return runnable;
    }

    bool post(runnable&& runnable)
    {
        if (! running())
        {
            // Deny insertion
            std::cout << "Denying insertion, as the looper is not running.\n";
            return false;
        }

        try
        {
            std::lock_guard guard{m_runnables_mutex_};

            m_runnables_.push(std::move(runnable));
        }
        catch (...)
        {
            return false;
        }

        return true;
    }

private: // Members
    std::thread m_thread_;

    std::atomic_bool m_running_;

    std::atomic_bool m_abort_requested_;

    std::recursive_mutex m_runnables_mutex_;

    std::queue<runnable> m_runnables_;

    std::shared_ptr<c_dispatcher> m_dispatcher_;
};

int main(int argc, char* argv[])
{
    auto looper = std::make_unique<clooper>();

    std::cout << "Starting looper" << std::endl;
    // To start and run
    looper->run();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto dispatcher = looper->get_dispatcher();

    std::cout << "Adding tasks" << std::endl;

    for (uint32_t k = 0; k < 500; ++k)
    {
        auto const task = [k]()
        {
            std::cout << "Invocation " << k
                << ": Hello, I have been executed asynchronously on the looper for " << (k + 1)
                << " times." << std::endl;
        };

        dispatcher->post(std::move(task));
    }

    std::cout << "Waiting 5 seconds for completion" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "Stopping looper" << std::endl;
    // To stop it and clean it up
    dispatcher = nullptr;
    looper->stop();
    looper = nullptr;
}
