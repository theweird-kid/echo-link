#include "../include/thread_safe_queue.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>

void producer(ThreadSafeQueue<int>& queue, int start, int count) {
    for (int i = 0; i < count; ++i) {
        queue.push(start + i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void consumer(ThreadSafeQueue<int>& queue, int expected_count, std::vector<int>& out) {
    int value;
    for (int i = 0; i < expected_count; ++i) {
        bool popped = queue.pop_for(value, std::chrono::milliseconds(100));
        if (popped) {
            out.push_back(value);
        } else {
            std::cout << "Timeout waiting for value\n";
        }
    }
}

int main() {
    ThreadSafeQueue<int> queue;

    const int items_per_producer = 5;
    const int producer_count = 2;
    const int total_items = items_per_producer * producer_count;

    std::vector<int> consumed;
    std::thread prod1(producer, std::ref(queue), 0, items_per_producer);
    std::thread prod2(producer, std::ref(queue), 100, items_per_producer);
    std::thread cons(consumer, std::ref(queue), total_items, std::ref(consumed));

    prod1.join();
    prod2.join();
    cons.join();

    std::cout << "Consumed values: ";
    for (int v : consumed) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    assert(consumed.size() == static_cast<size_t>(total_items));
    std::cout << "Test passed: All items consumed." << std::endl;

    // Test shutdown
    queue.Shutdown();
    int dummy;
    bool got = queue.pop_for(dummy, std::chrono::milliseconds(10));
    assert(!got);
    std::cout << "Test passed: Shutdown disables pop_for." << std::endl;

    return 0;
}
