#pragma once

#include <string>
#include <string_view>

enum class OrderType
{
	GoodTillCancel,
	Market,
	FillAndKill,
	FillOrKill,
};

inline std::string_view OrderTypeToString(OrderType type)
{
	switch (type)
	{
	case OrderType::FillAndKill: return "Fill and Kill";
	case OrderType::FillOrKill: return "Fill or Kill";
	case OrderType::GoodTillCancel: return "Good Till Cancel";
	case OrderType::Market: return "Market";
	default: return "N/A";
	}
}