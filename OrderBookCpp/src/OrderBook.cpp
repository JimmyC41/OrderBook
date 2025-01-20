#include "../include/OrderBook.h"

Trades OrderBook::AddOrder(OrderPointer order)
{
	std::scoped_lock ordersLock{ ordersMutex_ };

	if (orders_.contains(order->GetOrderId()))
		return { };

	// Check if FAK can be matched

	if (order->GetOrderType() == OrderType::FillAndKill &&
		!CanMatchInternal(order->GetSide(), order->GetPrice()))
		return { };

	// Check if FOK can be fully filled

	if (order->GetOrderType() == OrderType::FillOrKill &&
		!CanBeFullyFilledInternal(order->GetSide(), order->GetPrice(), order->GetRemainingQuantity()))
		return { };

	// Set the price if the order is a market order

	auto setMarketPrice = [&](auto& order, auto& ordersOtherSide)
		{
			const auto& [worstPrice, _] = *ordersOtherSide.rbegin();
			order->SetMarketPrice(worstPrice);
		};

	if (order->GetOrderType() == OrderType::Market)
	{
		if (order->GetSide() == Side::Buy && !asks_.empty())
			setMarketPrice(order, asks_);
		else if (order->GetSide() == Side::Sell && !bids_.empty())
			setMarketPrice(order, bids_);
		else
			return { };
	}

	// Insert the order at the given side and price

	auto& level = (order->GetSide() == Side::Buy)
		? bids_[order->GetPrice()]
		: asks_[order->GetPrice()];

	level.push_back(order);

	// Add order to the aggregate orders map

	auto iterator = std::prev(level.end());
	orders_.insert({ order->GetOrderId(), OrderEntry { order, iterator } });

	// Update the level info struct

	OrderAddEventInternal(order);

	// Run matching algorithm and return new trades (if any)

	return MatchOrdersInternal();
}

Trades OrderBook::ModifyOrder(OrderModify order)
{
	OrderType orderType;

	// Fetch the existing order

	{
		std::scoped_lock ordersLock{ ordersMutex_ };

		if (!orders_.contains(order.GetOrderId()))
			return { };

		const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
		orderType = existingOrder->GetOrderType();
	}

	// Cancel the existing order and add the modified order, returning any trades

	CancelOrder(order.GetOrderId());
	return AddOrder(order.ToOrderPointer(orderType));
}

void OrderBook::CancelOrder(OrderId orderId)
{
	std::scoped_lock ordersLock{ ordersMutex_ };
	CancelOrderInternal(orderId);
}

void OrderBook::CancelOrders(OrderIds orderIds)
{
	std::scoped_lock ordersLock{ ordersMutex_ };
	CancelOrdersInternal(orderIds);
}

void OrderBook::Display() const
{
	if (orders_.empty())
	{
		std::cout << "Orderbook is empty!\n" << std::endl;
		return;
	}

	std::scoped_lock ordersLock{ ordersMutex_ };

	std::cout << "------ Displaying the Orderbook ------\n";

	std::cout << "Displaying Bids:\n";
	if (!bids_.empty())
	{
		for (const auto& [price, bids] : bids_)
			for (const auto& bid : bids)
				std::cout << "\t" << bid->ToString() << std::endl;
	}

	std::cout << "Displaying Asks:\n";
	if (!asks_.empty())
	{
		for (const auto& [price, asks] : asks_)
			for (const auto& ask : asks)
				std::cout << "\t" << ask->ToString() << std::endl;
	}

	std::cout << std::format("Orderbook contains {} orders\n", orders_.size());
}

std::size_t OrderBook::Size() const
{
	std::scoped_lock ordersLock{ ordersMutex_ };
	return orders_.size();
}

OrderBookLevelInfos OrderBook::GetOrderInfos() const
{
	LevelInfos bidInfos, askInfos;
	bidInfos.reserve(orders_.size());
	askInfos.reserve(orders_.size());

	auto CreateLevelInfos = [](Price price, const OrderPointers& orders)
		{
			return LevelInfo{ price, std::accumulate(orders.begin(), orders.end(), (Quantity)0,
				[](Quantity runningSum, const OrderPointer& order)
				{ return runningSum + order->GetRemainingQuantity(); }) };
		};

	for (const auto& [price, orders] : bids_)
		bidInfos.push_back(CreateLevelInfos(price, orders));

	for (const auto& [price, orders] : asks_)
		askInfos.push_back(CreateLevelInfos(price, orders));

	return OrderBookLevelInfos{ bidInfos, askInfos };
}

bool OrderBook::CanMatchInternal(Side side, Price price) const
{
	if (side == Side::Buy)
		return !asks_.empty() && price >= asks_.begin()->first;
	else
		return !bids_.empty() && price <= bids_.begin()->first;
}

// Check if the quantity requested can be fulfilled by the aggregate
// quantity available at crossed price levels on the opposite side

