// Integration tests for OrderRepository.
#include <gtest/gtest.h>
#include "../src/repository/OrderRepository.h"
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

class OrderRepositoryTest : public ::testing::Test {
protected:
    const std::string path = "test_data/orders_test.json";

    void SetUp() override {
        fs::create_directories("test_data");
        fs::remove(path);
    }
    void TearDown() override { fs::remove(path); }

    Order makeOrder(const std::string& id, const std::string& sid,
                    int qty, OrderStatus st = OrderStatus::RESERVED) {
        return { id, sid, "TestCo", qty, st, "2026-05-08T00:00:00" };
    }
};

// ============================================================
// PRD 검증 시나리오 2: 주문 상태 변경 후 재시작해도 상태 유지
// ============================================================

TEST_F(OrderRepositoryTest, Save_ThenReload_DataPersists) {
    {
        OrderRepository repo(path);
        repo.save(makeOrder("ORD-001", "S-001", 200));
    }
    OrderRepository repo2(path);
    Order o = repo2.findById("ORD-001");

    EXPECT_EQ(o.orderId,      "ORD-001");
    EXPECT_EQ(o.sampleId,     "S-001");
    EXPECT_EQ(o.customerName, "TestCo");
    EXPECT_EQ(o.quantity,     200);
    EXPECT_EQ(o.status,       OrderStatus::RESERVED);
}

TEST_F(OrderRepositoryTest, UpdateStatus_Persists_AfterReload) {
    OrderRepository repo(path);
    repo.save(makeOrder("ORD-001", "S-001", 100));
    repo.updateStatus("ORD-001", OrderStatus::CONFIRMED);

    OrderRepository repo2(path);
    EXPECT_EQ(repo2.findById("ORD-001").status, OrderStatus::CONFIRMED);
}

TEST_F(OrderRepositoryTest, UpdateStatus_Producing_Persists) {
    OrderRepository repo(path);
    repo.save(makeOrder("ORD-002", "S-002", 300));
    repo.updateStatus("ORD-002", OrderStatus::PRODUCING);

    OrderRepository repo2(path);
    EXPECT_EQ(repo2.findById("ORD-002").status, OrderStatus::PRODUCING);
}

TEST_F(OrderRepositoryTest, UpdateStatus_Released_Persists) {
    OrderRepository repo(path);
    repo.save(makeOrder("ORD-003", "S-001", 50, OrderStatus::CONFIRMED));
    repo.updateStatus("ORD-003", OrderStatus::RELEASED);

    OrderRepository repo2(path);
    EXPECT_EQ(repo2.findById("ORD-003").status, OrderStatus::RELEASED);
}

TEST_F(OrderRepositoryTest, UpdateStatus_Rejected_Persists) {
    OrderRepository repo(path);
    repo.save(makeOrder("ORD-004", "S-001", 50));
    repo.updateStatus("ORD-004", OrderStatus::REJECTED);

    OrderRepository repo2(path);
    EXPECT_EQ(repo2.findById("ORD-004").status, OrderStatus::REJECTED);
}

TEST_F(OrderRepositoryTest, FindByStatus_FiltersCorrectly) {
    OrderRepository repo(path);
    repo.save(makeOrder("ORD-001", "S-001", 100, OrderStatus::RESERVED));
    repo.save(makeOrder("ORD-002", "S-001", 200, OrderStatus::CONFIRMED));
    repo.save(makeOrder("ORD-003", "S-002", 300, OrderStatus::RESERVED));

    auto reserved = repo.findByStatus(OrderStatus::RESERVED);
    EXPECT_EQ(reserved.size(), 2u);

    auto confirmed = repo.findByStatus(OrderStatus::CONFIRMED);
    EXPECT_EQ(confirmed.size(), 1u);
    EXPECT_EQ(confirmed[0].orderId, "ORD-002");
}

// PRD: REJECTED는 모니터링에서 제외 — findByStatus로 걸러낼 수 있어야 함
TEST_F(OrderRepositoryTest, FindByStatus_ExcludesRejected) {
    OrderRepository repo(path);
    repo.save(makeOrder("ORD-001", "S-001", 100, OrderStatus::REJECTED));
    repo.save(makeOrder("ORD-002", "S-001", 200, OrderStatus::RESERVED));

    auto reserved  = repo.findByStatus(OrderStatus::RESERVED);
    auto rejected  = repo.findByStatus(OrderStatus::REJECTED);

    EXPECT_EQ(reserved.size(), 1u);
    EXPECT_EQ(rejected.size(), 1u);
}

TEST_F(OrderRepositoryTest, FindById_NotFound_Throws) {
    OrderRepository repo(path);
    EXPECT_THROW(repo.findById("ORD-999"), std::runtime_error);
}

TEST_F(OrderRepositoryTest, SaveMultiple_FindAll_ReturnsAll) {
    OrderRepository repo(path);
    repo.save(makeOrder("ORD-001", "S-001", 100));
    repo.save(makeOrder("ORD-002", "S-002", 200));

    EXPECT_EQ(repo.findAll().size(), 2u);
}

// ============================================================
// PRD 검증 시나리오 5: sampleId 참조 정합성 확인용 보조 테스트
// ============================================================

TEST_F(OrderRepositoryTest, SampleIdReference_IsPreserved) {
    OrderRepository repo(path);
    repo.save(makeOrder("ORD-001", "S-003", 200));

    OrderRepository repo2(path);
    EXPECT_EQ(repo2.findById("ORD-001").sampleId, "S-003");
}
