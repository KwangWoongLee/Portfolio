# C++ MMORPG Live Server Portfolio

대형 온라인 RPG 서버 직무에서 요구하는 C++ 비동기 네트워크, 라이브 콘텐츠, DB 정합성, 운영 대응 역량을 보여주기 위한 포트폴리오입니다.

핵심 목표는 단순 샘플 서버가 아니라, **라이브 MMORPG 서버에서 자주 마주치는 네트워크, 비동기 처리, 월드/존 동시성, 콘텐츠 상태 관리, 운영 지표 분석**을 코드와 문서로 설명할 수 있게 만드는 것입니다.

현재 구현은 IOCP 기반 네트워크 코어, Player/Zone/World actor 실행 모델, 길드와 공성전 lifecycle, 공통 상태 머신 유틸, 부하 테스트 및 메트릭 분석에 집중되어 있습니다. 이후 보상/상품 흐름, MySQL 스키마와 라이브 이슈 대응 문서를 확장할 예정입니다.

---

## 라이브 MMORPG 서버 역량 매핑

라이브 MMORPG 서버 직무에서 자주 요구되는 항목을 아래 구현 범위와 연결합니다.

| 핵심 역량 | 현재 포트폴리오 대응 |
|---|---|
| C/C++, 자료구조, 알고리즘 | IOCP 서버 코어, LinearBuffer, Stream, ObjectPool, Grid 기반 시야 처리 |
| 비동기/네트워크/소켓 프로그래밍 | IOCP, AcceptEx/WSARecv/WSASend, completion worker pool, session lifecycle |
| 라이브 서비스 콘텐츠 개발 | World/Zone, Player, 길드 가입·탈퇴·위임, 공성전 선포와 진행 상태 머신 |
| MySQL/SQL 이해 | 2~3주차에 MySQL schema와 보상/공성전 저장 흐름 추가 예정 |
| 디버깅과 이슈 추적 | race condition, SendOverflow, backpressure 측정 narrative 문서화 |
| 최적화/리팩토링 | Zone actor화, single zone bottleneck 분석, tick batching 설계 |
| 운영툴/각종 툴 개발 | Observer channel, metrics logger, Raylib Viewer(optional), Grafana CSV 연동 |

---

## 현재 구현 범위

### 1. IOCP 네트워크 코어

- IOCP 기반 비동기 I/O
- Listener / Connector / IOCPSession 구조
- Accept, Connect, Recv, Send, Disconnect completion 처리
- IOCP completion worker pool
- Packet receive task와 game logic task 분리
- send/recv buffer overflow 감지 및 disconnect reason 기록

관련 코드:

- `PortfolioServer/Core/include/IOCP.h`
- `PortfolioServer/Core/src/IOCP.cpp`
- `PortfolioServer/Core/include/IOCPSession.h`
- `PortfolioServer/Core/src/IOCPSession.cpp`
- `PortfolioServer/Core/include/LinearBuffer.h`
- `PortfolioServer/Core/include/Stream.h`

### 2. TaskDispatcher와 Actor 실행 모델

- 작업 성격별 executor 구성: `GameLogic`, `NetworkIO`, `DB`, `Timer`
- `DbDispatcher`가 DB command를 DB executor로 넘기고, 완료 콜백은 actor post 또는 error tracking만 수행
- `KeySerialTaskExecutor`로 같은 key의 작업을 같은 worker에서 직렬 처리
- Player, Zone, World와 진행 중인 SiegeWar를 key 기반 메시지 처리 흐름으로 분리
- Manager는 registry/lifecycle을 lock으로 관리하고, 내부 객체 변경은 `ManagedActorTaskAccess` 기반 Runner로만 실행
- 공유 상태 접근을 특정 worker로 모아 race condition을 줄이는 구조

관련 코드:

- `PortfolioServer/Core/include/TaskDispatcher.h`
- `PortfolioServer/Core/src/TaskDispatcher.cpp`
- `PortfolioServer/Core/include/KeySerialTaskExecutor.h`
- `PortfolioServer/Common/include/ManagedActorTaskAccess.h`
- `PortfolioServer/Common/include/ActorTask.h`
- `PortfolioServer/Server/WorldServer/include/ZoneTask.h`
- `PortfolioServer/Server/WorldServer/include/WorldTask.h`
- `PortfolioServer/Server/WorldServer/include/WorldActorRegistry.h`

