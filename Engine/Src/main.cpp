#include <iostream>

#include "../include/OrderBook/OrderBook.h"

int main()
{
	OrderBook orderbook;
	
	orderbook.AddOrderToQueue(1, OrderType::GoodTillCancel, Side::Buy, 100, 10);
	orderbook.AddOrderToQueue(2, OrderType::GoodTillCancel, Side::Buy, 90, 10);
	orderbook.AddOrderToQueue(3, OrderType::FillAndKill, Side::Sell, 95, 5);
	orderbook.AddOrderToQueue(4, OrderType::Market, Side::Sell, 0, 5);
	orderbook.Display();

	return 0;
}	