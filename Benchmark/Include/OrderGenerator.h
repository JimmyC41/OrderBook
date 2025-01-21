#pragma once

#include <thread>
#include <chrono>
#include <random>
#include <boost/lockfree/queue.hpp>

#include "BenchmarkParams.h"
#include "Include/Util/EventInformation.h"

class OrderGenerator
{
public:

	OrderGenerator();

	OrderGenerator(const OrderGenerator&) = delete;
	OrderGenerator(OrderGenerator&&) = delete;
	OrderGenerator& operator=(const OrderGenerator&) = delete;
	OrderGenerator& operator=(OrderGenerator&&) = delete;

	// Thread entry point to generate orders and push them to the queue
	void GenerateOrders(const BenchmarkParams& p);

	// Thread entry point to send order requests to the orderbook
	void ProcessOrders(OrderBook& book);

private:

	boost::lockfree::queue<EventInformation> orderQueue_;
	std::atomic<bool> done_;

};