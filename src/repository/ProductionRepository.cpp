#include "ProductionRepository.h"
#include "../json/JsonParser.h"

ProductionRepository::ProductionRepository(const std::string& filePath) : filePath_(filePath) {}

void ProductionRepository::enqueue(const ProductionItem& item) {
    JsonValue root = load();
    root["queue"].push(toJson(item));
    JsonParser::writeFile(filePath_, root);
}

std::optional<ProductionItem> ProductionRepository::dequeue() {
    JsonValue root = load();
    auto& arr = root["queue"];
    if (arr.size() == 0) return std::nullopt;
    ProductionItem first = fromJson(arr[0]);
    // Rebuild array without first element
    JsonValue newArr = JsonValue::makeArray();
    for (size_t i = 1; i < arr.size(); ++i) newArr.push(arr[i]);
    root["queue"] = newArr;
    JsonParser::writeFile(filePath_, root);
    return first;
}

std::vector<ProductionItem> ProductionRepository::getQueue() {
    JsonValue root = load();
    std::vector<ProductionItem> result;
    const auto& arr = root["queue"];
    for (size_t i = 0; i < arr.size(); ++i)
        result.push_back(fromJson(arr[i]));
    return result;
}

std::optional<ProductionItem> ProductionRepository::getCurrent() {
    JsonValue root = load();
    if (!root.contains("current") || root["current"].isNull())
        return std::nullopt;
    return fromJson(root["current"]);
}

void ProductionRepository::setCurrent(const std::optional<ProductionItem>& item) {
    JsonValue root = load();
    root["current"] = item ? toJson(*item) : JsonValue::makeNull();
    JsonParser::writeFile(filePath_, root);
}

void ProductionRepository::remove(int queueId) {
    JsonValue root = load();
    JsonValue newArr = JsonValue::makeArray();
    const auto& arr = root["queue"];
    for (size_t i = 0; i < arr.size(); ++i)
        if (arr[i]["queueId"].asInt() != queueId) newArr.push(arr[i]);
    root["queue"] = newArr;
    JsonParser::writeFile(filePath_, root);
}

JsonValue ProductionRepository::load() {
    JsonValue v = JsonParser::parseFile(filePath_);
    if (v.isNull()) {
        v = JsonValue::makeObject();
        v["queue"]   = JsonValue::makeArray();
        v["current"] = JsonValue::makeNull();
    }
    return v;
}

JsonValue ProductionRepository::toJson(const ProductionItem& p) {
    JsonValue obj = JsonValue::makeObject();
    obj["queueId"]          = JsonValue::fromDouble(p.queueId);
    obj["orderId"]          = JsonValue::fromString(p.orderId);
    obj["sampleId"]         = JsonValue::fromString(p.sampleId);
    obj["shortage"]         = JsonValue::fromDouble(p.shortage);
    obj["actualProduction"] = JsonValue::fromDouble(p.actualProduction);
    obj["estimatedTime"]    = JsonValue::fromDouble(p.estimatedTime);
    return obj;
}

ProductionItem ProductionRepository::fromJson(const JsonValue& j) {
    return { j["queueId"].asInt(), j["orderId"].asString(), j["sampleId"].asString(),
             j["shortage"].asInt(), j["actualProduction"].asInt(), j["estimatedTime"].asDouble() };
}
