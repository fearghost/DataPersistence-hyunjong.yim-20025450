# PRD: DataPersistence PoC

## 개요

반도체 시료 생산주문관리 시스템([CRA_AI] 과제)의 PoC 중 **데이터 영속성 처리** 항목에 해당하는 PoC이다.

> **데이터 영속성(Data Persistence)**: application을 다시 실행해도 데이터를 유지할 수 있는 성질

본 PoC의 목적은 본 프로젝트에서 사용할 데이터 저장·불러오기 구조를 검증하는 것이다.  
구현 언어는 **C++**, 데이터 포맷은 **JSON 파일**을 사용한다.

---

## 기술 스택

| 항목 | 내용 |
|------|------|
| 언어 | C++ (C++17 이상) |
| 데이터 포맷 | JSON |
| 저장 방식 | 로컬 파일 시스템 (`.json` 파일) |
| JSON 파서 참고 | [fearghost/PoC_JsonParser](https://github.com/fearghost/PoC_JsonParser) |

> **JSON 파서 활용 안내**  
> JSON 직렬화/역직렬화 구현 시 [https://github.com/fearghost/PoC_JsonParser](https://github.com/fearghost/PoC_JsonParser) 레포지터리의 구현체를 참고하거나 직접 활용할 수 있다.  
> 해당 레포지터리는 C++ 기반의 경량 JSON 파서 PoC로, 본 프로젝트의 파일 기반 데이터 영속성 구현에 적합하다.

---

## 도메인 모델

반도체 시료 생산주문관리 시스템에서 영속화가 필요한 핵심 엔티티는 다음과 같다.

### Sample (시료)

시스템의 기본 단위. 시스템에 등록된 시료만 주문 가능.

```json
{
  "id": "S-001",
  "name": "실리콘 웨이퍼-8인치",
  "avgProductionTime": 0.5,
  "yield": 0.92,
  "stock": 480
}
```

| 필드 | 타입 | 설명 |
|------|------|------|
| id | string | 시료 고유 ID (예: S-001) |
| name | string | 시료명 |
| avgProductionTime | double | 평균 생산시간 (min/ea) |
| yield | double | 수율 (정상 시료 / 총 생산 시료, 0~1) |
| stock | int | 현재 재고 수량 (ea) |

### Order (주문)

고객이 시료를 요청하면 생성되는 주문 단위.

```json
{
  "orderId": "ORD-20260416-0043",
  "sampleId": "S-003",
  "customerName": "삼성전자 파운드리",
  "quantity": 200,
  "status": "RESERVED",
  "createdAt": "2026-04-16T09:32:15"
}
```

| 필드 | 타입 | 설명 |
|------|------|------|
| orderId | string | 주문 고유 번호 |
| sampleId | string | 주문 시료 ID |
| customerName | string | 고객명 |
| quantity | int | 주문 수량 |
| status | string | 주문 상태 (아래 상태 참고) |
| createdAt | string | 주문 생성 일시 (ISO 8601) |

#### 주문 상태 정의

| 상태 | 의미 |
|------|------|
| RESERVED | 주문 접수 (초기 상태) |
| REJECTED | 주문 거절 (모니터링 제외) |
| PRODUCING | 승인 완료 + 재고 부족으로 생산 중 |
| CONFIRMED | 승인 완료 + 출고 대기 중 |
| RELEASED | 출고 완료 |

### ProductionQueue (생산 큐)

재고 부족 시 생산 라인에 등록되는 대기 항목. FIFO 스케줄링.

```json
{
  "queueId": 1,
  "orderId": "ORD-20260416-0043",
  "sampleId": "S-003",
  "shortage": 170,
  "actualProduction": 206,
  "estimatedTime": 165
}
```

| 필드 | 타입 | 설명 |
|------|------|------|
| queueId | int | 큐 순서 번호 |
| orderId | string | 연결된 주문 ID |
| sampleId | string | 생산할 시료 ID |
| shortage | int | 부족분 수량 |
| actualProduction | int | 실 생산량 = ceil(부족분 / (수율 × 0.9)) |
| estimatedTime | double | 총 생산 시간 = 평균 생산시간 × 실 생산량 |

---

## JSON 파일 구조

모든 데이터는 `data/` 디렉터리 하위의 JSON 파일로 관리한다.

```
data/
├── samples.json       # 시료 목록
├── orders.json        # 주문 목록
└── production.json    # 생산 큐
```

### samples.json 예시

```json
{
  "samples": [
    {
      "id": "S-001",
      "name": "실리콘 웨이퍼-8인치",
      "avgProductionTime": 0.5,
      "yield": 0.92,
      "stock": 480
    },
    {
      "id": "S-002",
      "name": "GaN 에피택셜-4인치",
      "avgProductionTime": 0.3,
      "yield": 0.78,
      "stock": 220
    }
  ]
}
```

### orders.json 예시

```json
{
  "orders": [
    {
      "orderId": "ORD-20260416-0041",
      "sampleId": "S-005",
      "customerName": "LG이노텍",
      "quantity": 300,
      "status": "RESERVED",
      "createdAt": "2026-04-16T09:10:00"
    }
  ]
}
```

### production.json 예시

```json
{
  "queue": [
    {
      "queueId": 1,
      "orderId": "ORD-20260416-0040",
      "sampleId": "S-005",
      "shortage": 150,
      "actualProduction": 190,
      "estimatedTime": 114.0
    }
  ],
  "currentOrderId": "ORD-20260416-0038"
}
```

---

## 기능 명세 (CRUD)

### 1. Create — 데이터 생성

| 기능 | 설명 |
|------|------|
| 시료 등록 | 새로운 시료를 `samples.json`에 추가 |
| 주문 접수 | 신규 주문을 `orders.json`에 RESERVED 상태로 추가 |
| 생산 큐 등록 | 재고 부족 시 `production.json` 큐에 항목 추가 |

### 2. Read — 데이터 조회

| 기능 | 설명 |
|------|------|
| 시료 목록 조회 | `samples.json` 전체 로드 후 목록 출력 |
| 시료 검색 | ID 또는 이름으로 특정 시료 검색 |
| 주문 목록 조회 | 상태별 필터링하여 주문 목록 출력 |
| 생산 큐 조회 | 현재 생산 중 및 대기 중인 큐 목록 조회 |
| 재고 현황 조회 | 시료별 재고 수량 및 상태(여유/부족/고갈) 조회 |

### 3. Update — 데이터 수정

| 기능 | 설명 |
|------|------|
| 주문 상태 변경 | RESERVED → CONFIRMED / PRODUCING / REJECTED |
| 주문 출고 처리 | CONFIRMED → RELEASED 상태 전환 |
| 생산 완료 처리 | PRODUCING → CONFIRMED 상태 전환, 재고 업데이트 |
| 재고 차감 | 출고 시 해당 시료의 stock 감소 |
| 재고 증가 | 생산 완료 시 해당 시료의 stock 증가 |

### 4. Delete — 데이터 삭제

| 기능 | 설명 |
|------|------|
| 생산 큐 항목 제거 | 생산 완료된 큐 항목을 `production.json`에서 제거 |

---

## 비즈니스 로직

### 주문 승인 처리

```
승인 요청
  ├── 재고 >= 주문 수량  →  상태: CONFIRMED, 재고 차감
  └── 재고 < 주문 수량   →  상태: PRODUCING, 생산 큐 등록
                              실 생산량 = ceil(부족분 / (수율 × 0.9))
                              총 생산시간 = 평균 생산시간 × 실 생산량
```

### 재고 상태 판정

| 상태 | 조건 |
|------|------|
| 고갈 | stock == 0 |
| 부족 | 0 < stock < 활성 주문 요구량 |
| 여유 | stock >= 활성 주문 요구량 |

---

## C++ 클래스 구조 (권장)

```
src/
├── model/
│   ├── Sample.h / Sample.cpp         # 시료 엔티티
│   ├── Order.h / Order.cpp           # 주문 엔티티
│   └── ProductionQueue.h / .cpp      # 생산 큐 엔티티
├── repository/
│   ├── SampleRepository.h / .cpp     # 시료 CRUD (JSON 파일 I/O)
│   ├── OrderRepository.h / .cpp      # 주문 CRUD (JSON 파일 I/O)
│   └── ProductionRepository.h / .cpp # 생산 큐 CRUD (JSON 파일 I/O)
├── json/
│   └── JsonParser.h / JsonParser.cpp # JSON 파서 (PoC_JsonParser 활용)
└── main.cpp                          # 진입점 및 테스트 시나리오
```

### Repository 인터페이스 예시

각 Repository는 다음 메서드를 구현한다:

```cpp
// SampleRepository
void save(const Sample& sample);           // Create / Update
Sample findById(const std::string& id);    // Read
std::vector<Sample> findAll();             // Read all
void remove(const std::string& id);        // Delete

// OrderRepository
void save(const Order& order);
Order findById(const std::string& orderId);
std::vector<Order> findByStatus(const std::string& status);
std::vector<Order> findAll();
```

---

## 검증 시나리오

PoC 완료 기준으로 다음 시나리오를 모두 통과해야 한다.

1. **시료 등록 후 재시작**: 시료를 등록하고 프로그램 재시작 후 동일 시료가 조회되는지 확인
2. **주문 상태 변경 후 재시작**: 주문 상태 변경 후 재시작해도 변경된 상태가 유지되는지 확인
3. **생산 큐 FIFO 검증**: 여러 건 등록 후 FIFO 순서로 처리되는지 확인
4. **재고 차감 영속성**: 출고 처리 후 재고 감소가 파일에 반영되는지 확인
5. **복수 파일 정합성**: samples.json ↔ orders.json 간 sampleId 참조 정합성 유지 확인

---

## 참고 링크

- JSON 파서 구현 참고: [https://github.com/fearghost/PoC_JsonParser](https://github.com/fearghost/PoC_JsonParser)
- 원본 과제 문서: `[CRA_AI]반도체 시료 생산주문관리 프로젝트.pdf`
