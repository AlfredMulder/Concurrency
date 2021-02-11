#include <iostream>
#include <thread>
#include <future>

void task_1()
{
    for (auto i = 0; i < 100; ++i)
    {
        std::cout << "Task 1: Thread with ID " << std::this_thread::get_id() << " in loop " << i <<  std::endl;
    }
}

void task_2()
{
    for (auto i = 0; i < 100; ++i)
    {
        std::cout << "Task 2: Thread with ID " << std::this_thread::get_id() << " in loop " << i << std::endl;
    }
}

int main()
{
    // std::thread worker1(task_1);
    // std::thread worker2(task_2);
    //
    // worker1.join();
    // worker2.join();

    std::cout << "Main Thread ID: " << std::this_thread::get_id() << std::endl;

    // Async on one thread(asynchronously)
    // std::async(task_1);
    // std::async(task_2);

    // With Future, two threads(concurrently)
    // auto o1 = std::async(task_1);
    // auto o2 = std::async(task_2);

    // Lazy evaluation, asynchronous, one thread
    // auto o1 = std::async(std::launch::deferred, task_1);
    // auto o2 = std::async(std::launch::deferred, task_2);
    //
    // o1.get();
    // o2.get();

    // The std::packaged_task is one of the possible ways of associating a task with an std::future.
    // full concurrent result because we used std::thread
    std::packaged_task<void()> job1(task_1);
    std::packaged_task<void()> job2(task_2);

    auto o1 = job1.get_future();
    auto o2 = job2.get_future();

    std::thread worker1(std::move(job1));
    std::thread worker2(std::move(job2));

    worker1.join();
    worker2.join();

    return 0;
}
