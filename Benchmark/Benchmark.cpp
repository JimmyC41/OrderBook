#include <chrono>
#include <random>
#include <boost/lockfree/queue.hpp>

#include "Include/OrderBook/OrderBook.h"
#include "Include/Util/EventInformation.h"
#include "Include/OrderGenerator.h"

void BenchmarkOrderBook(const BenchmarkParams& params)
{
    OrderGenerator generator;
    OrderBook orderbook;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread generateThread(&OrderGenerator::GenerateOrders, &generator, std::ref(params));
    std::thread processThread(&OrderGenerator::ProcessOrders, &generator, std::ref(orderbook));

    if (generateThread.joinable()) generateThread.join();
    if (processThread.joinable()) processThread.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    const auto& bookInfos = orderbook.GetOrderInfos();
    std::cout << std::format
    (
        "[!] Benchmark Result: Processed {} random orders in {} ms.", 
        params.numEvents_, 
        duration
    ) << std::endl;

    // orderbook.Display();
}

int main()
{
    int start = std::pow(10, 3);
    int end = std::pow(10, 7);

    for (int num = start; num <= end; num *= 10)
    {
        auto params = DefaultParams(num);
        BenchmarkOrderBook(params);
    }

    return 0;
}