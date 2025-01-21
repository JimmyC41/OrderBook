#pragma once

#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <format>
#include <string>
#include <string_view>
#include <optional>
#include <numeric>
#include <variant>

#include "Using.h"
#include "Order.h"
#include "OrderModify.h"
#include "Trade.h"
#include "OrderbookLevelInfos.h"
#include "../Enum/OrderEvent.h"
#include "../Queue/QueueManager.h"
#include "../Log/FileLogger.h"

class OrderBook
{
public:

	OrderBook()
		: queueManager_([this](const QueueEvent& event) { HandleEvent(event); })
	{ 
		FileLogger::Init("Debug/OrderBook.Log");
		FileLogger::Get()->info("Orderbook initialized.");
	}

	~OrderBook()
	{
		FileLogger::Get()->info("Orderbook destroyed.");
		FileLogger::Cleanup();
	}

	OrderBook(const OrderBook&) = delete;
	OrderBook(OrderBook&&) = delete;
	OrderBook& operator=(const OrderBook&) = delete;
	OrderBook& operator=(OrderBook&&) = delete;

	// APIs to queue order requests
	void AddOrderToQueue(OrderId id, OrderType type, Side side, Price price, Quantity quantity);
	void ModifyOrderToQueue(OrderId id, Side side, Price price, Quantity quantity);
	void CancelOrderToQueue(OrderId id);

	// Thread-safe API to parse events, extract order information payload and call
	// private APIs to process in the orderbook - invoked by the QueueManager's worked thread
	void HandleEvent(const QueueEvent& event);

	// Other public APIS - blocks until all order requests have been processed
	void Display() const;
	OrderBookLevelInfos GetOrderInfos() const;
	std::size_t Size() const;

private:

	// Contains a given orderpointer and its location in the bids or asks map
	struct OrderEntry
	{
		OrderPointer order_{ nullptr };
		OrderPointers::iterator location_;
	};

	// Contains aggregate quantity and order count for a given price in the orderbook
	struct LevelDepth
	{
		Quantity quantity_{ };
		Quantity count_{ };
	};

	// Global mutex to protect orderbook during add, modify and cancel order events
	mutable std::mutex ordersMutex_;

	// Map of prices to orders for bids and asks
	std::map<Price, OrderPointers, std::greater<Price>> bids_;
	std::map<Price, OrderPointers, std::less<Price>> asks_;

	// Map of ids to orders and iterator for quick lookup / deletion
	std::unordered_map<OrderId, OrderEntry> orders_;

	// Map of prices to level information
	std::unordered_map<Price, LevelDepth> levels_;

	// Manages order requests and processes them synchronously in a thread-safe manner
	QueueManager queueManager_;

	// Handles new order requests in the orderbook
	Trades AddOrderInternal(const AddOrderPayload& payload);
	Trades ModifyOrderInternal(const ModifyOrderPayload& payload);
	void CancelOrderInternal(const CancelOrderPayload& payload);
	
	// Matches new or modified orders
	Trades MatchOrdersInternal();

	bool CanMatchInternal(Side side, Price price) const;
	bool CanBeFullyFilledInternal(Side side, Price price, Quantity quantity) const;

	void UpdateLevelsInternal(Price price, Quantity quantity, OrderEvent event);
	void UpdateLevelOnAddOrder(OrderPointer order);
	void UpdateLevelOnCancelOrder(OrderPointer order);
	void UpdateLevelOnMatchOrders(Price price, Quantity quantity, bool isFullyFilled);
};