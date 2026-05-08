#pragma once
#include <vector>
#include <string>
#include "../model/Order.h"

class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;
    virtual void save(const Order& order) = 0;
    virtual Order findById(const std::string& orderId) = 0;
    virtual std::vector<Order> findAll() = 0;
    virtual std::vector<Order> findByStatus(OrderStatus status) = 0;
    virtual void updateStatus(const std::string& orderId, OrderStatus status) = 0;
};
