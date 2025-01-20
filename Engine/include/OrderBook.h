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

#include "Using.h"
#include "Order.h"
#include "OrderModify.h"
#include "Trade.h"
#include "Enum/OrderEvent.h"
#include "OrderbookLevelInfos.h"

class OrderBook
{
public:

	OrderBook() = default;

	// Self-locking methods

	Trades AddOrder(OrderPointer order);
	Trades ModifyOrder(OrderModify order);
	void CancelOrder(OrderId orderId);
	void CancelOrders(OrderIds orderIds);

	void Display() const;
	std::size_t Size() const;
	OrderBookLevelInfos GetOrderInfos() const;

private:

	struct OrderEntry
	{
		OrderPointer order_{ nullptr };
		OrderPointers::iterator location_;
	};

	struct LevelDepth
	{
		Quantity quantity_{ };
		Quantity count_{ };
	};

	mutable std::mutex ordersMutex_;

	std::map<Price, OrderPointers, std::greater<Price>> bids_;
	std::map<Price, OrderPointers, std::less<Price>> asks_;
	std::unordered_map<OrderId, OrderEntry> orders_;
	std::unordered_map<Price, LevelDepth> levels_;

	// Lock-free methods for internal use

	bool CanMatchInternal(Side side, Price price) const;
	bool CanBeFullyFilledInternal(Side side, Price price, Quantity quantity) const;
	Trades MatchOrdersInternal();
	void CancelOrderInternal(OrderId orderId);
	void CancelOrdersInternal(OrderIds orderIds);
	void UpdateLevelsInternal(Price price, Quantity quantity, OrderEvent event);
	void OrderAddEventInternal(OrderPointer order);
	void OrderCancelEventInternal(OrderPointer order);
	void OrderMatchEventInternal(Price price, Quantity quantity, bool isFullyFilled);
};