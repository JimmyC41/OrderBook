#include <iostream>

#include "../include/OrderBook.h"

int main()
{
	OrderBook orderbook;
	orderbook.AddOrder(std::make_shared<Order>(1, OrderType::GoodTillCancel, Side::Buy, 90, 10));
	orderbook.AddOrder(std::make_shared<Order>(2, OrderType::GoodTillCancel, Side::Buy, 100, 10));
	orderbook.AddOrder(std::make_shared<Order>(3, OrderType::FillAndKill, Side::Sell, 95, 5));
	orderbook.AddOrder(std::make_shared<Order>(4, OrderType::Market, Side::Sell, 1000, 3));
	orderbook.Display();
	//orderbook.AddOrder(std::make_shared<Order>(1, OrderType::FillOrKill, Side::Buy, 100, 1));
	//orderbook.Display();
	return 0;
}