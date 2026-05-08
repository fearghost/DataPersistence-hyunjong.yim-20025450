#include "SampleRepository.h"
#include "../json/JsonParser.h"
#include <stdexcept>

SampleRepository::SampleRepository(const std::string& filePath) : filePath_(filePath) {}

void SampleRepository::save(const Sample& sample) {
    JsonValue root = load();
    auto& arr = root["samples"];
    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i]["id"].asString() == sample.id) {
            arr[i] = toJson(sample);
            JsonParser::writeFile(filePath_, root);
            return;
        }
    }
    arr.push(toJson(sample));
    JsonParser::writeFile(filePath_, root);
}

Sample SampleRepository::findById(const std::string& id) {
    JsonValue root = load();
    const auto& arr = root["samples"];
    for (size_t i = 0; i < arr.size(); ++i)
        if (arr[i]["id"].asString() == id) return fromJson(arr[i]);
    throw std::runtime_error("Sample not found: " + id);
}

std::vector<Sample> SampleRepository::findAll() {
    JsonValue root = load();
    std::vector<Sample> result;
    const auto& arr = root["samples"];
    for (size_t i = 0; i < arr.size(); ++i)
        result.push_back(fromJson(arr[i]));
    return result;
}

bool SampleRepository::existsById(const std::string& id) {
    JsonValue root = load();
    const auto& arr = root["samples"];
    for (size_t i = 0; i < arr.size(); ++i)
        if (arr[i]["id"].asString() == id) return true;
    return false;
}

void SampleRepository::updateStock(const std::string& id, int delta) {
    JsonValue root = load();
    auto& arr = root["samples"];
    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i]["id"].asString() == id) {
            arr[i]["stock"] = JsonValue::fromDouble(arr[i]["stock"].asInt() + delta);
            JsonParser::writeFile(filePath_, root);
            return;
        }
    }
    throw std::runtime_error("Sample not found: " + id);
}

JsonValue SampleRepository::load() {
    JsonValue v = JsonParser::parseFile(filePath_);
    if (v.isNull()) {
        v = JsonValue::makeObject();
        v["samples"] = JsonValue::makeArray();
    }
    return v;
}

JsonValue SampleRepository::toJson(const Sample& s) {
    JsonValue obj = JsonValue::makeObject();
    obj["id"]                 = JsonValue::fromString(s.id);
    obj["name"]               = JsonValue::fromString(s.name);
    obj["avgProductionTime"]  = JsonValue::fromDouble(s.avgProductionTime);
    obj["yield"]              = JsonValue::fromDouble(s.yield);
    obj["stock"]              = JsonValue::fromDouble(s.stock);
    return obj;
}

Sample SampleRepository::fromJson(const JsonValue& j) {
    return { j["id"].asString(), j["name"].asString(),
             j["avgProductionTime"].asDouble(), j["yield"].asDouble(),
             j["stock"].asInt() };
}
