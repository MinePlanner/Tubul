//
// Created by Carlos Acosta on 11-04-23.
//

#include <gtest/gtest.h>
#include "tubul.h"
#include <array>

std::atomic_size_t g_counter;
std::array<char, 1024> g_buffer;

size_t fib(int n) {
    size_t f0 = 0, f1 = 1, f2, i;
    if( n == 0)
        return f0;
    for(i = 2; i <= n; i++) {
        f2 = f0 + f1;
        f0 = f1;
        f1 = f2;
    }
    using namespace  std::chrono_literals;
    std::this_thread::sleep_for(0.1s);
    return f1;
}
void sleep_log(int ms)
{
    {
        std::stringstream out;
        out << "Thread (" << std::this_thread::get_id() << ") : sleeping for" << ms << "ms" << std::endl;
        auto text = out.str();
        size_t textSize = text.size();
        auto prev = g_counter.fetch_add(textSize, std::memory_order_seq_cst);
        std::copy( text.begin(), text.end(), g_buffer.data()+prev);
    }
    std::this_thread::sleep_for(std::chrono::microseconds(ms));
    {
        std::stringstream out;
        out << "Thread (" << std::this_thread::get_id() << ") : woke up! " << std::endl;
        auto text = out.str();
        size_t textSize = text.size();
        auto prev = g_counter.fetch_add(textSize, std::memory_order_seq_cst);
        std::copy( text.begin(), text.end(), g_buffer.data()+prev);
    }

}

TEST(TUBULThread, testPool) {

    g_counter.store(0);

    TU::ThreadPool pool(1);
    auto work = [](int f) {
        std::stringstream out;
        out << "Fibo("<< f << ") : " << fib(f) << std::endl;
        auto text = out.str();
        size_t textSize = text.size();
        auto prev = g_counter.fetch_add(textSize, std::memory_order_seq_cst);
        std::copy( text.begin(), text.end(), g_buffer.data()+prev);
    };
    pool.pushTask(work, 90);
    pool.pushTask(work, 70);
    pool.pushTask(work, 80);
    pool.pushTask(work, 40);
    pool.pushTask(work, 50);
    pool.waitForTasks();

    std::string res(g_buffer.data(), g_counter.load());
    std::cout << res << std::endl;
    std::cout << "Esto es despues" << std::endl;
}

TEST(TUBULThread, testPool2) {
    g_counter.store(0);
    TU::ThreadPool pool(4);
    auto work = [](int f) {
        std::stringstream out;
        out << "Fibo("<< f << ") : " << fib(f) << std::endl;
        auto text = out.str();
        size_t textSize = text.size();
        auto prev = g_counter.fetch_add(textSize, std::memory_order_seq_cst);
        std::copy( text.begin(), text.end(), g_buffer.data()+prev);
    };
    pool.pushTask(work, 90);
    pool.pushTask(work, 70);
    pool.pushTask(work, 80);
    pool.pushTask(work, 40);
    pool.pushTask(work, 50);
    pool.pushTask(work, 90);
    pool.pushTask(work, 90);
    pool.pushTask(work, 70);
    pool.pushTask(work, 80);
    pool.pushTask(work, 40);
    pool.pushTask(work, 50);
    pool.pushTask(work, 70);
    pool.pushTask(work, 80);
    pool.pushTask(work, 40);
    pool.pushTask(work, 50);
    pool.pushTask(work, 90);
    pool.pushTask(work, 70);
    pool.pushTask(work, 80);
    pool.pushTask(work, 40);
    pool.pushTask(work, 50);
    pool.waitForTasks();
    std::string res(g_buffer.data(), g_counter.load());
    std::cout << res << std::endl;
    std::cout << "Esto es despues" << std::endl;
}

TEST(TUBULThread, testSleepyPool) {
    for (std::integral auto i: TU::irange(40000)) {
        std::cout << "Iteration " << i << std::endl;
        g_counter.store(0);
        TU::ThreadPool pool(4);
        pool.pushTask(sleep_log, 5000);  pool.pushTask(sleep_log, 5000);
        pool.pushTask(sleep_log, 15000); pool.pushTask(sleep_log, 5000);
        pool.pushTask(sleep_log, 5000);  pool.pushTask(sleep_log, 20000);
        pool.pushTask(sleep_log, 7500);  pool.pushTask(sleep_log, 7500);
        pool.pushTask(sleep_log, 5000);  pool.pushTask(sleep_log, 5000);
        pool.pushTask(sleep_log, 15000); pool.pushTask(sleep_log, 5000);
        pool.pushTask(sleep_log, 5000);  pool.pushTask(sleep_log, 20000);
        pool.pushTask(sleep_log, 7500);  pool.pushTask(sleep_log, 7500);
        pool.waitForTasks();
        std::string res(g_buffer.data(), g_counter.load());
        std::cout << res << std::endl;
        std::cout << "Iteracion " << i << " terminada " << std::endl;
    }



}
