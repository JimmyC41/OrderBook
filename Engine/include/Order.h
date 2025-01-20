#pragma once

#include <list>
#include <format>

#include "Enum/OrderType.h"
#include "Enum/Side.h"
#include "Using.h"

class Order
{
public:
	Order(OrderId orderId, OrderType orderType, Side side, Price price, Quantity quantity)
		: orderId_{ orderId }
		, orderType_{ orderType }
		, side_{ side }
		, price_{ price }
		, initialQuantity_{ quantity }
		, remainingQuantity_{ quantity }
	{
	}

	OrderId GetOrderId() const { return orderId_; }
	OrderType GetOrderType() const { return orderType_; }
	Side GetSide() const { return side_; }
	Price GetPrice() const { return price_; }
	Quantity GetInitialQuantity() const { return initialQuantity_; }
	Quantity GetRemainingQuantity() const { return remainingQuantity_; }

	bool IsFilled() const { return GetRemainingQuantity() == 0; }
	void Fill(Quantity quantity) { remainingQuantity_ -= quantity; }
	void SetMarketPrice(Price price) { price_ = price; }

	std::string ToString() const {
		return std::format("ID: {}, Type: {}, Price: {}, Quantity: {}",
			GetOrderId(),
			OrderTypeToString(GetOrderType()),
			GetPrice(),
			GetRemainingQuantity());
	}

private:
	OrderId orderId_;
	OrderType orderType_;
	Side side_;
	Price price_;
	Quantity initialQuantity_;
	Quantity remainingQuantity_;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;
