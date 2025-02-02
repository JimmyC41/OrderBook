# Order Book Engine
* Supports the following order types: GTC, Market, FAK, FOK.
* Simultaneous order requests handled synchronously through a lock-free queue (provided by the Boost library).
* Log of orders and trades written to a generated file.

### Project Goals
1. Implement best practices gleaned from Meyer's "Effective Modern C++." Code is much 'prettier' compared to the absolute backend atrocity that is TexasHoldEmV1, my previous project.
2. Gain familiarity with the C++ concurrency API.
3. Use the unit testing framework provided by GTest.

### Simulation Benchmark
Paramaters for the benchmark, such as the distribution of order types (GTC, Market, FAK, FOK), or the std deviation of prices and quantity for the random order generation can be found (and tweaked) in BenchmarkParams.h.

The following benchmark result uses the following parameters to simulate real-life order flow:
* Event Types: 90% add order, 5% modify order and 5% cancel order.
* Order Types: 60% GTC, 10% market, 25% FAK and 5% FOK.
* Side: 50% buy, 50% sell.
* Prices: Median of 1000.0 and std deviation of 50.0.
* Quantity: LogNormal(3.0, 0.5).

**WARNING:** Running the benchmark in Benchmark.cpp will generate ~2GB of log files in Benchmark/Debug. Make sure to comment out FileLogger in OrderBook.cpp to disable logging.

![Benchmark Results] (images/BenchmarkResult)

You may also wish to display outstanding orders sitting on the orderbook following the simulation. Note that the 'final' state of the orderbook is only displayed once ALL orders have been matched (the thread will block until all events in the queue have been processed). Expect a slight delay if you simulate a large number of orders (1,000,000 and above). For reference, here's what the log and orderbook looks like after 25 random events.

![Display] (images/ExampleDisplay)
![Order Log] (images/ExampleLog)
