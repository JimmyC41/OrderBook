#include "pch.h"
#include "../OrderBookCpp/Src/OrderBook.cpp"

namespace googletest = ::testing;

enum class EventType
{
	AddOrder,
	ModifyOrder,
	CancelOrder,
};

struct Information
{
	EventType eventType_;
	OrderId orderId_;
	OrderType orderType_;
	Side side_;
	Price price_;
	Quantity quantity_;
};

using Informations = std::vector<Information>;

struct Result
{
	std::size_t allCount_;
	std::size_t bidCount_;
	std::size_t askCount_;
};

using Results = std::vector<Result>;

struct InputHandler
{
private:
	std::uint32_t ToNumber(const std::string_view& str) const
	{
		std::int64_t value{};
		std::from_chars(str.data(), str.data() + str.size(), value);
		if (value < 0)
			throw std::logic_error("Value is below zero.");
		return static_cast<std::uint32_t>(value);
	}

	bool TryParseResult(const std::string_view& str, Result& result) const
	{
		if (str.at(0) != 'R')
			return false;

		auto values = Split(str, ' ');
		result.allCount_ = ToNumber(values[1]);
		result.bidCount_ = ToNumber(values[2]);
		result.askCount_ = ToNumber(values[3]);

		return true;
	}

	bool TryParseOrderInfo(const std::string_view& str, Information& event) const
	{
		auto value = str.at(0);
		auto values = Split(str, ' ');
		if (value == 'A')
		{
			event.eventType_ = EventType::AddOrder;
			event.orderId_ = ParseOrderId(values[1]);
			event.orderType_ = ParseOrderType(values[2]);
			event.side_ = ParseSide(values[3]);
			event.price_ = ParsePrice(values[4]);
			event.quantity_ = ParseQuantity(values[5]);
		}
		else if (value == 'M')
		{
			event.eventType_ = EventType::ModifyOrder;
			event.orderId_ = ParseOrderId(values[1]);
			event.side_ = ParseSide(values[2]);
			event.price_ = ParsePrice(values[3]);
			event.quantity_ = ParseQuantity(values[4]);
		}
		else if (value == 'C')
		{
			event.eventType_ = EventType::CancelOrder;
			event.orderId_ = ParseOrderId(values[1]);
		}
		else
			return false;

		return true;
	}

	std::vector<std::string_view> Split(const std::string_view& str, char delimiter) const
	{
		std::vector<std::string_view> columns;
		columns.reserve(5);

		std::size_t startIndex{}, endIndex{};
		while ((endIndex = str.find(delimiter, startIndex)) != std::string::npos)
		{
			auto distance = endIndex - startIndex;
			auto column = str.substr(startIndex, distance);
			startIndex = endIndex + 1;
			columns.push_back(column);
		}
		columns.push_back(str.substr(startIndex));
		return columns;
	}

	OrderId ParseOrderId(const std::string_view& str) const
	{
		if (str.empty())
			throw std::logic_error("Empty OrderId.");
		return static_cast<OrderId>(ToNumber(str));
	}

	OrderType ParseOrderType(const std::string_view& str) const
	{
		if (str == "FillAndKill")
			return OrderType::FillAndKill;
		else if (str == "GoodTillCancel")
			return OrderType::GoodTillCancel;
		else if (str == "FillOrKill")
			return OrderType::FillOrKill;
		else if (str == "Market")
			return OrderType::Market;
		else
			throw std::logic_error("OrderType N/A.");
	}

	Side ParseSide(const std::string_view& str) const
	{
		if (str == "B")
			return Side::Buy;
		else if (str == "S")
			return Side::Sell;
		else
			throw std::logic_error("Side N/A.");
	}

	Price ParsePrice(const std::string_view& str) const
	{
		if (str.empty())
			throw std::logic_error("Price N/A.");
		return ToNumber(str);
	}

	Quantity ParseQuantity(const std::string_view& str) const
	{
		if (str.empty())
			throw std::logic_error("Quantity N/A.");
		return ToNumber(str);
	}

public:
	std::tuple<Informations, Result> GetOrderInformations(const std::filesystem::path& path) const
	{
		Informations events;
		events.reserve(1'000);

		std::string line;
		std::ifstream file{ path };
		while (std::getline(file, line))
		{
			if (line.empty()) break;

			const bool isResult = line.at(0) == 'R';
			const bool isEvent = !isResult;

			if (isEvent)
			{
				Information event;

				auto isValid = TryParseOrderInfo(line, event);
				if (!isValid) continue;

				events.push_back(event);
			}
			else
			{
				if (!file.eof())
					throw std::logic_error("Result should only be specified at the end.");

				Result result;

				auto isValid = TryParseResult(line, result);
				if (!isValid) continue;

				return { events, result };
			}

		}

		throw std::logic_error("No result specified.");
	}
};

class OrderBookTestsFixture : public googletest::TestWithParam<const char*>
{
private:
	const static inline std::filesystem::path Root{ std::filesystem::current_path() };
	const static inline std::filesystem::path TestFolder{ "TestFiles" };
public:
	const static inline std::filesystem::path TestFolderPath{ Root / TestFolder };
};


TEST_P(OrderBookTestsFixture, OrderbookTestSuite)
{
	const auto file = OrderBookTestsFixture::TestFolderPath / GetParam();

	InputHandler handler;
	const auto [events, result] = handler.GetOrderInformations(file);

	auto GetOrder = [](const Information& event)
	{
		return std::make_shared<Order>
		(
			event.orderId_,
			event.orderType_,
			event.side_,
			event.price_,
			event.quantity_
		);
	};

	auto GetOrderModify = [](const Information& event)
	{
			return OrderModify
			{
				event.orderId_,
				event.side_,
				event.price_,
				event.quantity_,
			};
	};

	OrderBook orderbook;
	for (const auto& event : events)
	{
		switch (event.eventType_)
		{
			case EventType::AddOrder:
				orderbook.AddOrder(GetOrder(event));
				break;
			case EventType::ModifyOrder:
				orderbook.ModifyOrder(GetOrderModify(event));
				break;
			case EventType::CancelOrder:
				orderbook.CancelOrder(event.orderId_);
				break;
			default:
				throw std::logic_error("Unsupported event.");
		}
	}

	const auto& orderbookInfos = orderbook.GetOrderInfos();
	ASSERT_EQ(orderbook.Size(), result.allCount_);
	ASSERT_EQ(orderbookInfos.GetBids().size(), result.bidCount_);
	ASSERT_EQ(orderbookInfos.GetAsks().size(), result.askCount_);
}

INSTANTIATE_TEST_CASE_P(Tests, OrderBookTestsFixture, googletest::ValuesIn({
	"Match_GoodTillCancel.txt",
	"Match_FillAndKill.txt",
	"Match_FillOrKill_Hit.txt",
	"Match_FillOrKill_Miss.txt",
	"Cancel_Success.txt",
	"Modify_Side.txt",
	"Match_Market.txt"
}));