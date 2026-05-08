#include <iostream>
#include <iomanip>
#include <string>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <stdexcept>
#include "repository/SampleRepository.h"
#include "repository/OrderRepository.h"
#include "repository/ProductionRepository.h"
#include "service/OrderService.h"

namespace fs = std::filesystem;

// ---- utilities -------------------------------------------------------------

static void initDataDir() { fs::create_directories("data"); }

static std::string nowTimestamp() {
    auto t  = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
    localtime_s(&tm, &t);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return buf;
}

static std::string generateOrderId() {
    static int seq = 1;
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
    localtime_s(&tm, &t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "ORD-%Y%m%d-", &tm);
    return std::string(buf) + std::to_string(1000 + seq++).substr(1);
}

static int    inputInt(const std::string& p) { std::cout << p; int v; std::cin >> v; std::cin.ignore(1024,'\n'); return v; }
static double inputDbl(const std::string& p) { std::cout << p; double v; std::cin >> v; std::cin.ignore(1024,'\n'); return v; }
static std::string inputStr(const std::string& p) { std::cout << p; std::string s; std::getline(std::cin, s); return s; }

static void sep() { std::cout << "--------------------------------------------\n"; }
static void hdr(const std::string& t) {
    std::cout << "\n============================================\n"
              << "  " << t << "\n"
              << "============================================\n";
}

static const char* stockLabel(int stock) {
    if (stock == 0)   return "[고갈]";
    if (stock < 100)  return "[부족]";
    return "[여유]";
}

// ---- 1. 시료 관리 -----------------------------------------------------------

void menuSample(ISampleRepository& repo) {
    while (true) {
        hdr("시료 관리");
        std::cout << " [1] 시료 등록\n [2] 시료 목록\n [3] 시료 검색(ID)\n [4] 재고 현황\n [0] 돌아가기\n";
        sep();
        switch (inputInt("선택: ")) {
        case 0: return;

        case 1: {
            hdr("시료 등록");
            Sample s;
            s.id                = inputStr("시료 ID (예: S-001): ");
            s.name              = inputStr("시료명: ");
            s.avgProductionTime = inputDbl("평균 생산시간 (min/ea): ");
            s.yield             = inputDbl("수율 (0.0~1.0): ");
            s.stock             = inputInt("초기 재고 (ea): ");
            repo.save(s);
            std::cout << ">> 등록 완료: [" << s.id << "] " << s.name << "\n";
            break;
        }

        case 2: {
            hdr("시료 목록");
            auto all = repo.findAll();
            if (all.empty()) { std::cout << "  등록된 시료 없음\n"; break; }
            std::cout << std::left
                      << std::setw(8)  << "ID"
                      << std::setw(26) << "시료명"
                      << std::setw(8)  << "재고"
                      << std::setw(8)  << "수율"
                      << "생산시간  상태\n";
            sep();
            for (auto& s : all)
                std::cout << std::setw(8)  << s.id
                          << std::setw(26) << s.name
                          << std::setw(8)  << s.stock
                          << std::setw(8)  << s.yield
                          << std::setw(10) << s.avgProductionTime
                          << stockLabel(s.stock) << "\n";
            break;
        }

        case 3: {
            hdr("시료 검색");
            std::string id = inputStr("시료 ID: ");
            try {
                Sample s = repo.findById(id);
                std::cout << "  ID         : " << s.id   << "\n"
                          << "  시료명     : " << s.name << "\n"
                          << "  재고       : " << s.stock << " ea  " << stockLabel(s.stock) << "\n"
                          << "  수율       : " << s.yield << "\n"
                          << "  평균생산시간: " << s.avgProductionTime << " min/ea\n";
            } catch (...) {
                std::cout << ">> 시료를 찾을 수 없습니다: " << id << "\n";
            }
            break;
        }

        case 4: {
            hdr("재고 현황");
            auto all = repo.findAll();
            if (all.empty()) { std::cout << "  시료 없음\n"; break; }
            sep();
            for (auto& s : all)
                std::cout << std::setw(8)  << s.id
                          << std::setw(26) << s.name
                          << "재고: " << std::setw(6) << s.stock
                          << "  " << stockLabel(s.stock) << "\n";
            break;
        }

        default: std::cout << "잘못된 입력\n";
        }
    }
}

// ---- 2. 주문 관리 -----------------------------------------------------------

