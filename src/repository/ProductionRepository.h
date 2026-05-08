#pragma once
#include "IProductionRepository.h"
#include "../json/JsonValue.h"
#include <string>

class ProductionRepository : public IProductionRepository {
public:
    explicit ProductionRepository(const std::string& filePath);

    void enqueue(const ProductionItem& item) override;
    std::optional<ProductionItem> dequeue() override;
    std::vector<ProductionItem> getQueue() override;
    std::optional<ProductionItem> getCurrent() override;
    void setCurrent(const std::optional<ProductionItem>& item) override;
    void remove(int queueId) override;

private:
    std::string filePath_;

    JsonValue load();
    static JsonValue toJson(const ProductionItem& p);
    static ProductionItem fromJson(const JsonValue& j);
};
