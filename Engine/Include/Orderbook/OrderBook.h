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

class OrderBook
{
public:

	OrderBook()
		: queueManager_([this](const QueueEvent& event) { HandleEvent(event); })
	{ }

	// APIs to queue order requests

	void AddOrderToQueue(OrderId id, OrderType type, Side side, Price price, Quantity quantity);
	void ModifyOrderToQueue(OrderId id, Side side, Price price, Quantity quantity);
	void CancelOrderToQueue(OrderId id);

	// API to process the event in the OrderBook

	void HandleEvent(const QueueEvent& event);

	// Other public APIs

	void Display() const;
	OrderBookLevelInfos GetOrderInfos() const;
	std::size_t Size() const;

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
	QueueManager queueManager_;

	// Non-locking methods

	void HandleAddOrderInternal(const AddOrderPayload& payload);
	void HandleModifyOrderInternal(const ModifyOrderPayload& payload);
	void HandleCancelOrderInternal(const CancelOrderPayload& payload);

	Trades AddOrderInternal(OrderPointer order);
	Trades ModifyOrderInternal(OrderModify order);
	void CancelOrderInternal(OrderId orderId);
	
	Trades MatchOrdersInternal();
	bool CanMatchInternal(Side side, Price price) const;
	bool CanBeFullyFilledInternal(Side side, Price price, Quantity quantity) const;

	void UpdateLevelsInternal(Price price, Quantity quantity, OrderEvent event);
	void OrderAddEventInternal(OrderPointer order);
	void OrderCancelEventInternal(OrderPointer order);
	void OrderMatchEventInternal(Price price, Quantity quantity, bool isFullyFilled);
};