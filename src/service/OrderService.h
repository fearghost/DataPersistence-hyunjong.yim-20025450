#pragma once
#include "../repository/ISampleRepository.h"
#include "../repository/IOrderRepository.h"
#include "../repository/IProductionRepository.h"
#include <string>
#include <cmath>

// Business logic layer — depends on interfaces only, enabling mock-based testing.
class OrderService {
public:
    OrderService(ISampleRepository& samples, IOrderRepository& orders, IProductionRepository& production);

    void placeOrder(const std::string& sampleId, const std::string& customerName,
                    int quantity, const std::string& orderId, const std::string& createdAt);

    // Approval: stock sufficient → CONFIRMED + stock deducted
    //           stock insufficient → PRODUCING + enqueued
    void approveOrder(const std::string& orderId);

    void rejectOrder(const std::string& orderId);
    void processShipment(const std::string& orderId);
    void completeProduction(const std::string& orderId);

private:
    ISampleRepository& samples_;
    IOrderRepository& orders_;
    IProductionRepository& production_;

    static int nextQueueId();
};
