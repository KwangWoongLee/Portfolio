# Portfolio Server Project

포트폴리오 제작을 위한 C++ 게임 서버 프로젝트 저장소입니다.  
학생 시절부터 개발해 온 IOCP 기반 네트워크 코어를 현재 지식 수준에 맞게 리팩터링하고,  
간단한 멀티 서버 구조(월드/상위 서버)와 콘텐츠를 구현하는 것을 목표로 합니다.

---

## 개발 및 테스트 환경

- OS : Windows 10 / 11 (64-bit)
- CPU : Intel Core i5-12400F
- RAM : 32GB
- IDE : Visual Studio 2022 Community
- 언어 : C++23 (`/std:c++latest`)

---

## 구현 계획 및 진행

1. **네트워크 라이브러리**
   - IOCP 기반 비동기 I/O, 세션 관리, 워커 스레드 풀
   - 워크로드 성격별 스레드 풀 분리 (GameLogic / NetworkIO / DB / Timer)

2. **Timer 시스템**
   - min-heap + condition_variable 기반 스케줄러
   - 일회성 / 반복 타이머, TaskDispatcher 연동
   - 콘텐츠 시간 이벤트(공성전 phase 등) 처리용

3. **CMS (기획 데이터 관리)**
   - CSV 파서 + 도메인 ID 타입 안전 처리(`StrongId<Tag, Value>`)

4. **Memory Transaction**
   - Undo Log 기반 원자적 메모리 변경
   - 즉시 Apply, 부분 실패 시 역순 Rollback, 성공 시 DB 반영

5. **Actor 모델 동시성**
   - 액터(`ActorId` 키) 단위 단일 스레드 직렬 처리 (`KeySerialTaskExecutor`)
   - Typed Message 처리

6. **월드 서버**
   - Zone / Instance 구조, Grid 기반 시야 처리
   - `Player` / `PlayerManager`를 액터 모델로 통합 완료
   - `Zone`을 액터화 (`SendToZone` 메시지 디스패치) — `_players`/`_grid` 접근을 zone worker로 단일화하여 race-free
   - 시야 입출입 (W2CActorEnter/Leave) + Welcome 진입 스냅샷

7. **부하 측정 및 시각화 인프라**
   - 비동기 logger thread + 큐로 메트릭 CSV 저장 (worker 차단 없음)
   - `Metrics`: send/recv packet count, byte count, CPU%, queue size
   - 옵저버 채널 + Raylib 기반 자체 시각화 도구 (`Viewer`) — 액터 위치/시야 + 실시간 메트릭
   - Grafana 시계열 대시보드 연동 (CSV datasource)
   - 자세한 측정 결과: [PortfolioServer/measurements.md](PortfolioServer/measurements.md)

8. **공성전 콘텐츠** (예정)

9. **상위 서버군** (예정)
   - World ↔ Universe 서버 간 통신, 이벤트 전파

---

## 측정 narrative

자세한 단계별 측정 기록은 [PortfolioServer/measurements.md](PortfolioServer/measurements.md) 참고.

### 1. 시작점 — 락 없는 zone에서 race condition

초기 zone은 `_players`/`_grid` 자료구조에 락을 두지 않은 채 여러 worker thread에서 접근. 1000봇 부하 시 `unordered_map` iterator invalidation으로 access violation crash.

### 2. Zone 완전 액터화

모든 zone 접근을 `SendToZone(zoneId, msg)` 메시지로 단일화.

- `ZoneMsg::ActorMoved` / `PlayerEntered` / `PlayerLeft` / `BroadcastInSightRequest`
- `KeySerialTaskExecutor`의 `taskKey % threadCount`로 같은 zone은 항상 같은 worker에서 직렬 처리
- `_players`/`_grid` 수정은 zone worker만 → race-free
- 옵저버용 read는 `atomic<shared_ptr<vector<ActorSnapshot> const>>` 캐시로 lock-free (RCU 비슷한 패턴)

### 3. Spawn 차원 측정 (1프로세스 2000봇)

| Spawn 반경 | 끊긴 봇 | 한 봇 시야 안 평균 |
|---|---|---|
| 100 | ~1891 | ~2000 (전체) |
| 200 | ~510 | ~800 |
| 300 | 0 | ~50 |