### 3. World / Zone 서버 구조

- FieldZone / InstanceZone 구분
- Grid 기반 시야 처리
- player enter/leave, move, attack, HP change, death event 처리
- Welcome snapshot과 actor enter/leave packet
- Zone 내부 상태는 zone worker에서만 수정
- Observer read와 player count 조회는 snapshot/atomic cache로 분리
- ZoneManager의 zone/instance registry는 `shared_mutex`로 보호
- 인스턴스 제거는 manager lock 밖에서 후보를 모은 뒤 zone worker에서 empty 여부를 재확인
- 외부에는 raw zone lookup을 공개하지 않고 `PostToZone`/`SendToZone`/snapshot API만 사용
- zone 입장은 `Open` 상태에서 발급한 enter permit이 있을 때만 진행하고, 실제 enter 성공 후 player current zone을 확정
- WorldActor는 WorldId key로 실행되며 GuildManager를 멤버로 소유
- GuildManager는 길드 registry/index와 내부 SiegeManager를 함께 조정하고 snapshot 복사 조회만 제공
- Guild는 자체 lock으로 길드장·길드원 상태를 보호하고, 진행 중인 SiegeWar는 별도 actor task에서 상태를 변경

관련 코드:

- `PortfolioServer/Server/WorldServer/include/IZone.h`
- `PortfolioServer/Server/WorldServer/src/IZone.cpp`
- `PortfolioServer/Server/WorldServer/include/ZoneManager.h`
- `PortfolioServer/Server/WorldServer/src/ZoneManager.cpp`
- `PortfolioServer/Server/WorldServer/include/PlayerTaskRunner.h`
- `PortfolioServer/Server/WorldServer/include/ZoneTaskRunner.h`
- `PortfolioServer/Server/WorldServer/include/Grid.h`

### 4. CMS 성격의 기획 데이터 로딩

- CSV parser 기반 데이터 로딩
- `StrongId<Tag, Value>`로 도메인 ID 타입 분리
- 공성전 데이터와 스케줄 데이터 샘플 로딩

관련 코드:

- `PortfolioServer/Common/include/CmsManager.h`
- `PortfolioServer/Common/src/CmsManager.cpp`
- `PortfolioServer/Common/data/SiegeWar.csv`
- `PortfolioServer/Common/data/SiegeSchedule.csv`

### 5. 길드와 공성전 lifecycle

- 길드 생성 시 생성자를 길드장으로 등록하고 가입·탈퇴·길드장 위임을 지원
- 공성전 참여 길드는 `InProgress` 동안 신규 가입과 해산을 제한
- 길드장 개인 골드는 Player actor에서 `MemoryTransaction`으로 차감하고, DB command dispatch 이후의 SP 실패는 별도 error tracking으로 분리
- 선포는 WorldActor의 예약, Player actor의 결제, WorldActor의 확정 순서로 처리
- SiegeWar actor는 의미 있는 상태 변경 시 revision snapshot을 WorldActor로 전달
- 종료 snapshot 반영 후 참가 제한을 해제하고 DB·보상 작업 생성을 위한 TODO 지점을 유지

관련 코드:

- `PortfolioServer/Server/WorldServer/include/Guild/GuildManager.h`
- `PortfolioServer/Server/WorldServer/include/Siege/SiegeManager.h`
- `PortfolioServer/Server/WorldServer/include/Siege/SiegeWarTaskRunner.h`

### 6. 공통 상태 머신 유틸

- 현재 상태, 허용 전이, guard, `onEnter`/`onTick`/`onExit` 처리
- 선형 phase 흐름은 `AllowSequentialTransitions`, 예외 전이는 `AllowTransition`으로 정의
- 자체 thread/lock 없이 actor 또는 콘텐츠 객체가 has-a로 소유
- Tick에서 상태별 진행 로직과 전환 조건을 처리하고, 전이 결과는 호출자가 후속 처리
- 공성전, 라이브 이벤트 단계 흐름에 재사용 예정

