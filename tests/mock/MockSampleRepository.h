#pragma once
#include <gmock/gmock.h>
#include "../../src/repository/ISampleRepository.h"

class MockSampleRepository : public ISampleRepository {
public:
    MOCK_METHOD(void, save, (const Sample& sample), (override));
    MOCK_METHOD(Sample, findById, (const std::string& id), (override));
    MOCK_METHOD(std::vector<Sample>, findAll, (), (override));
    MOCK_METHOD(bool, existsById, (const std::string& id), (override));
    MOCK_METHOD(void, updateStock, (const std::string& id, int delta), (override));
};