좁은 spawn = 한 시야에 봇 몰림 = O(N²) broadcast. 한 봇 시야 안 ~800명에서 `SendOverflow` 발생.

### 4. SendOverflow 원인 — TCP backpressure

처음엔 server 한계로 오해. 실제 원인 분석:

```
[데이터 흐름]
Server SendPacket → _sendBuffer (64KB)
    ↓ WSASend
  Kernel send buffer (OS)
    ↓ TCP
  Network (loopback)
    ↓
  Kernel recv buffer (Client OS)
    ↓ WSARecv
  Client _recvBuffer
    ↓
  Client 처리 (NetworkIO worker)
```

**Client 측이 못 따라가면 위로 backpressure 전파**:

1. Client NetworkIO worker가 매초 ~40만 packet 처리 못 함
2. Client `_recvBuffer` 적체 → WSARecv pending
3. Client kernel recv buffer 가득 → TCP가 ACK 안 보냄 (window=0)
4. Server 측: client 못 받는다고 인식, kernel send buffer 가득
5. Server `OnSendCompleted` 늦음 → `_sendBuffer` 안 비워짐
6. 다음 SendPacket 호출 시 `_sendBuffer.GetWritableSize() < contentSize`
7. `Disconnect(EDisconnectReason::SendOverflow)` 호출

즉 server `SendOverflow`는 결과고, **진짜 원인은 client recv 처리 한계**.

### 5. Client 분산이 해결한 이유

| 구성 | 끊긴 봇 | 비고 |
|---|---|---|
| 1프로세스 × 2000봇 | 510 | client 한계 |
| 6프로세스 × 500봇 | 0 (game) | server 여유 |

**왜 분산이 효과적인가**:
- 1프로세스 NetIO=8 → CPU 12% (16 core 중 ~2 core만 사용, 나머지 IO wait)
- 6프로세스 × NetIO=4 = 총 24 worker → CPU 코어 더 분산 활용
- 각 프로세스 독립 메모리/cache → cache miss 감소
- TCP 흐름 정상 → server `_sendBuffer` 안 차게 됨

**시사점**: 부하 테스트 도구는 분산 형태가 정석 (Locust, k6 등). localhost loopback에서는 client/server가 동일 머신 자원 경쟁이라 분산이 필수.

### 6. 진짜 server 한계 — 단일 zone worker

분산으로 client 한계 해소 후 server 한계 측정:

| 봇 수 | sendBps | queueSize | Server CPU |
|---|---|---|---|
| 3000 | 60~73 MB/s | 0~103 | 4~7% |
| 3500 | 80~96 MB/s | 251~1,460 | 5~6% |
| **4000** | 80~87 MB/s | **5천 → 20만** | 6~8% |

**CPU 6%에도 queue 폭발**:
- 모든 봇이 zone 1에 들어감 (한 zone만 활용)
- `KeySerialTaskExecutor`의 `taskKey % threadCount`는 zone 수가 적으면 분산 효과 없음
- 단일 zone worker thread 1개가 4000봇 메시지 전부 처리 → 한 CPU 코어 풀가동
- 다른 worker는 놀고 있음

### 7. 해결 방향 (다음 단계)

- **Zone 분할**: 큰 zone 1개 → 작은 zone N개로 분할, worker N개 활용
- **Tick batching**: 매 이동마다 broadcast X, zone tick(50ms)에 모인 이동들을 `W2CMoveBatch` 1개로 묶어 broadcast. 메시지 수 본질적 감소
- **Shared queue / work stealing 후보 제외**: 같은 key 직렬화 보장 깨져 액터 모델 무너짐 → race 회귀

---

## 향후 확장 아이디어

1. **DB 연동**
   - MSSQL 연동
   - 콘텐츠 상태, 유저 / 길드 정보, 매치 / 랭킹 데이터 저장 및 조회

2. **웹 서버 활용** (고민)

---

## 라이선스 / 참고

- 이 프로젝트는 **개인 포트폴리오 및 학습 목적**의 프로젝트입니다.
- IOCP 네트워크 코어는 학생 시절부터 작성해 온 개인 코드를  
  현재 지식 수준으로 리팩터링한 것이며,  
  **현재 재직 중인 회사의 소스 코드는 포함하지 않습니다.**
