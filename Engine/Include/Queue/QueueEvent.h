#pragma once

#include <variant>

#include "Payload.h"
#include "EventType.h"

// Struct to encapsulate order request information to the Orderbook

struct QueueEvent
{
	EventType event_;
	Payload payload_;
};