#pragma once
#include <vector>
#include <optional>
#include <string>
#include "../model/ProductionQueue.h"

class IProductionRepository {
public:
    virtual ~IProductionRepository() = default;
    virtual void enqueue(const ProductionItem& item) = 0;
    virtual std::optional<ProductionItem> dequeue() = 0;
    virtual std::vector<ProductionItem> getQueue() = 0;
    virtual std::optional<ProductionItem> getCurrent() = 0;
    virtual void setCurrent(const std::optional<ProductionItem>& item) = 0;
    virtual void remove(int queueId) = 0;
};
