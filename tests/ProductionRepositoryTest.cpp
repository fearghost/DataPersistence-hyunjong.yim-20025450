// Integration tests for ProductionRepository.
// Validates FIFO queue behaviour and persistence (PRD scenario 3).
#include <gtest/gtest.h>
#include "../src/repository/ProductionRepository.h"
#include <filesystem>

namespace fs = std::filesystem;

class ProductionRepositoryTest : public ::testing::Test {
protected:
    const std::string path = "test_data/production_test.json";

    void SetUp() override {
        fs::create_directories("test_data");
        fs::remove(path);
    }
    void TearDown() override { fs::remove(path); }

    ProductionItem makeItem(int queueId, const std::string& orderId,
                            const std::string& sampleId = "S-001",
                            int shortage = 100, int actual = 120, double time = 60.0) {
        return { queueId, orderId, sampleId, shortage, actual, time };
    }
};

// ============================================================
// PRD 검증 시나리오 3: 생산 큐 FIFO 검증
// ============================================================

TEST_F(ProductionRepositoryTest, Enqueue_Dequeue_FIFO_Order) {
    ProductionRepository repo(path);
    repo.enqueue(makeItem(1, "ORD-001"));
    repo.enqueue(makeItem(2, "ORD-002"));
    repo.enqueue(makeItem(3, "ORD-003"));

    auto first = repo.dequeue();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->orderId, "ORD-001"); // first in, first out

    auto second = repo.dequeue();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->orderId, "ORD-002");

    auto third = repo.dequeue();
    ASSERT_TRUE(third.has_value());
    EXPECT_EQ(third->orderId, "ORD-003");

    auto empty = repo.dequeue();
    EXPECT_FALSE(empty.has_value());
}

TEST_F(ProductionRepositoryTest, Queue_Persists_AfterReload) {
    {
        ProductionRepository repo(path);
        repo.enqueue(makeItem(1, "ORD-001"));
        repo.enqueue(makeItem(2, "ORD-002"));
    }
    ProductionRepository repo2(path);
    auto queue = repo2.getQueue();
    ASSERT_EQ(queue.size(), 2u);
    EXPECT_EQ(queue[0].orderId, "ORD-001");
    EXPECT_EQ(queue[1].orderId, "ORD-002");
}

TEST_F(ProductionRepositoryTest, Dequeue_UpdatesPersisted) {
    ProductionRepository repo(path);
    repo.enqueue(makeItem(1, "ORD-001"));
    repo.enqueue(makeItem(2, "ORD-002"));
    repo.dequeue();

    ProductionRepository repo2(path);
    auto queue = repo2.getQueue();
    ASSERT_EQ(queue.size(), 1u);
    EXPECT_EQ(queue[0].orderId, "ORD-002");
}

TEST_F(ProductionRepositoryTest, Dequeue_EmptyQueue_ReturnsNullopt) {
    ProductionRepository repo(path);
    auto result = repo.dequeue();
    EXPECT_FALSE(result.has_value());
}

TEST_F(ProductionRepositoryTest, GetQueue_Empty_ReturnsEmptyVector) {
    ProductionRepository repo(path);
    EXPECT_TRUE(repo.getQueue().empty());
}

// ============================================================
// current item (currently producing)
// ============================================================

TEST_F(ProductionRepositoryTest, SetGetCurrent_ItemPersists) {
    ProductionRepository repo(path);
    repo.setCurrent(makeItem(1, "ORD-001", "S-003", 170, 206, 164.8));

    ProductionRepository repo2(path);
    auto cur = repo2.getCurrent();
    ASSERT_TRUE(cur.has_value());
    EXPECT_EQ(cur->orderId,          "ORD-001");
    EXPECT_EQ(cur->sampleId,         "S-003");
    EXPECT_EQ(cur->shortage,         170);
    EXPECT_EQ(cur->actualProduction, 206);
    EXPECT_DOUBLE_EQ(cur->estimatedTime, 164.8);
}

TEST_F(ProductionRepositoryTest, SetCurrent_Null_ReturnsNullopt) {
    ProductionRepository repo(path);
    repo.setCurrent(makeItem(1, "ORD-001"));
    repo.setCurrent(std::nullopt);

    ProductionRepository repo2(path);
    EXPECT_FALSE(repo2.getCurrent().has_value());
}

TEST_F(ProductionRepositoryTest, GetCurrent_InitiallyEmpty_ReturnsNullopt) {
    ProductionRepository repo(path);
    EXPECT_FALSE(repo.getCurrent().has_value());
}

// ============================================================
// remove
// ============================================================

TEST_F(ProductionRepositoryTest, Remove_ByQueueId_RemovesCorrectItem) {
    ProductionRepository repo(path);
    repo.enqueue(makeItem(1, "ORD-001"));
    repo.enqueue(makeItem(2, "ORD-002"));
    repo.enqueue(makeItem(3, "ORD-003"));
    repo.remove(2);

    auto queue = repo.getQueue();
    ASSERT_EQ(queue.size(), 2u);
    EXPECT_EQ(queue[0].orderId, "ORD-001");
    EXPECT_EQ(queue[1].orderId, "ORD-003");
}

// ============================================================
// production item field validation
// ============================================================

TEST_F(ProductionRepositoryTest, EnqueuedItem_AllFieldsPersist) {
    ProductionItem item{ 1, "ORD-001", "S-003", 170, 206, 164.8 };
    ProductionRepository repo(path);
    repo.enqueue(item);

    ProductionRepository repo2(path);
    auto q = repo2.getQueue();
    ASSERT_EQ(q.size(), 1u);
    EXPECT_EQ(q[0].queueId,          1);
    EXPECT_EQ(q[0].orderId,          "ORD-001");
    EXPECT_EQ(q[0].sampleId,         "S-003");
    EXPECT_EQ(q[0].shortage,         170);
    EXPECT_EQ(q[0].actualProduction, 206);
    EXPECT_DOUBLE_EQ(q[0].estimatedTime, 164.8);
}
