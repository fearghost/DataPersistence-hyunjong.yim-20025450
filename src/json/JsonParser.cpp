#include "JsonParser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

// ---- public ----------------------------------------------------------------

JsonValue JsonParser::parse(const std::string& json) {
    Ctx c(json);
    skip(c);
    return parseValue(c);
}

std::string JsonParser::stringify(const JsonValue& value, int indent) {
    return stringifyImpl(value, indent, 0);
}

JsonValue JsonParser::parseFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return JsonValue::makeNull();
    std::string content((std::istreambuf_iterator<char>(f)), {});
    return parse(content);
}

void JsonParser::writeFile(const std::string& path, const JsonValue& value, int indent) {
    std::ofstream f(path);
    if (!f.is_open()) throw std::runtime_error("Cannot open file: " + path);
    f << stringify(value, indent);
}

// ---- parsing ---------------------------------------------------------------

void JsonParser::skip(Ctx& c) {
    while (c.pos < c.input.size() && std::isspace((unsigned char)c.input[c.pos]))
        ++c.pos;
}

JsonValue JsonParser::parseValue(Ctx& c) {
    skip(c);
    if (c.pos >= c.input.size()) throw std::runtime_error("Unexpected end of input");
    char ch = c.input[c.pos];
    if (ch == '{') return parseObject(c);
    if (ch == '[') return parseArray(c);
    if (ch == '"') return parseString(c);
    if (ch == 't' || ch == 'f' || ch == 'n') return parseLiteral(c);
    if (ch == '-' || std::isdigit((unsigned char)ch)) return parseNumber(c);
    throw std::runtime_error(std::string("Unexpected char: ") + ch);
}

JsonValue JsonParser::parseObject(Ctx& c) {
    ++c.pos;
    JsonValue obj = JsonValue::makeObject();
    skip(c);
    if (c.pos < c.input.size() && c.input[c.pos] == '}') { ++c.pos; return obj; }

    while (true) {
        skip(c);
        if (c.input[c.pos] != '"') throw std::runtime_error("Expected key string");
        std::string key = parseString(c).asString();
        skip(c);
        if (c.input[c.pos] != ':') throw std::runtime_error("Expected ':'");
        ++c.pos;
        obj[key] = parseValue(c);
        skip(c);
        if (c.pos >= c.input.size()) throw std::runtime_error("Unterminated object");
        if (c.input[c.pos] == '}') { ++c.pos; break; }
        if (c.input[c.pos] != ',') throw std::runtime_error("Expected ',' or '}'");
        ++c.pos;
    }
    return obj;
}

JsonValue JsonParser::parseArray(Ctx& c) {
    ++c.pos;
    JsonValue arr = JsonValue::makeArray();
    skip(c);
    if (c.pos < c.input.size() && c.input[c.pos] == ']') { ++c.pos; return arr; }

    while (true) {
        arr.push(parseValue(c));
        skip(c);
        if (c.pos >= c.input.size()) throw std::runtime_error("Unterminated array");
        if (c.input[c.pos] == ']') { ++c.pos; break; }
        if (c.input[c.pos] != ',') throw std::runtime_error("Expected ',' or ']'");
        ++c.pos;
    }
    return arr;
}

JsonValue JsonParser::parseString(Ctx& c) {
    ++c.pos;
    std::string result;
    while (c.pos < c.input.size() && c.input[c.pos] != '"') {
        if (c.input[c.pos] == '\\') {
            ++c.pos;
            switch (c.input[c.pos]) {
            case '"':  result += '"';  break;
            case '\\': result += '\\'; break;
            case '/':  result += '/';  break;
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            default:   result += c.input[c.pos];
            }
        } else {
            result += c.input[c.pos];
        }
        ++c.pos;
    }
    ++c.pos;
    return JsonValue::fromString(result);
}

JsonValue JsonParser::parseNumber(Ctx& c) {
    size_t start = c.pos;
    if (c.input[c.pos] == '-') ++c.pos;
    while (c.pos < c.input.size() && std::isdigit((unsigned char)c.input[c.pos])) ++c.pos;
    if (c.pos < c.input.size() && c.input[c.pos] == '.') {
        ++c.pos;
        while (c.pos < c.input.size() && std::isdigit((unsigned char)c.input[c.pos])) ++c.pos;
    }
    return JsonValue::fromDouble(std::stod(c.input.substr(start, c.pos - start)));
}

JsonValue JsonParser::parseLiteral(Ctx& c) {
    if (c.input.compare(c.pos, 4, "true")  == 0) { c.pos += 4; return JsonValue::fromBool(true); }
    if (c.input.compare(c.pos, 5, "false") == 0) { c.pos += 5; return JsonValue::fromBool(false); }
    if (c.input.compare(c.pos, 4, "null")  == 0) { c.pos += 4; return JsonValue::makeNull(); }
    throw std::runtime_error("Unknown literal");
}

// ---- serialization ---------------------------------------------------------

std::string JsonParser::stringifyImpl(const JsonValue& v, int indent, int depth) {
    std::string pad(depth * indent, ' ');
    std::string inner((depth + 1) * indent, ' ');

    switch (v.getType()) {
    case JsonValue::Type::Null:   return "null";
    case JsonValue::Type::Bool:   return v.asBool() ? "true" : "false";
    case JsonValue::Type::Number: {
        double d = v.asDouble();
        if (d == static_cast<long long>(d)) return std::to_string(static_cast<long long>(d));
        std::ostringstream oss; oss << d; return oss.str();
    }
    case JsonValue::Type::String: return "\"" + escape(v.asString()) + "\"";
    case JsonValue::Type::Array: {
        if (v.size() == 0) return "[]";
        std::string r = "[\n";
        const auto& arr = v.asArray();
        for (size_t i = 0; i < arr.size(); ++i) {
            r += inner + stringifyImpl(arr[i], indent, depth + 1);
            if (i + 1 < arr.size()) r += ",";
            r += "\n";
        }
        return r + pad + "]";
    }
    case JsonValue::Type::Object: {
        if (v.size() == 0) return "{}";
        std::string r = "{\n";
        const auto& obj = v.asObject();
        size_t i = 0;
        for (const auto& [k, val] : obj) {
            r += inner + "\"" + k + "\": " + stringifyImpl(val, indent, depth + 1);
            if (++i < obj.size()) r += ",";
            r += "\n";
        }
        return r + pad + "}";
    }
    }
    return "null";
}

std::string JsonParser::escape(const std::string& s) {
    std::string r;
    for (char c : s) {
        switch (c) {
        case '"':  r += "\\\""; break;
        case '\\': r += "\\\\"; break;
        case '\n': r += "\\n";  break;
        case '\r': r += "\\r";  break;
        case '\t': r += "\\t";  break;
        default:   r += c;
        }
    }
    return r;
}
