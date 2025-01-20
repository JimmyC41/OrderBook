#pragma once

#include <filesystem>
#include <vector>
#include <string_view>
#include <tuple>
#include <stdexcept>

#include "../OrderBook.h"
#include "../Order.h"
#include "../Using.h"

enum class EventType {
    AddOrder,
    ModifyOrder,
    CancelOrder,
};

struct Information {
    EventType eventType_;
    OrderId orderId_;
    OrderType orderType_;
    Side side_;
    Price price_;
    Quantity quantity_;
};

using Informations = std::vector<Information>;

struct Result {
    std::size_t allCount_;
    std::size_t bidCount_;
    std::size_t askCount_;
};

using Results = std::vector<Result>;

class InputHandler {
public:
    // Reads and parses input events and expected results from a file
    std::tuple<Informations, Result> GetOrderInformations(const std::filesystem::path& path) const;

private:
    // Helper methods for parsing
    std::uint32_t ToNumber(const std::string_view& str) const;
    bool TryParseResult(const std::string_view& str, Result& result) const;
    bool TryParseOrderInfo(const std::string_view& str, Information& event) const;
    std::vector<std::string_view> Split(const std::string_view& str, char delimiter) const;

    // Specific parsers for different fields
    OrderId ParseOrderId(const std::string_view& str) const;
    OrderType ParseOrderType(const std::string_view& str) const;
    Side ParseSide(const std::string_view& str) const;
    Price ParsePrice(const std::string_view& str) const;
    Quantity ParseQuantity(const std::string_view& str) const;
};
