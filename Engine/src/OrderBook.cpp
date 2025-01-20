#include "../Include/OrderBook/OrderBook.h"

void OrderBook::AddOrderToQueue(OrderId id, OrderType type, Side side, Price price, Quantity quantity)
{
	queueManager_.EnqueueEvent(QueueEvent
		{
			EventType::AddOrder,
			AddOrderPayload{ id, type, side, price, quantity}
		});
}

void OrderBook::ModifyOrderToQueue(OrderId id, Side side, Price price, Quantity quantity)
{
	queueManager_.EnqueueEvent(QueueEvent
		{
			EventType::ModifyOrder,
			ModifyOrderPayload{ id, side, price, quantity }
		});
}

void OrderBook::CancelOrderToQueue(OrderId id)
{
	queueManager_.EnqueueEvent(QueueEvent
		{
			EventType::CancelOrder,
			CancelOrderPayload{ id }
		});
}

void OrderBook::Display() const
{
	queueManager_.WaitForAllEvents();

	std::scoped_lock ordersLock{ ordersMutex_ };
	
	if (orders_.empty())
	{
		std::cout << "Orderbook is empty!\n" << std::endl;
		return;
	}

	std::cout << "------ Displaying the Orderbook ------\n";
	std::cout << std::format("Orderbook contains {} orders\n", orders_.size()) << std::endl;

	auto orderInfos = GetOrderInfos();
	auto bidLevelInfos = orderInfos.GetBids();
	auto askLevelInfos = orderInfos.GetAsks();

	std::cout << "Displaying Bids:\n";
	for (const auto& bidLevel : bidLevelInfos)
	{
		std::cout << std::format("\t{{ Price: {}, Quantity: {} }}\n",
			bidLevel.price_, bidLevel.quantity_);
	}

	std::cout << "Displaying Asks:\n";
	for (const auto& askLevel : askLevelInfos)
	{
		std::cout << std::format("\t{{ Price: {}, Quantity: {} }}\n",
			askLevel.price_, askLevel.quantity_);
	}

	std::cout << "\n";
}

std::size_t OrderBook::Size() const
{
	queueManager_.WaitForAllEvents();

	std::scoped_lock ordersLock{ ordersMutex_ };
	return orders_.size();
}

OrderBookLevelInfos OrderBook::GetOrderInfos() const
{
	queueManager_.WaitForAllEvents();

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

Trades OrderBook::AddOrderInternal(OrderPointer order)
{
	if (orders_.contains(order->GetOrderId()))
	{
		// std::cout << std::format("Add: Order with {} already exists!\n", order->GetOrderId()) << std::flush;
		return { };
	}

	// Check if FAK can be matched

	if (order->GetOrderType() == OrderType::FillAndKill &&
		!CanMatchInternal(order->GetSide(), order->GetPrice()))
	{
		// std::cout << std::format("Add: FAK Order {} cannot be matched!\n", order->GetOrderId()) << std::flush;
		return { };
	}

	// Check if FOK can be fully filled

	if (order->GetOrderType() == OrderType::FillOrKill &&
		!CanBeFullyFilledInternal(order->GetSide(), order->GetPrice(), order->GetRemainingQuantity()))
	{
		// std::cout << std::format("Add: FOK Order {} cannot be fully filled!\n", order->GetOrderId()) << std::flush;
		return { };
	}

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
	// std::cout << std::format("Add: {}\n", order->ToString()) << std::flush;

	// Run matching algorithm and return new trades (if any)

	return MatchOrdersInternal();
}

Trades OrderBook::ModifyOrderInternal(OrderModify order)
{
	OrderType orderType;

	if (!orders_.contains(order.GetOrderId()))
		// std::cout << std::format("Modify: Order {} does not exist!\n", order.GetOrderId());
		return { };

	const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
	orderType = existingOrder->GetOrderType();

	// Cancel the existing order and add the modified order, returning any trades

	CancelOrderInternal(order.GetOrderId());
	return AddOrderInternal(order.ToOrderPointer(orderType));
}

void OrderBook::CancelOrderInternal(OrderId orderId)
{
	if (!orders_.contains(orderId))
		// std::cout << std::format("Cancel: Order {} does not exist!\n", orderId);
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

	// std::cout << std::format("Cancel: {}\n", order->ToString());
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

void OrderBook::HandleEvent(const QueueEvent& event)
{
	std::scoped_lock ordersLock{ ordersMutex_ };

	std::visit([this](auto&& payload)
	{
		using T = std::decay_t<decltype(payload)>;
		if constexpr (std::is_same_v<T, AddOrderPayload>)
			HandleAddOrderInternal(payload);
		else if constexpr (std::is_same_v<T, ModifyOrderPayload>)
			HandleModifyOrderInternal(payload);
		else if constexpr (std::is_same_v<T, CancelOrderPayload>)
			HandleCancelOrderInternal(payload);
	}, event.payload_);
}

void OrderBook::HandleAddOrderInternal(const AddOrderPayload& payload)
{
	AddOrderInternal(std::make_shared<Order>(
		payload.orderId_,
		payload.orderType_,
		payload.side_,
		payload.price_,
		payload.quantity_
	));
}

void OrderBook::HandleModifyOrderInternal(const ModifyOrderPayload& payload)
{
	ModifyOrderInternal(OrderModify{
		payload.orderId_,
		payload.side_,
		payload.price_,
		payload.quantity_
	});
}

void OrderBook::HandleCancelOrderInternal(const CancelOrderPayload& payload)
{
	CancelOrderInternal(payload.orderId_);
}