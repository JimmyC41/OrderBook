#pragma once

#include <random>
#include <vector>

struct BenchmarkParams
{
	int numEvents_;
	std::pair<int, int> orderIdDist_;
	std::vector<int> eventTypeDist_;
	std::vector<int> orderTypeDist_;
	double sideDist_;
	std::pair<double, double> priceDist_;
	std::pair<double, double> quantityDist_;
};

inline BenchmarkParams DefaultParams(int numEvents)
{
	return BenchmarkParams
	{
		numEvents,
		{1, numEvents * 0.8},
		{90, 5, 5},
		{60, 10, 25, 5},
		0.5,
		{1000.0, 100.0},
		{6.0, 1.0}
	};
}