#include "../Include/OrderGenerator.h"

OrderGenerator::OrderGenerator()
	: orderQueue_(16248)
	, done_(false)
{ }

void OrderGenerator::GenerateOrders(const BenchmarkParams& p)
{
	// Parse benchmark parameters
	
	std::default_random_engine generator;
	std::uniform_int_distribution<int> orderIdDist(p.orderIdDist_.first, p.orderIdDist_.second);
	std::discrete_distribution<int> eventTypeDist (p.eventTypeDist_.begin(), p.eventTypeDist_.end());
	std::discrete_distribution<int> orderTypeDist (p.orderTypeDist_.begin(), p.orderTypeDist_.end());
	std::bernoulli_distribution sideDist(p.sideDist_);
	std::normal_distribution<double> priceDist(p.priceDist_.first, p.priceDist_.second);
	std::lognormal_distribution<double> quantityDist(p.quantityDist_.first, p.quantityDist_.second);
	

	// Push orders to the queue
	for (int i = 0; i < p.numEvents_; ++i)
	{
		OrderId orderId = orderIdDist(generator);
		EventType eventType = static_cast<EventType>(eventTypeDist(generator));
		OrderType orderType = static_cast<OrderType>(orderTypeDist(generator));
		Side side = sideDist(generator) ? Side::Buy : Side::Sell;
		Price price = static_cast<Price>(priceDist(generator));
		Quantity quantity = static_cast<Quantity>(quantityDist(generator));

		EventInformation event;
		switch (eventType)
		{
			case EventType::AddOrder:
				event = { EventType::AddOrder, orderId, orderType, side, price, quantity };
				break;
			case EventType::ModifyOrder:
				event = { EventType::ModifyOrder, orderId, {}, side, price, quantity };
				break;
			case EventType::CancelOrder:
				event = { EventType::CancelOrder, orderId };
				break;
			default:
				throw std::logic_error("Unsupported Event");
		}

		while (!orderQueue_.push(std::move(event))) {}
	}

	done_ = true;
}

void OrderGenerator::ProcessOrders(OrderBook& book)
{
	while (!done_ || !orderQueue_.empty())
	{
		EventInformation event;

		if (orderQueue_.pop(event))
		{
			switch (event.eventType_)
			{
				case EventType::AddOrder:
					book.AddOrderToQueue(
						event.orderId_,
						event.orderType_,
						event.side_,
						event.price_,
						event.quantity_
					);
					break;
				case EventType::ModifyOrder:
					book.ModifyOrderToQueue(
						event.orderId_,
						event.side_,
						event.price_,
						event.quantity_
					);
					break;
				case EventType::CancelOrder:
					book.CancelOrderToQueue(event.orderId_);
					break;
				default:
					throw std::logic_error("Unsupported Event.");
			}
		}
	}
}