#include <chrono>
#include <random>
#include <boost/lockfree/queue.hpp>

#include "Include/OrderBook/OrderBook.h"
#include "Include/Util/EventInformation.h"

void producer(boost::lockfree::queue<EventInformation>& queue, int numEvents, std::atomic<bool>& done)
{
    static std::default_random_engine generator;
    static std::uniform_int_distribution<int> orderIdDist(1, static_cast<int>(numEvents * 0.9));
    static std::discrete_distribution<int> eventTypeDist({ 90, 5, 5 });
    static std::discrete_distribution<int> orderTypeDist({60, 10, 25 , 5});
    static std::bernoulli_distribution sideDist(0.5);
    static std::normal_distribution<double> priceDist(1000.0, 100.0);
    static std::lognormal_distribution<double> quantityDist(6.0, 1.0);

    for (int i = 0; i < numEvents; ++i)
    {
        EventType eventType = static_cast<EventType>(eventTypeDist(generator));
        OrderId orderId = orderIdDist(generator);
        OrderType orderType = static_cast<OrderType>(orderTypeDist(generator));
        Side side = sideDist(generator) ? Side::Buy : Side::Sell;
        Price price = static_cast<Price>(priceDist(generator));
        Quantity quantity = static_cast<Quantity>(quantityDist(generator));

        EventInformation event;
        switch (eventType)
        {
            case EventType::AddOrder:
                event = { EventType::AddOrder, orderId, orderType, side, price, quantity };
                break;
            case EventType::ModifyOrder:
                event = { EventType::ModifyOrder, orderId, {}, side, price, quantity };
                break;
            case EventType::CancelOrder:
                event = { EventType::CancelOrder, orderId };
                break;
            default:
                throw std::logic_error("Unsupported Event");
        }

        while (!queue.push(event)) { }
    }
    done = true;
}

void consumer(boost::lockfree::queue<EventInformation>& queue, OrderBook& book, std::atomic<bool>& done)
{
    while (!done || !queue.empty())
    {
        EventInformation event;
        if (queue.pop(event))
        {
            switch (event.eventType_)
            {
                case EventType::AddOrder:
                    book.AddOrderToQueue(
                        event.orderId_,
                        event.orderType_,
                        event.side_,
                        event.price_,
                        event.quantity_
                    );
                    break;
                case EventType::ModifyOrder:
                    book.ModifyOrderToQueue(
                        event.orderId_,
                        event.side_,
                        event.price_,
                        event.quantity_
                    );
                    break;
                case EventType::CancelOrder:
                    book.CancelOrderToQueue(event.orderId_);
                    break;
                default:
                    throw std::logic_error("Unsupported Event.");
            }
        }
    }
}

void BenchmarkOrderBook(int numEvents)
{
    OrderBook book;
    boost::lockfree::queue<EventInformation> queue(128);
    std::atomic<bool> done{ false };
    
    auto start = std::chrono::high_resolution_clock::now();

    std::thread producerThread(producer, std::ref(queue), numEvents, std::ref(done));
    std::thread consumerThread(consumer, std::ref(queue), std::ref(book), std::ref(done));

    producerThread.join();
    consumerThread.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Benchmark

    const auto& bookInfos = book.GetOrderInfos();
    std::cout << std::format("Processed {} random events in {} ms.\n", numEvents, duration) << std::endl;

    book.Display();
}

int main() {
    std::cout << "Running random OrderBook Benchmarks...\n";
    BenchmarkOrderBook(1000);
    BenchmarkOrderBook(100000);
    BenchmarkOrderBook(1000000);
    BenchmarkOrderBook(10000000);
    return 0;
}