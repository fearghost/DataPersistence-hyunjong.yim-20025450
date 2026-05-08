// Unit tests for OrderService business logic using gmock.
// All file I/O is eliminated via mock repositories.
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/service/OrderService.h"
#include "mock/MockSampleRepository.h"
#include "mock/MockOrderRepository.h"
#include "mock/MockProductionRepository.h"

using ::testing::Return;
using ::testing::_;
using ::testing::Eq;

namespace {

Sample makeSample(const std::string& id, int stock, double yield = 0.9, double prodTime = 0.5) {
    return { id, "Test Sample", prodTime, yield, stock };
}

Order makeOrder(const std::string& orderId, const std::string& sampleId,
                int qty, OrderStatus status = OrderStatus::RESERVED) {
    return { orderId, sampleId, "TestCo", qty, status, "2026-05-08T00:00:00" };
}

} // namespace

class OrderServiceTest : public ::testing::Test {
protected:
    MockSampleRepository     sampleRepo;
    MockOrderRepository      orderRepo;
    MockProductionRepository prodRepo;
    OrderService             svc{ sampleRepo, orderRepo, prodRepo };
};

// ============================================================
// placeOrder
// ============================================================

TEST_F(OrderServiceTest, PlaceOrder_SampleExists_SavesCalled) {
    EXPECT_CALL(sampleRepo, existsById("S-001")).WillOnce(Return(true));
    EXPECT_CALL(orderRepo, save(_)).Times(1);

    EXPECT_NO_THROW(svc.placeOrder("S-001", "CustomerA", 100, "ORD-001", "2026-05-08T00:00:00"));
}

TEST_F(OrderServiceTest, PlaceOrder_SampleNotFound_Throws) {
    EXPECT_CALL(sampleRepo, existsById("S-999")).WillOnce(Return(false));

    EXPECT_THROW(svc.placeOrder("S-999", "CustomerA", 100, "ORD-001", "2026-05-08T00:00:00"),
                 std::runtime_error);
}

// ============================================================
// approveOrder — stock sufficient
// ============================================================

TEST_F(OrderServiceTest, ApproveOrder_SufficientStock_BecomesConfirmed) {
    EXPECT_CALL(orderRepo, findById("ORD-001")).WillOnce(Return(makeOrder("ORD-001", "S-001", 100)));
    EXPECT_CALL(sampleRepo, findById("S-001")).WillOnce(Return(makeSample("S-001", 200)));
    EXPECT_CALL(sampleRepo, updateStock("S-001", -100)).Times(1);
    EXPECT_CALL(orderRepo, updateStatus("ORD-001", OrderStatus::CONFIRMED)).Times(1);
    EXPECT_CALL(prodRepo, enqueue(_)).Times(0); // must NOT enqueue

    EXPECT_NO_THROW(svc.approveOrder("ORD-001"));
}

TEST_F(OrderServiceTest, ApproveOrder_ExactStock_BecomesConfirmed) {
    EXPECT_CALL(orderRepo, findById("ORD-002")).WillOnce(Return(makeOrder("ORD-002", "S-001", 50)));
    EXPECT_CALL(sampleRepo, findById("S-001")).WillOnce(Return(makeSample("S-001", 50)));
    EXPECT_CALL(sampleRepo, updateStock("S-001", -50)).Times(1);
    EXPECT_CALL(orderRepo, updateStatus("ORD-002", OrderStatus::CONFIRMED)).Times(1);
    EXPECT_CALL(prodRepo, enqueue(_)).Times(0);

    EXPECT_NO_THROW(svc.approveOrder("ORD-002"));
}

// ============================================================
// approveOrder — stock insufficient → PRODUCING
// ============================================================

TEST_F(OrderServiceTest, ApproveOrder_InsufficientStock_BecomesProducing) {
    EXPECT_CALL(orderRepo, findById("ORD-003")).WillOnce(Return(makeOrder("ORD-003", "S-001", 200)));
    EXPECT_CALL(sampleRepo, findById("S-001")).WillOnce(Return(makeSample("S-001", 30, 0.92)));
    EXPECT_CALL(prodRepo, enqueue(_)).Times(1);                                  // must enqueue
    EXPECT_CALL(orderRepo, updateStatus("ORD-003", OrderStatus::PRODUCING)).Times(1);
    EXPECT_CALL(sampleRepo, updateStock(_, _)).Times(0);                         // no deduction yet

    EXPECT_NO_THROW(svc.approveOrder("ORD-003"));
}

