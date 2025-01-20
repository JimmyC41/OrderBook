#include <chrono>
#include <random>

#include "include/util/InputHandler.h"
#include "include/OrderBook.h"

// Function to generate random events
std::vector<Information> generateRandomEvents(int numEvents, int maxOrders) {
    std::vector<Information> events;
    events.reserve(numEvents);

    static std::default_random_engine generator;
    static std::uniform_int_distribution<int> orderIdDist(1, maxOrders);
    static std::uniform_int_distribution<int> eventTypeDist(0, 2); // 0: AddOrder, 1: ModifyOrder, 2: CancelOrder
    static std::uniform_real_distribution<double> priceDist(1.0, 1000.0); // Prices between 1 and 1000
    static std::uniform_int_distribution<int> quantityDist(1, 100); // Quantities between 1 and 100
    static std::bernoulli_distribution sideDist(0.5); // 50% Buy, 50% Sell

    for (int i = 0; i < numEvents; ++i) {
        EventType eventType = static_cast<EventType>(eventTypeDist(generator));
        OrderId orderId = orderIdDist(generator);
        Side side = sideDist(generator) ? Side::Buy : Side::Sell;
        Price price = static_cast<Price>(priceDist(generator));
        Quantity quantity = static_cast<Quantity>(quantityDist(generator));

        if (eventType == EventType::AddOrder) {
            events.push_back({ EventType::AddOrder, orderId, OrderType::GoodTillCancel, side, price, quantity });
        }
        else if (eventType == EventType::ModifyOrder) {
            events.push_back({ EventType::ModifyOrder, orderId, {}, side, price, quantity });
        }
        else { // CancelOrder
            events.push_back({ EventType::CancelOrder, orderId });
        }
    }

    return events;
}


void benchmarkOrderBook(int numEvents, int maxOrders) {
    auto events = generateRandomEvents(numEvents, maxOrders);

    auto GetOrder = [](const Information& event) {
        return std::make_shared<Order>(
            event.orderId_,
            event.orderType_,
            event.side_,
            event.price_,
            event.quantity_
        );
        };

    auto GetOrderModify = [](const Information& event) {
        return OrderModify{
            event.orderId_,
            event.side_,
            event.price_,
            event.quantity_
        };
        };

    OrderBook orderbook;

    // Measure the time taken to process all events
    auto start = std::chrono::high_resolution_clock::now();

    for (const auto& event : events) {
        switch (event.eventType_) {
        case EventType::AddOrder:
            orderbook.AddOrder(GetOrder(event));
            break;
        case EventType::ModifyOrder:
            orderbook.ModifyOrder(GetOrderModify(event));
            break;
        case EventType::CancelOrder:
            orderbook.CancelOrder(event.orderId_);
            break;
        default:
            throw std::logic_error("Unsupported event.");
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    // Calculate duration in milliseconds
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Output benchmark results
    const auto& orderbookInfos = orderbook.GetOrderInfos();
    std::cout << "Processed " << events.size() << " random events in " << duration << " ms.\n";
    std::cout << "Final OrderBook State:\n";
    std::cout << "- Total orders: " << orderbook.Size() << "\n";
    std::cout << "- Bid orders: " << orderbookInfos.GetBids().size() << "\n";
    std::cout << "- Ask orders: " << orderbookInfos.GetAsks().size() << "\n\n";
}

int main() {

    std::cout << "Running random OrderBook Benchmarks...\n";

    benchmarkOrderBook(1000, 500);
    benchmarkOrderBook(5000, 2000);
    benchmarkOrderBook(10000, 5000);
   
    return 0;
}