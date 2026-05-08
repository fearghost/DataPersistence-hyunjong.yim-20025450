#pragma once
#include "IOrderRepository.h"
#include "../json/JsonValue.h"
#include <string>

class OrderRepository : public IOrderRepository {
public:
    explicit OrderRepository(const std::string& filePath);

    void save(const Order& order) override;
    Order findById(const std::string& orderId) override;
    std::vector<Order> findAll() override;
    std::vector<Order> findByStatus(OrderStatus status) override;
    void updateStatus(const std::string& orderId, OrderStatus status) override;

private:
    std::string filePath_;

    JsonValue load();
    static JsonValue toJson(const Order& o);
    static Order fromJson(const JsonValue& j);
};
