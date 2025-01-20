#include "pch.h"
#include "include/OrderBook.h"
#include "include/util/InputHandler.h"

namespace googletest = ::testing;

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