#pragma once

#include <variant>

#include "Payload.h"
#include "EventType.h"

struct QueueEvent
{
	EventType event_;
	Payload payload_;
};