관련 코드:

- `PortfolioServer/Common/include/StateMachine.h`

### 7. 부하 테스트와 관찰성

- TestClient 부하 봇
- Observer channel로 actor snapshot과 metrics 전송
- CSV metrics logging
- Grafana CSV datasource 연동
- SendOverflow, pending send bytes, packet count, byte count, queue size 기록

자세한 측정 기록:

- `PortfolioServer/measurements.md`
- `PortfolioServer/docs/live-issue-case.md`

---

## 대표 분석 사례

### Zone race condition

초기 구조에서는 여러 worker thread가 Zone의 `_players`, `_grid`를 직접 접근하면서 1000봇 부하 시 `unordered_map` iterator invalidation crash가 발생했습니다.

해결 방향:

- 모든 Zone 접근을 `SendToZone(zoneId, msg)` 메시지로 단일화
- 같은 zone key는 항상 같은 worker에서 처리
- `_players`, `_grid` 수정은 zone worker에서만 수행
- Observer read는 atomic snapshot cache로 분리
- Zone registry/lifecycle은 `ZoneManager`에서 짧은 lock으로 보호

### SendOverflow와 TCP backpressure

부하 테스트 중 server `SendOverflow`가 발생했지만, 분석 결과 직접 원인은 server send logic보다 client receive 처리 한계였습니다.

분석 흐름:

- client NetworkIO worker가 packet 처리량을 따라가지 못함
- client recv buffer와 kernel recv buffer가 밀림
- TCP window가 줄어 server kernel send buffer가 밀림
- server `OnSendCompleted`가 늦어져 `_sendBuffer`가 비워지지 않음
- 다음 `SendPacket`에서 `SendOverflow` 발생

이 과정은 `measurements.md`에 측정 조건별로 정리했습니다.

---

## 라이브 서버 완성도를 높이기 위한 추가 작업

1개월 완성 목표로 아래 기능을 추가합니다.

### 1. 공성전 서버 상태 머신

- Scheduled / Prepare / InProgress / Finished / Canceled
- 스케줄 로딩
- GM 명령 기반 강제 시작/종료
- 참여자/진영/길드 등록
- 점령 점수, 킬/데스, 기여도 누적
- 종료 시 랭킹 계산

문서:

- `PortfolioServer/docs/siege-system.md`

### 2. 보상/상품/DB 흐름

- 공성전 종료 보상 생성
- reward claim 중복 수령 방지
- transaction / rollback / idempotency 흐름
- MySQL schema와 repository interface

문서:

- `PortfolioServer/docs/db-and-reward.md`

### 3. 라이브 이슈 대응 문서

- 공성전 종료 중 서버 재시작
- 보상 중복 요청
- 대규모 전투 중 broadcast 폭증
- packet validation, rate limit, disconnect reason 분석

문서:

- `PortfolioServer/docs/live-issue-case.md`

---

## 빌드 구성

개발 환경:

- Windows 10 / 11 x64
- Visual Studio 2022
- C++23 (`/std:c++latest`)
- Release x64 기준 측정

필수 빌드 대상:

- `Core`
- `Common`
- `WorldServer`
- `TestClient`

선택 빌드 대상:

- `Viewer`
  - Raylib 기반 관찰용 클라이언트입니다.
  - `raylib.h`와 `raylib.lib`가 필요하므로 기본 솔루션 빌드 대상에서는 제외합니다.

---

## 문서

- `PortfolioServer/docs/architecture.md`
- `PortfolioServer/docs/siege-system.md`
- `PortfolioServer/docs/db-and-reward.md`
- `PortfolioServer/docs/live-issue-case.md`
- `PortfolioServer/measurements.md`

---

## 참고

이 프로젝트는 개인 포트폴리오 및 학습 목적의 프로젝트입니다.

IOCP 네트워크 코어는 학생 시절부터 작성해 온 개인 코드를 현재 지식 수준에 맞게 리팩터링한 것이며, 재직 중인 회사의 소스 코드나 내부 데이터를 포함하지 않습니다.
