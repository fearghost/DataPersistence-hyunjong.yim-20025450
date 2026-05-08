#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

// Minimal JSON value type for PoC data persistence.
// Reference: https://github.com/fearghost/PoC_JsonParser
class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    static JsonValue makeNull()                     { return JsonValue(); }
    static JsonValue fromBool(bool v)               { JsonValue j; j.type_ = Type::Bool;   j.bool_ = v;   return j; }
    static JsonValue fromDouble(double v)           { JsonValue j; j.type_ = Type::Number; j.num_  = v;   return j; }
    static JsonValue fromString(const std::string& v){ JsonValue j; j.type_ = Type::String; j.str_  = v;   return j; }
    static JsonValue makeArray()                    { JsonValue j; j.type_ = Type::Array;  return j; }
    static JsonValue makeObject()                   { JsonValue j; j.type_ = Type::Object; return j; }

    Type        getType()  const { return type_; }
    bool        isNull()   const { return type_ == Type::Null; }
    bool        isBool()   const { return type_ == Type::Bool; }
    bool        isNumber() const { return type_ == Type::Number; }
    bool        isString() const { return type_ == Type::String; }
    bool        isArray()  const { return type_ == Type::Array; }
    bool        isObject() const { return type_ == Type::Object; }

    bool        asBool()   const { return bool_; }
    double      asDouble() const { return num_; }
    int         asInt()    const { return static_cast<int>(num_); }
    const std::string& asString() const { return str_; }
    const std::vector<JsonValue>& asArray()  const { return arr_; }
    const std::map<std::string, JsonValue>& asObject() const { return obj_; }

    // Array
    void push(const JsonValue& v) { arr_.push_back(v); }
    size_t size() const { return type_ == Type::Array ? arr_.size() : obj_.size(); }

    JsonValue& operator[](size_t i)             { return arr_[i]; }
    const JsonValue& operator[](size_t i) const { return arr_[i]; }

    // Object
    bool contains(const std::string& key) const {
        return type_ == Type::Object && obj_.count(key) > 0;
    }

    JsonValue& operator[](const std::string& key) {
        if (type_ == Type::Null) { type_ = Type::Object; }
        return obj_[key];
    }
    const JsonValue& operator[](const std::string& key) const {
        auto it = obj_.find(key);
        if (it == obj_.end()) throw std::runtime_error("Key not found: " + key);
        return it->second;
    }

private:
    Type type_ = Type::Null;
    bool bool_ = false;
    double num_ = 0.0;
    std::string str_;
    std::vector<JsonValue> arr_;
    std::map<std::string, JsonValue> obj_;
};
