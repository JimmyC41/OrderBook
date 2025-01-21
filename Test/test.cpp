#include "pch.h"
#include "Include/Orderbook/OrderBook.h"
#include "Include/Util/InputHandler.h"

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
	const auto [events, result] = handler.GetEventInformationsFromFile(file);

	// Sequentially process order requests from the file

	OrderBook orderbook;

	for (const auto& info : events)
	{
		switch (info.eventType_)
		{
			case EventType::AddOrder:
				orderbook.AddOrderToQueue
				(
					info.orderId_,
					info.orderType_,
					info.side_,
					info.price_,
					info.quantity_
				);
				break;
			case EventType::ModifyOrder:
				orderbook.ModifyOrderToQueue
				(
					info.orderId_,
					info.side_,
					info.price_,
					info.quantity_
				);
				break;
			case EventType::CancelOrder:
				orderbook.CancelOrderToQueue
				(
					info.orderId_
				);
				break;
			default:
				throw std::logic_error("Unsupported event.");
		}
	}

	// Assert

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