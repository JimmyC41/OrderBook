#pragma once

#include <variant>

#include "../Orderbook/Using.h"
#include "../Enum/Side.h"
#include "../Enum/OrderType.h"
#include "../Enum/OrderEvent.h"

struct AddOrderPayload
{
	OrderId orderId_;
	OrderType orderType_;
	Side side_;
	Price price_;
	Quantity quantity_;
};

struct ModifyOrderPayload
{
	OrderId orderId_;
	Side side_;
	Price price_;
	Quantity quantity_;
};

struct CancelOrderPayload
{
	OrderId orderId_;
};

using Payload = std::variant<AddOrderPayload, ModifyOrderPayload, CancelOrderPayload>;