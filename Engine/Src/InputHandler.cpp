#include <fstream>
#include <charconv>
#include <stdexcept>

#include "../Include/Util/InputHandler.h"

std::tuple<EventInformations, TestResult> InputHandler::GetEventInformationsFromFile(const std::filesystem::path& path) const
{
    EventInformations events;
    events.reserve(1'000);

    std::string line;
    std::ifstream file{ path };
    while (std::getline(file, line))
    {
        if (line.empty()) break;

        const bool isResult = line.at(0) == 'R';
        const bool isEvent = !isResult;

        if (isEvent)
        {
            EventInformation event;
            auto isValid = TryParseEventInfo(line, event);
            if (!isValid) continue;
            events.push_back(event);
        }
        else
        {
            if (!file.eof())
                throw std::logic_error("Result should only be specified at the end.");

            TestResult result;
            auto isValid = TryParseTestResult(line, result);
            if (!isValid) continue;
            return { events, result };
        }
    }

    throw std::logic_error("No result specified.");
}

bool InputHandler::TryParseEventInfo(const std::string_view& str, EventInformation& event) const
{
    auto value = str.at(0);
    auto values = Split(str, ' ');

    if (value == 'A')
    {
        event.eventType_ = EventType::AddOrder;
        event.orderId_ = ParseOrderId(values[1]);
        event.orderType_ = ParseOrderType(values[2]);
        event.side_ = ParseSide(values[3]);
        event.price_ = ParsePrice(values[4]);
        event.quantity_ = ParseQuantity(values[5]);
    }
    else if (value == 'M')
    {
        event.eventType_ = EventType::ModifyOrder;
        event.orderId_ = ParseOrderId(values[1]);
        event.side_ = ParseSide(values[2]);
        event.price_ = ParsePrice(values[3]);
        event.quantity_ = ParseQuantity(values[4]);
    }
    else if (value == 'C')
    {
        event.eventType_ = EventType::CancelOrder;
        event.orderId_ = ParseOrderId(values[1]);
    }
    else
    {
        return false;
    }

    return true;
}

bool InputHandler::TryParseTestResult(const std::string_view& str, TestResult& result) const
{
    if (str.at(0) != 'R')
        return false;

    auto values = Split(str, ' ');
    result.allCount_ = ToNumber(values[1]);
    result.bidCount_ = ToNumber(values[2]);
    result.askCount_ = ToNumber(values[3]);
    return true;
}

OrderId InputHandler::ParseOrderId(const std::string_view& str) const
{
    if (str.empty())
        throw std::logic_error("Empty OrderId.");
    return static_cast<OrderId>(ToNumber(str));
}

OrderType InputHandler::ParseOrderType(const std::string_view& str) const
{
    if (str == "FillAndKill") return OrderType::FillAndKill;
    if (str == "GoodTillCancel") return OrderType::GoodTillCancel;
    if (str == "FillOrKill") return OrderType::FillOrKill;
    if (str == "Market") return OrderType::Market;
    throw std::logic_error("OrderType N/A.");
}

Side InputHandler::ParseSide(const std::string_view& str) const
{
    if (str == "B") return Side::Buy;
    if (str == "S") return Side::Sell;
    throw std::logic_error("Side N/A.");
}

Price InputHandler::ParsePrice(const std::string_view& str) const
{
    if (str.empty())
        throw std::logic_error("Price N/A.");
    return ToNumber(str);
}

Quantity InputHandler::ParseQuantity(const std::string_view& str) const
{
    if (str.empty())
        throw std::logic_error("Quantity N/A.");
    return ToNumber(str);
}

std::uint32_t InputHandler::ToNumber(const std::string_view& str) const
{
    std::int64_t value{};
    std::from_chars(str.data(), str.data() + str.size(), value);
    if (value < 0) throw std::logic_error("Value is below zero.");
    return static_cast<std::uint32_t>(value);
}

std::vector<std::string_view> InputHandler::Split(const std::string_view& str, char delimiter) const
{
    std::vector<std::string_view> columns;
    columns.reserve(5);

    std::size_t startIndex{}, endIndex{};
    while ((endIndex = str.find(delimiter, startIndex)) != std::string::npos)
    {
        auto distance = endIndex - startIndex;
        auto column = str.substr(startIndex, distance);
        startIndex = endIndex + 1;
        columns.push_back(column);
    }
    columns.push_back(str.substr(startIndex));
    return columns;
}