# C++ MMORPG Live Server Portfolio

대형 온라인 RPG 서버 직무에서 요구하는 C++ 비동기 네트워크, actor 기반 상태 소유권, 라이브 콘텐츠 lifecycle, MySQL 정합성, 운영 지표 분석 역량을 보여주기 위한 포트폴리오입니다.

목표는 완성된 게임 클라이언트가 아니라, 라이브 MMORPG 서버에서 자주 마주치는 네트워크 backpressure, 월드/존 동시성, 장시간 이벤트 상태 전이, DB 경계, 보상 중복 방지 설계를 코드와 문서로 설명할 수 있게 만드는 것입니다.

## 현재 구현 요약

- Windows IOCP 기반 비동기 TCP 서버
- `GameLogic`, `NetworkIO`, `DB`, `Timer` executor 분리
- `Player`, `Zone`, `World`, `SiegeWar` 단위 key-serial actor 실행
- Zone 내부 공유 상태를 worker 단일 소유로 제한해 race condition 완화
- Observer snapshot, metrics CSV, disconnect reason, pending send bytes 측정
- MySQL Connector/C++ 기반 캐릭터 생성 및 골드 저장 경계
- `DbCompletionTarget` 기반 DB 완료 라우팅과 reward claim idempotency
- 길드 생성, 가입, 탈퇴, 길드장 이전
- 공성전 스케줄, 상태 머신, 예약 wake-up, 점령 grace, 선포 비용 결제/환불
- 공성전 종료 후 `SiegeRewardPlanner` 기반 reward job/claim 생성 로그
- 공성 보상 claim 생성 영속화를 위한 `siege_reward_claim` repository 및 SQL schema
- GoogleTest 단위 테스트: `StateMachine`, `SiegeWar`, `SiegeRewardPlanner`

## 라이브 서버 역량 매핑

| 핵심 역량 | 현재 포트폴리오 대응 |
|---|---|
| C/C++, 자료구조, 알고리즘 | IOCP 서버 코어, LinearBuffer, Stream, ObjectPool, Grid 기반 시야 처리 |
| 비동기/네트워크/소켓 프로그래밍 | IOCP, AcceptEx/WSARecv/WSASend, completion worker pool, session lifecycle |
| 동시성 설계 | key-serial executor, actor-owned state, manager registry lock 경계 |
| 라이브 콘텐츠 개발 | 길드 lifecycle, 공성전 선포/진행/종료, 상태 머신과 timer wake-up |
| MySQL/SQL 이해 | character/gold repository, SP, 공성 reward claim repository/schema |
| 장애/중복 방어 | DB transaction/idempotency 경계, reward claim unique key 설계, 재처리 문서 |
| 디버깅/병목 분석 | Zone race condition, SendOverflow, TCP backpressure, 단일 zone 병목 측정 |
| 운영 관찰성 | Observer channel, metrics logger, CSV/Grafana 연동 준비 |

## 구현 범위

### IOCP 네트워크 코어

- IOCP 기반 비동기 I/O
- Listener / Connector / IOCPSession 구조
- Accept, Connect, Recv, Send, Disconnect completion 처리
- Packet receive task와 game logic task 분리
- send/recv buffer overflow 감지 및 disconnect reason 기록

대표 코드:

- `PortfolioServer/Core/include/IOCP.h`
- `PortfolioServer/Core/src/IOCPSession.cpp`
- `PortfolioServer/Core/include/LinearBuffer.h`
- `PortfolioServer/Core/include/Stream.h`

### Actor 실행 모델

이 프로젝트의 Actor 모델은 라이브 서비스에서 겪은 Player 내부 모델 동시 접근, race condition, lock order/deadlock 문제를 설계 배경으로 삼았습니다. 현재 구현에서는 NetworkIO, Timer, DB 완료 경로가 Player 상태를 직접 수정하지 않고, PlayerActor key lane에 메시지를 모아 상태 변경 순서를 고정합니다.
- 같은 key의 작업을 같은 worker에서 직렬 처리
- Player / Zone / World / SiegeWar를 메시지 기반 task로 분리
- Manager는 registry/lifecycle만 lock으로 보호
- 내부 actor 상태 변경은 Runner를 통해서만 수행
- DB callback은 `DbCompletionTarget` 기반 post-only 경계로 actor/session executor에 넘김




대표 코드:

