// A semaphore is a data structure with a queue and a counter. The counter is initialized to a value equal
// to or greater than zero. It supports the two operations wait and signal. wait acquires the semaphore and
// decreases the counter; it blocks the thread acquiring the semaphore if the counter is zero. signal releases
// the semaphore and increases the counter. Blocked threads are added to the queue to avoid starvation.
//
// Originally, a semaphore is a railway signal.

// Semaphores are typically used in sender-receiver workflows. For example, initializing the semaphore sem
// with 0 will block the receivers sem.acquire() call until the sender calls sem.release(). Consequently,
// the receiver waits for the notification of the sender. A one-time synchronization of threads can easily be
// implemented using semaphores.

#include <iostream>
#include <semaphore>
#include <thread>
#include <vector>

std::vector<int> my_vec{};

std::counting_semaphore<1> prepare_signal(0); // (1.1)

void prepare_work()
{
    my_vec.insert(my_vec.end(), {0, 1, 0, 3});
    std::cout << "Sender: Data prepared." << '\n';
    prepare_signal.release(); // (2.1)
}

void complete_work()
{
    std::cout << "Waiter: Waiting for data." << '\n';
    prepare_signal.acquire(); // (3.1)
    my_vec[2] = 2;
    std::cout << "Waiter: Complete the work." << '\n';
    for (auto i : my_vec)
    {
        std::cout << i << " ";
    }
    std::cout << '\n';
}

// Ping Pong
std::counting_semaphore<1> signal2_ping(0); // (1.2)
std::counting_semaphore<1> signal2_pong(0); // (2.3)

std::atomic<int> counter{};
constexpr int count_limit = 1'000'000;

void ping()
{
    while (counter <= count_limit)
    {
        signal2_ping.acquire(); // (5.2)
        ++counter;
        signal2_pong.release();
    }
}

void pong()
{
    while (counter < count_limit)
    {
        signal2_pong.acquire();
        signal2_ping.release(); // (3.2)
    }
}

int main()
{
    std::cout << '\n';

    std::jthread t11(prepare_work);
    std::jthread t21(complete_work);

    std::cout << '\n';

    // Ping Pong
    const auto start = std::chrono::system_clock::now();

    signal2_ping.release(); // (4.2)
    std::jthread t12(ping);
    std::jthread t22(pong);

    const std::chrono::duration<double> dur = std::chrono::system_clock::now() -
        start;
    std::cout << "Duration: " << dur.count() << " seconds" << '\n';
}
