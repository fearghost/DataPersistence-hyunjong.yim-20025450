// Unit tests for the JSON parser utility.
#include <gtest/gtest.h>
#include "../src/json/JsonParser.h"
#include <filesystem>

namespace fs = std::filesystem;

TEST(JsonParserTest, ParseObject_BasicFields) {
    auto v = JsonParser::parse(R"({"id":"S-001","stock":480})");
    EXPECT_EQ(v["id"].asString(), "S-001");
    EXPECT_EQ(v["stock"].asInt(), 480);
}

TEST(JsonParserTest, ParseArray_MultipleElements) {
    auto v = JsonParser::parse(R"([1, 2, 3])");
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0u].asInt(), 1);
    EXPECT_EQ(v[1u].asInt(), 2);
    EXPECT_EQ(v[2u].asInt(), 3);
}

TEST(JsonParserTest, ParseDouble_Preserved) {
    auto v = JsonParser::parse(R"({"yield":0.92})");
    EXPECT_DOUBLE_EQ(v["yield"].asDouble(), 0.92);
}

TEST(JsonParserTest, ParseBool) {
    auto v = JsonParser::parse(R"({"ok":true,"fail":false})");
    EXPECT_TRUE(v["ok"].asBool());
    EXPECT_FALSE(v["fail"].asBool());
}

TEST(JsonParserTest, ParseNull) {
    auto v = JsonParser::parse(R"({"current":null})");
    EXPECT_TRUE(v["current"].isNull());
}

TEST(JsonParserTest, ParseNestedObject) {
    auto v = JsonParser::parse(R"({"order":{"id":"ORD-001","qty":200}})");
    EXPECT_EQ(v["order"]["id"].asString(),  "ORD-001");
    EXPECT_EQ(v["order"]["qty"].asInt(),    200);
}

TEST(JsonParserTest, ParseEscapedString) {
    auto v = JsonParser::parse(R"({"name":"line1\nline2"})");
    EXPECT_EQ(v["name"].asString(), "line1\nline2");
}

TEST(JsonParserTest, Stringify_ThenParse_RoundTrip) {
    JsonValue obj = JsonValue::makeObject();
    obj["id"]    = JsonValue::fromString("S-001");
    obj["stock"] = JsonValue::fromDouble(480);
    obj["yield"] = JsonValue::fromDouble(0.92);

    std::string json = JsonParser::stringify(obj);
    auto parsed = JsonParser::parse(json);

    EXPECT_EQ(parsed["id"].asString(),   "S-001");
    EXPECT_EQ(parsed["stock"].asInt(),   480);
    EXPECT_DOUBLE_EQ(parsed["yield"].asDouble(), 0.92);
}

TEST(JsonParserTest, ParseFile_MissingFile_ReturnsNull) {
    auto v = JsonParser::parseFile("nonexistent_file.json");
    EXPECT_TRUE(v.isNull());
}

TEST(JsonParserTest, WriteFile_ThenParseFile_RoundTrip) {
    fs::create_directories("test_data");
    const std::string path = "test_data/json_test.json";
    fs::remove(path);

    JsonValue obj = JsonValue::makeObject();
    obj["key"] = JsonValue::fromString("value");
    JsonParser::writeFile(path, obj);

    auto loaded = JsonParser::parseFile(path);
    EXPECT_EQ(loaded["key"].asString(), "value");

    fs::remove(path);
}

TEST(JsonParserTest, ParseEmptyObject) {
    auto v = JsonParser::parse("{}");
    EXPECT_TRUE(v.isObject());
    EXPECT_EQ(v.size(), 0u);
}

TEST(JsonParserTest, ParseEmptyArray) {
    auto v = JsonParser::parse("[]");
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.size(), 0u);
}