void menuOrder(ISampleRepository& sampleRepo, IOrderRepository& orderRepo,
               IProductionRepository& prodRepo) {
    OrderService svc(sampleRepo, orderRepo, prodRepo);
    while (true) {
        hdr("주문 관리");
        std::cout << " [1] 주문 접수\n [2] 주문 목록\n [3] 주문 승인\n [4] 주문 거절\n [0] 돌아가기\n";
        sep();
        switch (inputInt("선택: ")) {
        case 0: return;

        case 1: {
            hdr("주문 접수");
            std::string sid      = inputStr("시료 ID: ");
            std::string customer = inputStr("고객명: ");
            int qty              = inputInt("수량 (ea): ");
            std::string oid      = generateOrderId();
            try {
                svc.placeOrder(sid, customer, qty, oid, nowTimestamp());
                std::cout << ">> 주문 접수 완료. 주문번호: " << oid << "\n";
            } catch (const std::exception& e) {
                std::cout << ">> 오류: " << e.what() << "\n";
            }
            break;
        }

        case 2: {
            hdr("주문 목록");
            auto all = orderRepo.findAll();
            if (all.empty()) { std::cout << "  주문 없음\n"; break; }
            std::cout << std::left
                      << std::setw(18) << "주문번호"
                      << std::setw(8)  << "시료ID"
                      << std::setw(20) << "고객명"
                      << std::setw(6)  << "수량"
                      << "상태\n";
            sep();
            for (auto& o : all)
                std::cout << std::setw(18) << o.orderId
                          << std::setw(8)  << o.sampleId
                          << std::setw(20) << o.customerName
                          << std::setw(6)  << o.quantity
                          << orderStatusToString(o.status) << "\n";
            break;
        }

        case 3: {
            hdr("주문 승인");
            std::string oid = inputStr("승인할 주문번호: ");
            try {
                svc.approveOrder(oid);
                Order o = orderRepo.findById(oid);
                std::cout << ">> 승인 완료. 최종 상태: " << orderStatusToString(o.status) << "\n";
                if (o.status == OrderStatus::PRODUCING)
                    std::cout << "   * 재고 부족 - 생산 큐에 등록되었습니다.\n";
            } catch (const std::exception& e) {
                std::cout << ">> 오류: " << e.what() << "\n";
            }
            break;
        }

        case 4: {
            hdr("주문 거절");
            std::string oid = inputStr("거절할 주문번호: ");
            try {
                svc.rejectOrder(oid);
                std::cout << ">> 거절 처리 완료: " << oid << "\n";
            } catch (const std::exception& e) {
                std::cout << ">> 오류: " << e.what() << "\n";
            }
            break;
        }

        default: std::cout << "잘못된 입력\n";
        }
    }
}

// ---- 3. 생산 / 출고 ---------------------------------------------------------

void menuProduction(ISampleRepository& sampleRepo, IOrderRepository& orderRepo,
                    IProductionRepository& prodRepo) {
    OrderService svc(sampleRepo, orderRepo, prodRepo);
    while (true) {
        hdr("생산 / 출고");
        std::cout << " [1] 출고 처리  (CONFIRMED -> RELEASED)\n"
                  << " [2] 생산 완료  (PRODUCING -> CONFIRMED + 재고 증가 + 큐 제거)\n"
                  << " [3] 생산 큐 조회\n"
                  << " [0] 돌아가기\n";
        sep();
        switch (inputInt("선택: ")) {
        case 0: return;

        case 1: {
            hdr("출고 처리");
            std::string oid = inputStr("출고할 주문번호: ");
            try {
                svc.processShipment(oid);
                std::cout << ">> 출고 완료: " << oid << "  [RELEASED]\n";
            } catch (const std::exception& e) {
                std::cout << ">> 오류: " << e.what() << "\n";
            }
            break;
        }

        case 2: {
            hdr("생산 완료 처리");
            std::string oid = inputStr("완료 처리할 주문번호: ");
            try {
                // PRODUCING -> CONFIRMED
                svc.completeProduction(oid);
                // 재고 증가 + 큐에서 제거
                auto queue = prodRepo.getQueue();
                for (auto& item : queue) {
                    if (item.orderId == oid) {
                        sampleRepo.updateStock(item.sampleId, item.actualProduction);
                        prodRepo.remove(item.queueId);
                        std::cout << ">> 생산 완료: " << oid
                                  << "  재고 +" << item.actualProduction
                                  << " (" << item.sampleId << ")\n";
                        break;
                    }
                }
            } catch (const std::exception& e) {
                std::cout << ">> 오류: " << e.what() << "\n";
            }
            break;
        }

        case 3: {
            hdr("생산 큐");
            auto queue = prodRepo.getQueue();
            if (queue.empty()) { std::cout << "  대기 항목 없음\n"; break; }
            std::cout << std::left
                      << std::setw(5)  << "순번"
                      << std::setw(18) << "주문번호"
                      << std::setw(8)  << "시료ID"
                      << std::setw(8)  << "부족"
                      << std::setw(10) << "실생산량"
                      << "예상시간(min)\n";
            sep();
            for (auto& p : queue)
                std::cout << std::setw(5)  << p.queueId
                          << std::setw(18) << p.orderId
                          << std::setw(8)  << p.sampleId
                          << std::setw(8)  << p.shortage
                          << std::setw(10) << p.actualProduction
                          << p.estimatedTime << "\n";
            break;
        }

        default: std::cout << "잘못된 입력\n";
        }
    }
}

// ---- main ------------------------------------------------------------------

int main() {
    initDataDir();

    SampleRepository     sampleRepo("data/samples.json");
    OrderRepository      orderRepo("data/orders.json");
    ProductionRepository prodRepo("data/production.json");

    while (true) {
        hdr("반도체 시료 생산주문관리  [DataPersistence PoC]");
        std::cout << " [1] 시료 관리\n [2] 주문 관리\n [3] 생산 / 출고\n [0] 종료\n";
        sep();
        switch (inputInt("선택: ")) {
        case 0:
            std::cout << "종료합니다.\n";
            return 0;
        case 1: menuSample(sampleRepo);                         break;
        case 2: menuOrder(sampleRepo, orderRepo, prodRepo);     break;
        case 3: menuProduction(sampleRepo, orderRepo, prodRepo); break;
        default: std::cout << "잘못된 입력\n";
        }
    }
}
