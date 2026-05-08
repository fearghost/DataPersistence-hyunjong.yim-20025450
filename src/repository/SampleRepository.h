#pragma once
#include "ISampleRepository.h"
#include "../json/JsonValue.h"
#include <string>

class SampleRepository : public ISampleRepository {
public:
    explicit SampleRepository(const std::string& filePath);

    void save(const Sample& sample) override;
    Sample findById(const std::string& id) override;
    std::vector<Sample> findAll() override;
    bool existsById(const std::string& id) override;
    void updateStock(const std::string& id, int delta) override;

private:
    std::string filePath_;

    JsonValue load();
    static JsonValue toJson(const Sample& s);
    static Sample fromJson(const JsonValue& j);
};
