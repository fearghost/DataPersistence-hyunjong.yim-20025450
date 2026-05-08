#pragma once
#include <vector>
#include <string>
#include "../model/Sample.h"

class ISampleRepository {
public:
    virtual ~ISampleRepository() = default;
    virtual void save(const Sample& sample) = 0;
    virtual Sample findById(const std::string& id) = 0;
    virtual std::vector<Sample> findAll() = 0;
    virtual bool existsById(const std::string& id) = 0;
    virtual void updateStock(const std::string& id, int delta) = 0;
};
