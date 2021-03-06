#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>
#include <execution>
#include <ranges>


struct user
{
    std::string name;

    auto get_name_callback()
    {
        return [this](const std::string& b)
        {
            return name + b;
        };
    }

    [[nodiscard]]
    auto get_safe_name_callback() const
    {
        return [*this](const std::string& b) // *this!
        {
            return name + b;
        };
    }
};

auto callback_test()
{
    auto p_john = std::make_unique<user>(user{"John"});
    const auto name_callback = p_john->get_name_callback();
    p_john.reset(); // Destroying object, trying to access a deleted memory region

    const auto new_name = name_callback(" is Super!");
    std::cout << new_name << '\n';
}

// One note: the copy is made when you create a lambda object, not at the place where you invoke it!
auto callback_safe_test()
{
    auto p_john = std::make_unique<user>(user{"John"});
    auto name_callback = p_john->get_safe_name_callback();
    p_john.reset(); // With *this even if the original object is destroyed, the lambda will contain a safe copy

    const auto new_name = name_callback(" is Super!");
    std::cout << new_name << '\n';
}

int main()
{
    std::vector<int> numbers(100);

    std::thread iota_thread([&numbers](const int start_arg)
                            {
                                std::iota(numbers.begin(), numbers.end(), start_arg);
                                std::cout << "from: " << std::this_thread::get_id() << " thread id\n";
                            }, 10
    );

    iota_thread.join();
    std::cout << "numbers in main (id " << std::this_thread::get_id() << "):\n";
    for (auto& num : numbers)
    {
        std::cout << num << ", ";
    }
    std::cout << "Hello World!\n";

    auto counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(5); //!!!
    for (auto i = 0; i < 5; ++i)
    {
        threads.emplace_back([&counter]()
        {
            for (auto j = 0; j < 100; ++j)
            {
                ++counter;
                --counter;
                ++counter;
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    std::cout << counter << std::endl;


    std::atomic counter1 = 0;

    for (auto i = 0; i < 5; ++i)
    {
        threads.emplace_back([&counter1]()
        {
            for (auto j = 0; j < 100; ++j)
            {
                counter1.fetch_add(1);
            }
        });
    }

    for (std::vector<std::thread> threads1; auto& thread : threads1)
    {
        thread.join();
    }

    std::cout << counter1.load() << std::endl;


    std::vector<int> numbers1(100);

    auto iota_future1 = std::async(std::launch::async, [&numbers1, start_arg = 10]()
    {
        std::iota(numbers1.begin(), numbers1.end(), start_arg);
        std::cout << "calling from: " << std::this_thread::get_id()
            << " thread id\n";
    });

    iota_future1.get(); // make sure we get the results...
    std::cout << "numbers in main (id " << std::this_thread::get_id() << "):\n";
    for (const auto& num : numbers)
    {
        std::cout << num << ", ";
    }

    auto iota_future_vector_pass = std::async(std::launch::async,
                                              [start_arg = 10]()
                                              {
                                                  std::vector<int> numbers_2(100);
                                                  // numbers_2 | std::ranges::for_each;
                                                  std::iota(numbers_2.begin(), numbers_2.end(), start_arg);
                                                  std::cout << "calling from: "
                                                      << std::this_thread::get_id() << " thread id\n";
                                                  return numbers_2;
                                              });
    auto vec = iota_future_vector_pass.get();

    // -sequenced_policy - It
    // is an execution policy type used as a unique type to disambiguate parallel algorithm overloading and
    // require that a parallel algorithmís execution
    // not be
    // paralleled.
    //
    // - parallel_policy - It
    // is an execution policy type used as a unique type to disambiguate parallel algorithm overloading and indicate
    // that a parallel algorithmís execution may be paralleled.
    //
    // -parallel_unsequenced_policy - It
    // is an execution policy type used as a unique type to disambiguate parallel algorithm overloading and
    // indicate that a parallel algorithmís execution may be paralleled and vectorized.
    std::vector<int> vec2(1000);
    std::iota(vec2.begin(), vec2.end(), 0);
    std::vector<int> output;
    std::for_each(std::execution::par, vec2.begin(), vec2.end(),
                  [&output](int& elem)
                  {
                      if (elem % 2 == 0)
                      {
                          output.emplace_back(elem);
                      }
                  });
}
