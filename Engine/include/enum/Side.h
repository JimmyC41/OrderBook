#pragma once

enum class Side
{
	Buy,
	Sell,
};

inline std::string_view SideToString(Side side)
{
	switch (side)
	{
	case Side::Buy: return "Buy";
	case Side::Sell: return "Sell";
	default: return "N/A";
	}
}