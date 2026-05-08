#pragma once
#include <gmock/gmock.h>
#include "../../src/repository/IProductionRepository.h"

class MockProductionRepository : public IProductionRepository {
public:
    MOCK_METHOD(void, enqueue, (const ProductionItem& item), (override));
    MOCK_METHOD(std::optional<ProductionItem>, dequeue, (), (override));
    MOCK_METHOD(std::vector<ProductionItem>, getQueue, (), (override));
    MOCK_METHOD(std::optional<ProductionItem>, getCurrent, (), (override));
    MOCK_METHOD(void, setCurrent, (const std::optional<ProductionItem>& item), (override));
    MOCK_METHOD(void, remove, (int queueId), (override));
};
