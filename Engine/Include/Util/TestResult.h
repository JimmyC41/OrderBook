#pragma once

#include <vector>

struct TestResult {
    std::size_t allCount_;
    std::size_t bidCount_;
    std::size_t askCount_;
};

using TestResults = std::vector<TestResult>;