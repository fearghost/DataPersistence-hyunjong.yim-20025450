#pragma once
#include <string>
#include "JsonValue.h"

// JSON serializer / deserializer.
// Reference: https://github.com/fearghost/PoC_JsonParser
class JsonParser {
public:
    static JsonValue parse(const std::string& json);
    static std::string stringify(const JsonValue& value, int indent = 2);

    static JsonValue parseFile(const std::string& path);
    static void writeFile(const std::string& path, const JsonValue& value, int indent = 2);

private:
    struct Ctx {
        const std::string& input;
        size_t pos = 0;
        explicit Ctx(const std::string& s) : input(s) {}
    };

    static void        skip(Ctx& c);
    static JsonValue   parseValue(Ctx& c);
    static JsonValue   parseObject(Ctx& c);
    static JsonValue   parseArray(Ctx& c);
    static JsonValue   parseString(Ctx& c);
    static JsonValue   parseNumber(Ctx& c);
    static JsonValue   parseLiteral(Ctx& c);

    static std::string stringifyImpl(const JsonValue& v, int indent, int depth);
    static std::string escape(const std::string& s);
};
