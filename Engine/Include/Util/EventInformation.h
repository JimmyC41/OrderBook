#pragma once

#include "../OrderBook/OrderBook.h"
#include "../OrderBook/Order.h"
#include "../OrderBook/Using.h"
#include "../Queue/EventType.h"

struct EventInformation
{
    EventType eventType_;
    OrderId orderId_;
    OrderType orderType_;
    Side side_;
    Price price_;
    Quantity quantity_;
};

using EventInformations = std::vector<EventInformation>;