// Integration tests for SampleRepository — verifies JSON file persistence.
// Each test uses an isolated temp file and cleans up on teardown.
#include <gtest/gtest.h>
#include "../src/repository/SampleRepository.h"
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

class SampleRepositoryTest : public ::testing::Test {
protected:
    const std::string path = "test_data/samples_test.json";

    void SetUp() override {
        fs::create_directories("test_data");
        fs::remove(path);
    }
    void TearDown() override { fs::remove(path); }
};

// ============================================================
// PRD 검증 시나리오 1: 시료 등록 후 재시작해도 데이터 유지
// ============================================================

TEST_F(SampleRepositoryTest, Save_ThenReload_DataPersists) {
    {
        SampleRepository repo(path);
        repo.save({ "S-001", "실리콘 웨이퍼-8인치", 0.5, 0.92, 480 });
    }
    // Simulate restart: new instance reads from same file
    SampleRepository repo2(path);
    Sample s = repo2.findById("S-001");

    EXPECT_EQ(s.id,   "S-001");
    EXPECT_EQ(s.name, "실리콘 웨이퍼-8인치");
    EXPECT_DOUBLE_EQ(s.avgProductionTime, 0.5);
    EXPECT_DOUBLE_EQ(s.yield,            0.92);
    EXPECT_EQ(s.stock, 480);
}

TEST_F(SampleRepositoryTest, SaveMultiple_FindAll_ReturnsAll) {
    SampleRepository repo(path);
    repo.save({ "S-001", "웨이퍼A", 0.5, 0.92, 100 });
    repo.save({ "S-002", "웨이퍼B", 0.3, 0.78, 200 });
    repo.save({ "S-003", "웨이퍼C", 0.8, 0.92,  30 });

    SampleRepository repo2(path);
    auto all = repo2.findAll();
    EXPECT_EQ(all.size(), 3u);
}

TEST_F(SampleRepositoryTest, Save_UpdateExisting_OverwritesInPlace) {
    SampleRepository repo(path);
    repo.save({ "S-001", "웨이퍼A", 0.5, 0.92, 100 });
    repo.save({ "S-001", "웨이퍼A-수정", 0.6, 0.95, 200 }); // same id → update

    auto all = repo.findAll();
    EXPECT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0].name,  "웨이퍼A-수정");
    EXPECT_EQ(all[0].stock, 200);
}

TEST_F(SampleRepositoryTest, FindById_NotFound_Throws) {
    SampleRepository repo(path);
    EXPECT_THROW(repo.findById("S-999"), std::runtime_error);
}

TEST_F(SampleRepositoryTest, ExistsById_Found_ReturnsTrue) {
    SampleRepository repo(path);
    repo.save({ "S-001", "웨이퍼A", 0.5, 0.92, 100 });
    EXPECT_TRUE(repo.existsById("S-001"));
}

TEST_F(SampleRepositoryTest, ExistsById_NotFound_ReturnsFalse) {
    SampleRepository repo(path);
    EXPECT_FALSE(repo.existsById("S-999"));
}

// ============================================================
// PRD 검증 시나리오 4: 재고 차감 영속성
// ============================================================

TEST_F(SampleRepositoryTest, UpdateStock_Deduction_Persists) {
    SampleRepository repo(path);
    repo.save({ "S-001", "웨이퍼A", 0.5, 0.92, 100 });
    repo.updateStock("S-001", -50);

    SampleRepository repo2(path);
    EXPECT_EQ(repo2.findById("S-001").stock, 50);
}

TEST_F(SampleRepositoryTest, UpdateStock_Addition_Persists) {
    SampleRepository repo(path);
    repo.save({ "S-001", "웨이퍼A", 0.5, 0.92, 30 });
    repo.updateStock("S-001", +100); // production completed

    SampleRepository repo2(path);
    EXPECT_EQ(repo2.findById("S-001").stock, 130);
}

TEST_F(SampleRepositoryTest, UpdateStock_NotFound_Throws) {
    SampleRepository repo(path);
    EXPECT_THROW(repo.updateStock("S-999", -10), std::runtime_error);
}

TEST_F(SampleRepositoryTest, EmptyFile_FindAll_ReturnsEmpty) {
    SampleRepository repo(path);
    EXPECT_TRUE(repo.findAll().empty());
}
