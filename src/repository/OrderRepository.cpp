#include "OrderRepository.h"
#include "../json/JsonParser.h"
#include <stdexcept>

OrderRepository::OrderRepository(const std::string& filePath) : filePath_(filePath) {}

void OrderRepository::save(const Order& order) {
    JsonValue root = load();
    auto& arr = root["orders"];
    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i]["orderId"].asString() == order.orderId) {
            arr[i] = toJson(order);
            JsonParser::writeFile(filePath_, root);
            return;
        }
    }
    arr.push(toJson(order));
    JsonParser::writeFile(filePath_, root);
}

Order OrderRepository::findById(const std::string& orderId) {
    JsonValue root = load();
    const auto& arr = root["orders"];
    for (size_t i = 0; i < arr.size(); ++i)
        if (arr[i]["orderId"].asString() == orderId) return fromJson(arr[i]);
    throw std::runtime_error("Order not found: " + orderId);
}

std::vector<Order> OrderRepository::findAll() {
    JsonValue root = load();
    std::vector<Order> result;
    const auto& arr = root["orders"];
    for (size_t i = 0; i < arr.size(); ++i)
        result.push_back(fromJson(arr[i]));
    return result;
}

std::vector<Order> OrderRepository::findByStatus(OrderStatus status) {
    std::vector<Order> result;
    for (const auto& o : findAll())
        if (o.status == status) result.push_back(o);
    return result;
}

void OrderRepository::updateStatus(const std::string& orderId, OrderStatus status) {
    JsonValue root = load();
    auto& arr = root["orders"];
    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i]["orderId"].asString() == orderId) {
            arr[i]["status"] = JsonValue::fromString(orderStatusToString(status));
            JsonParser::writeFile(filePath_, root);
            return;
        }
    }
    throw std::runtime_error("Order not found: " + orderId);
}

JsonValue OrderRepository::load() {
    JsonValue v = JsonParser::parseFile(filePath_);
    if (v.isNull()) {
        v = JsonValue::makeObject();
        v["orders"] = JsonValue::makeArray();
    }
    return v;
}

JsonValue OrderRepository::toJson(const Order& o) {
    JsonValue obj = JsonValue::makeObject();
    obj["orderId"]      = JsonValue::fromString(o.orderId);
    obj["sampleId"]     = JsonValue::fromString(o.sampleId);
    obj["customerName"] = JsonValue::fromString(o.customerName);
    obj["quantity"]     = JsonValue::fromDouble(o.quantity);
    obj["status"]       = JsonValue::fromString(orderStatusToString(o.status));
    obj["createdAt"]    = JsonValue::fromString(o.createdAt);
    return obj;
}

Order OrderRepository::fromJson(const JsonValue& j) {
    return { j["orderId"].asString(), j["sampleId"].asString(),
             j["customerName"].asString(), j["quantity"].asInt(),
             stringToOrderStatus(j["status"].asString()), j["createdAt"].asString() };
}
