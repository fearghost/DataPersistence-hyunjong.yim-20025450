#pragma once
#include <gmock/gmock.h>
#include "../../src/repository/IOrderRepository.h"

class MockOrderRepository : public IOrderRepository {
public:
    MOCK_METHOD(void, save, (const Order& order), (override));
    MOCK_METHOD(Order, findById, (const std::string& orderId), (override));
    MOCK_METHOD(std::vector<Order>, findAll, (), (override));
    MOCK_METHOD(std::vector<Order>, findByStatus, (OrderStatus status), (override));
    MOCK_METHOD(void, updateStatus, (const std::string& orderId, OrderStatus status), (override));
};
