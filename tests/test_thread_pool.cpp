//
// Created by Carlos Acosta on 11-04-23.
//

#include <gtest/gtest.h>
#include "tubul.h"
#include <array>

size_t fib(int n) {
    size_t f0 = 0, f1 = 1, f2, i;
    if( n == 0)
        return f0;
    for(i = 2; i <= n; i++) {
        f2 = f0 + f1;
        f0 = f1;
        f1 = f2;
    }
    return f1;
}
std::atomic_size_t g_counter;
std::array<char, 1024> g_buffer;

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
    pool.push_task(work, 90);
    pool.push_task(work, 70);
    pool.push_task(work, 80);
    pool.push_task(work, 40);
    pool.push_task(work, 50);
    pool.waitForTasks();

    std::string res(g_buffer.data(), g_counter.load());
    std::cout << res << std::endl;
    std::cout << "Esto es despues" << std::endl;
}

TEST(TUBULThread, testPool2) {

    TU::ThreadPool pool(4);
    auto work = [](int f) {
        std::stringstream out;
        out << "Fibo("<< f << ") : " << fib(f) << std::endl;
        auto text = out.str();
        size_t textSize = text.size();
        auto prev = g_counter.fetch_add(textSize, std::memory_order_seq_cst);
        std::copy( text.begin(), text.end(), g_buffer.data()+prev);
    };
    pool.push_task(work, 90);
    pool.push_task(work, 70);
    pool.push_task(work, 80);
    pool.push_task(work, 40);
    pool.push_task(work, 50);
    pool.push_task(work, 90);
    pool.push_task(work, 90);
    pool.push_task(work, 70);
    pool.push_task(work, 80);
    pool.push_task(work, 40);
    pool.push_task(work, 50);
    pool.push_task(work, 70);
    pool.push_task(work, 80);
    pool.push_task(work, 40);
    pool.push_task(work, 50);
    pool.push_task(work, 90);
    pool.push_task(work, 70);
    pool.push_task(work, 80);
    pool.push_task(work, 40);
    pool.push_task(work, 50);
    pool.waitForTasks();
    std::string res(g_buffer.data(), g_counter.load());
    std::cout << res << std::endl;
    std::cout << "Esto es despues" << std::endl;
}