TEST_F(OrderServiceTest, ApproveOrder_ZeroStock_BecomesProducing) {
    EXPECT_CALL(orderRepo, findById("ORD-004")).WillOnce(Return(makeOrder("ORD-004", "S-002", 150)));
    EXPECT_CALL(sampleRepo, findById("S-002")).WillOnce(Return(makeSample("S-002", 0, 0.78)));
    EXPECT_CALL(prodRepo, enqueue(_)).Times(1);
    EXPECT_CALL(orderRepo, updateStatus("ORD-004", OrderStatus::PRODUCING)).Times(1);

    EXPECT_NO_THROW(svc.approveOrder("ORD-004"));
}

// PRD: 실 생산량 = ceil(부족분 / (수율 * 0.9))
TEST_F(OrderServiceTest, ApproveOrder_ActualProductionFormula_Correct) {
    // shortage=170, yield=0.92 → ceil(170 / (0.92*0.9)) = ceil(170/0.828) = ceil(205.3) = 206
    Order order = makeOrder("ORD-005", "S-003", 200);
    Sample sample = makeSample("S-003", 30, 0.92, 0.8);

    EXPECT_CALL(orderRepo, findById("ORD-005")).WillOnce(Return(order));
    EXPECT_CALL(sampleRepo, findById("S-003")).WillOnce(Return(sample));
    EXPECT_CALL(prodRepo, enqueue(testing::Field(&ProductionItem::actualProduction, 206))).Times(1);
    EXPECT_CALL(orderRepo, updateStatus(_, _)).Times(1);

    svc.approveOrder("ORD-005");
}

// ============================================================
// approveOrder — invalid state
// ============================================================

TEST_F(OrderServiceTest, ApproveOrder_AlreadyConfirmed_Throws) {
    EXPECT_CALL(orderRepo, findById("ORD-006"))
        .WillOnce(Return(makeOrder("ORD-006", "S-001", 10, OrderStatus::CONFIRMED)));

    EXPECT_THROW(svc.approveOrder("ORD-006"), std::runtime_error);
}

// ============================================================
// rejectOrder
// ============================================================

TEST_F(OrderServiceTest, RejectOrder_Reserved_BecomesRejected) {
    EXPECT_CALL(orderRepo, findById("ORD-007")).WillOnce(Return(makeOrder("ORD-007", "S-001", 50)));
    EXPECT_CALL(orderRepo, updateStatus("ORD-007", OrderStatus::REJECTED)).Times(1);

    EXPECT_NO_THROW(svc.rejectOrder("ORD-007"));
}

TEST_F(OrderServiceTest, RejectOrder_NotReserved_Throws) {
    EXPECT_CALL(orderRepo, findById("ORD-008"))
        .WillOnce(Return(makeOrder("ORD-008", "S-001", 50, OrderStatus::CONFIRMED)));

    EXPECT_THROW(svc.rejectOrder("ORD-008"), std::runtime_error);
}

// ============================================================
// processShipment
// ============================================================

TEST_F(OrderServiceTest, Shipment_ConfirmedOrder_BecomesReleased) {
    EXPECT_CALL(orderRepo, findById("ORD-009"))
        .WillOnce(Return(makeOrder("ORD-009", "S-001", 50, OrderStatus::CONFIRMED)));
    EXPECT_CALL(orderRepo, updateStatus("ORD-009", OrderStatus::RELEASED)).Times(1);

    EXPECT_NO_THROW(svc.processShipment("ORD-009"));
}

TEST_F(OrderServiceTest, Shipment_NotConfirmed_Throws) {
    EXPECT_CALL(orderRepo, findById("ORD-010"))
        .WillOnce(Return(makeOrder("ORD-010", "S-001", 50, OrderStatus::RESERVED)));

    EXPECT_THROW(svc.processShipment("ORD-010"), std::runtime_error);
}

// ============================================================
// completeProduction
// ============================================================

TEST_F(OrderServiceTest, CompleteProduction_Producing_BecomesConfirmed) {
    EXPECT_CALL(orderRepo, findById("ORD-011"))
        .WillOnce(Return(makeOrder("ORD-011", "S-001", 50, OrderStatus::PRODUCING)));
    EXPECT_CALL(orderRepo, updateStatus("ORD-011", OrderStatus::CONFIRMED)).Times(1);

    EXPECT_NO_THROW(svc.completeProduction("ORD-011"));
}

TEST_F(OrderServiceTest, CompleteProduction_NotProducing_Throws) {
    EXPECT_CALL(orderRepo, findById("ORD-012"))
        .WillOnce(Return(makeOrder("ORD-012", "S-001", 50, OrderStatus::RESERVED)));

    EXPECT_THROW(svc.completeProduction("ORD-012"), std::runtime_error);
}