bool OrderBook::CanBeFullyFilledInternal(Side side, Price price, Quantity quantity) const
{
	if ((side == Side::Buy && asks_.empty()) ||
		(side == Side::Sell && bids_.empty()) ||
		!CanMatchInternal(side, price))
		return false;

	// Find the threshold price for executing an order
	// E.g. If side is buy, threshold price is the lowest ask

	Price bestPrice = (side == Side::Buy && !asks_.empty())
		? asks_.begin()->first
		: bids_.begin()->first;

	// A level is valid if we can cross the orderbook

	for (const auto& [levelPrice, levelDepth] : levels_)
	{
		// Skip levels which do not cross the orderbook

		if ((side == Side::Buy && levelPrice < bestPrice) ||
			(side == Side::Sell && levelPrice > bestPrice))
			continue;

		// Skip levels outside our range 
		// I.e. above a buy price or below a sell price

		if ((side == Side::Buy && levelPrice > price) ||
			(side == Side::Sell && levelPrice < price))
			continue;

		// Check if level can fill completely, otherwise reduce remaining quantity

		if (quantity <= levelDepth.quantity_)
			return true;

		quantity -= levelDepth.quantity_;
	}

	return false;
}

Trades OrderBook::MatchOrdersInternal()
{
	Trades trades;
	trades.reserve(std::min(bids_.size(), asks_.size()));

	// Lambda to remove (filled) orders from the level and aggregate of orders

	auto removeOrder = [&](auto& order, auto& level)
		{
			level.pop_front();
			orders_.erase(order->GetOrderId());
		};

	// Lambda to cancel FAK orders for a given side of the OB

	auto cancelFAK = [&](auto& side)
		{
			auto& [_, orders] = *side.begin();
			auto& best = orders.front();
			if (best->GetOrderType() == OrderType::FillAndKill)
				CancelOrderInternal(best->GetOrderId());
		};

	while (!bids_.empty() && !asks_.empty())
	{
		// Get the bid and ask levels by price priority

		auto& [bidPrice, bidLevel] = *bids_.begin();
		auto& [askPrice, askLevel] = *asks_.begin();

		if (bidPrice < askPrice) break;

		// Match orders by time priority

		while (!bidLevel.empty() && !askLevel.empty())
		{
			auto bid = bidLevel.front();
			auto ask = askLevel.front();

			Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());
			bid->Fill(quantity);
			ask->Fill(quantity);

			if (bid->IsFilled()) removeOrder(bid, bidLevel);
			if (ask->IsFilled()) removeOrder(ask, askLevel);

			// Record the trade

			trades.emplace_back(Trade{
				TradeInfo{ bid->GetOrderId(), bid->GetPrice(), quantity },
				TradeInfo{ ask->GetOrderId(), ask->GetPrice(), quantity }
				});

			// Update the level infos struct

			OrderMatchEventInternal(bid->GetPrice(), quantity, bid->IsFilled());
			OrderMatchEventInternal(ask->GetPrice(), quantity, ask->IsFilled());
		}

		// Clear level if all orders have been filled

		if (bidLevel.empty()) bids_.erase(bidPrice);
		if (askLevel.empty()) asks_.erase(askPrice);
	}

	// Remove FAK order if could not be filled

	if (!bids_.empty()) cancelFAK(bids_);
	if (!asks_.empty()) cancelFAK(asks_);

	return trades;
}

void OrderBook::CancelOrderInternal(OrderId orderId)
{
	if (!orders_.contains(orderId))
		return;

	// Remove order from the aggregate orders map

	const auto [order, iterator] = orders_.at(orderId);
	orders_.erase(orderId);

	// Lambda to remove the order from its level on the orderbook

	auto removeOrderFromLevel = [&](auto& side, auto& order)
		{
			auto price = order->GetPrice();
			auto& level = side.at(price);

			// If removal of order leaves a level empty, clear the level

			level.erase(iterator);
			if (level.empty()) side.erase(price);
		};

	if (order->GetSide() == Side::Sell) removeOrderFromLevel(asks_, order);
	if (order->GetSide() == Side::Buy) removeOrderFromLevel(bids_, order);

	// Update the levels info struct

	OrderCancelEventInternal(order);
}

void OrderBook::CancelOrdersInternal(OrderIds orderIds)
{
	for (auto& orderId : orderIds) CancelOrderInternal(orderId);
} 

void OrderBook::UpdateLevelsInternal(Price price, Quantity quantity, OrderEvent event)
{
	auto& levelDepth = levels_[price];

	switch (event)
	{
	case OrderEvent::AddOrder:
		levelDepth.quantity_ += quantity;
		levelDepth.count_ += 1;
		break;
	case OrderEvent::CancelOrder:
		levelDepth.quantity_ -= quantity;
		levelDepth.count_ -= 1;
		break;
	case OrderEvent::MatchOrder:
		levelDepth.quantity_ -= quantity;
		break;
	default:
		break;
	}

	if (levelDepth.count_ == 0) levels_.erase(price);
}

void OrderBook::OrderAddEventInternal(OrderPointer order)
{
	UpdateLevelsInternal
	(
		order->GetPrice(),
		order->GetRemainingQuantity(),
		OrderEvent::AddOrder
	);
}

void OrderBook::OrderCancelEventInternal(OrderPointer order)
{
	UpdateLevelsInternal
	(
		order->GetPrice(),
		order->GetRemainingQuantity(),
		OrderEvent::CancelOrder
	);
}

void OrderBook::OrderMatchEventInternal(Price price, Quantity quantity, bool isFullyFilled)
{
	UpdateLevelsInternal
	(
		price,
		quantity,
		isFullyFilled ? OrderEvent::CancelOrder : OrderEvent::MatchOrder
	);
}