- `PortfolioServer/Core/include/TaskDispatcher.h`
- `PortfolioServer/Core/include/KeySerialTaskExecutor.h`
- `PortfolioServer/Common/include/ManagedActorTaskAccess.h`
- `PortfolioServer/Server/WorldServer/include/PlayerTaskRunner.h`
- `PortfolioServer/Server/WorldServer/include/ZoneTaskRunner.h`
- `PortfolioServer/Server/WorldServer/include/WorldTaskRunner.h`

### World / Zone

- FieldZone / InstanceZone 구분
- Grid 기반 시야 처리
- player enter/leave, move, attack, HP change, death/respawn event 처리
- Observer read는 atomic snapshot cache로 분리
- Zone enter permit과 `Open/Closing` lifecycle로 입장/제거 경합 완화
- 단일 zone worker 병목을 metrics로 확인하고, 다음 개선 방향을 zone 분할과 tick batching으로 정리

### 길드와 공성전

- 길드 생성, 가입, 탈퇴, 길드장 이전
- 공성 선포는 WorldActor 예약, Player actor 개인 골드 결제, WorldActor 확정 순서로 처리
- 선포 예약 성공 시 참가 길드를 lock하고, 결제 실패/timeout/공성 종료 시 unlock
- 공성전은 `Scheduled -> Prepare -> InProgress -> Finished` 상태 머신과 `Canceled` 예외 전이를 사용
- `AttackWindow`와 `OccupationGrace`로 점령 흐름 처리
- 종료 snapshot 반영 후 reward job/claim을 런타임에 생성

대표 코드:

- `PortfolioServer/Server/WorldServer/include/Guild/GuildManager.h`
- `PortfolioServer/Server/WorldServer/include/Siege/SiegeWar.h`
- `PortfolioServer/Server/WorldServer/src/Siege/SiegeManager.cpp`
- `PortfolioServer/Server/WorldServer/src/Reward/SiegeRewardPlanner.cpp`

## DB 초기화

MySQL에서 아래 SQL을 순서대로 실행합니다.

```text
PortfolioServer/Database/sql/001_schema.sql
PortfolioServer/Database/sql/002_procedures.sql
PortfolioServer/Database/sql/003_siege_reward_claim.sql
```

현재 코드에 실제로 연결된 영속화 범위는 캐릭터 생성/조회, 골드 저장, 공성 보상 claim 생성 저장입니다. 실제 claim 수령 request와 pending/failed 재처리 정책은 다음 단계로 남겨두고, schema와 재처리 설계는 문서화했습니다.

## 대표 분석 사례

### Zone race condition

초기 구조에서는 여러 worker thread가 Zone의 `_actors`, `_grid`를 직접 접근하면서 1000봇 이상 부하에서 iterator invalidation crash가 발생했습니다. 이후 모든 Zone 접근을 `SendToZone(zoneId, msg)`와 `ZoneTaskRunner`로 정리해 zone worker가 authoritative state를 단일 소유하도록 바꿨습니다.

### SendOverflow와 TCP backpressure

부하 테스트 중 server `SendOverflow`가 발생했지만, 측정 결과 직접 원인은 server send logic보다 client receive 처리량과 TCP backpressure였습니다. client process 분산 후 3000 bot까지 안정화되었고, 4000 bot에서는 CPU가 낮아도 단일 zone worker queue가 폭발해 진짜 server 병목이 드러났습니다.

자세한 측정 narrative는 `PortfolioServer/measurements.md`에 정리되어 있습니다.

## 다음 주 마감 기준

다음 주까지는 기능을 넓히기보다 제출 가능한 상태로 닫습니다.

1. README와 설계 문서 최신화
2. 공성 reward claim 수령 request와 재처리 범위 결정
3. 공성 demo log와 부하 측정 PDF에 넣을 Grafana/metrics 캡처 준비
4. 최종 TODO와 면접 설명 포인트 정리

세부 마감 계획은 `PortfolioServer/docs/finalization-plan.md`를 봅니다.

## 문서

- `PortfolioServer/README.md`
- `PortfolioServer/docs/architecture.md`
- `PortfolioServer/docs/siege-system.md`
- `PortfolioServer/docs/db-and-reward.md`
- `PortfolioServer/docs/live-issue-case.md`
- `PortfolioServer/docs/finalization-plan.md`
- `PortfolioServer/measurements.md`

## 참고

이 프로젝트는 개인 포트폴리오 및 학습 목적의 프로젝트입니다. IOCP 네트워크 코어는 학생 시절부터 작성해 온 개인 코드를 현재 지식 수준에 맞게 리팩터링한 것이며, 재직 중인 회사의 소스 코드나 내부 데이터를 포함하지 않습니다.
