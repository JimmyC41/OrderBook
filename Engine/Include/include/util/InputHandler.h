#pragma once

#include <filesystem>
#include <string_view>
#include <tuple>
#include <stdexcept>

#include "../OrderBook/OrderBook.h"
#include "../Util/EventInformation.h"
#include "../Util/TestResult.h"

class InputHandler {
public:

    std::tuple<EventInformations, TestResult> GetEventInformationsFromFile(const std::filesystem::path& path) const;

private:

    bool TryParseEventInfo(const std::string_view& str, EventInformation& event) const;
    bool TryParseTestResult(const std::string_view& str, TestResult& result) const;

    OrderId ParseOrderId(const std::string_view& str) const;
    OrderType ParseOrderType(const std::string_view& str) const;
    Side ParseSide(const std::string_view& str) const;
    Price ParsePrice(const std::string_view& str) const;
    Quantity ParseQuantity(const std::string_view& str) const;

    std::uint32_t ToNumber(const std::string_view& str) const;
    std::vector<std::string_view> Split(const std::string_view& str, char delimiter) const;
};
