#include "OrderService.h"
#include <stdexcept>
#include <cmath>

OrderService::OrderService(ISampleRepository& samples,
                           IOrderRepository& orders,
                           IProductionRepository& production)
    : samples_(samples), orders_(orders), production_(production) {}

void OrderService::placeOrder(const std::string& sampleId, const std::string& customerName,
                              int quantity, const std::string& orderId, const std::string& createdAt) {
    if (!samples_.existsById(sampleId))
        throw std::runtime_error("Sample not found: " + sampleId);

    Order o{ orderId, sampleId, customerName, quantity, OrderStatus::RESERVED, createdAt };
    orders_.save(o);
}

void OrderService::approveOrder(const std::string& orderId) {
    Order order = orders_.findById(orderId);
    if (order.status != OrderStatus::RESERVED)
        throw std::runtime_error("Order is not in RESERVED state");

    Sample sample = samples_.findById(order.sampleId);

    if (sample.stock >= order.quantity) {
        samples_.updateStock(sample.id, -order.quantity);
        orders_.updateStatus(orderId, OrderStatus::CONFIRMED);
    } else {
        int shortage = order.quantity - sample.stock;
        int actual   = static_cast<int>(std::ceil(shortage / (sample.yield * 0.9)));
        double time  = sample.avgProductionTime * actual;

        ProductionItem item{ nextQueueId(), orderId, sample.id, shortage, actual, time };
        production_.enqueue(item);
        orders_.updateStatus(orderId, OrderStatus::PRODUCING);
    }
}

void OrderService::rejectOrder(const std::string& orderId) {
    Order order = orders_.findById(orderId);
    if (order.status != OrderStatus::RESERVED)
        throw std::runtime_error("Only RESERVED orders can be rejected");
    orders_.updateStatus(orderId, OrderStatus::REJECTED);
}

void OrderService::processShipment(const std::string& orderId) {
    Order order = orders_.findById(orderId);
    if (order.status != OrderStatus::CONFIRMED)
        throw std::runtime_error("Only CONFIRMED orders can be shipped");
    orders_.updateStatus(orderId, OrderStatus::RELEASED);
}

void OrderService::completeProduction(const std::string& orderId) {
    Order order = orders_.findById(orderId);
    if (order.status != OrderStatus::PRODUCING)
        throw std::runtime_error("Order is not in PRODUCING state");
    orders_.updateStatus(orderId, OrderStatus::CONFIRMED);
}

int OrderService::nextQueueId() {
    static int id = 0;
    return ++id;
}
