# PortfolioServer

MMORPG C++ 게임 서버 개발자 포트폴리오입니다.

목표는 완성된 게임 클라이언트가 아니라, 라이브 MMORPG 서버에서 중요한 비동기 네트워크, actor 기반 상태 소유권, Zone 병목 분석, DB 정합성, 장시간 이벤트 상태 머신을 코드와 측정 기록으로 설명하는 것입니다.

## 핵심 포인트

- Windows IOCP 기반 비동기 TCP 서버
- IO completion, packet 처리, game logic, DB, timer worker 분리
- Player / Zone / World / SiegeWar 단위 key-serial actor 실행
- Zone 내부 공유 상태를 worker 단일 소유로 제한해 race condition 완화
- pending send bytes, queue size, disconnect reason, CPU, packet/byte rate 측정
- MySQL Connector/C++ 기반 캐릭터 생성 및 골드 저장 경계
- 공성전 상태 머신, 스케줄, 점령 grace, 선포 시점 참가 길드 lock, 선포 비용 결제/환불 흐름

## 프로젝트 구성

```text
Core                 IOCP, session, stream/packet, task dispatcher, timer, metrics
Common               packet schema, actor base, state machine, CMS csv loader
Database             MySQL connection pool, character repository, schema/procedure
WorldServer/Reward   siege reward planner and claim repository integration
Server/WorldServer   Player, Zone, Guild, SiegeWar, observer metrics
Client/TestClient    부하 테스트용 봇 클라이언트
Client/Viewer        raylib 기반 observer viewer
Tests/UnitTests      StateMachine, SiegeWar, SiegeRewardPlanner 단위 테스트
docs                 아키텍처, 라이브 이슈, DB/보상 설계 문서
```

## 실행 환경

- Windows 10/11
- Visual Studio 2022 이상, MSVC v143
- MySQL Server
- MySQL Connector/C++ 9.x
- raylib, Viewer 빌드 시 필요
- GoogleTest NuGet package, UnitTests 빌드 시 필요

MySQL은 현재 WorldServer 실행 경로에서 필수입니다. DB 없이 동작하는 memory repository 모드는 포트폴리오 범위에 포함하지 않습니다.

## 의존성 설정

MySQL Connector/C++ 설치 루트를 환경 변수로 설정합니다. zip 배포판을 사용하는 경우 `include`, `include/jdbc`, `lib64` 또는 `lib64/vs14`가 같은 루트 아래에 있어야 합니다.

```powershell
setx MYSQL_CONCPP_ROOT "C:\path\to\mysql-connector-c++"
```

Visual Studio 또는 PowerShell은 환경 변수 설정 후 다시 시작합니다.

WorldServer는 빌드 후 Connector DLL을 실행 폴더로 복사합니다.

```text
%MYSQL_CONCPP_ROOT%\lib64\mysqlcppconn-10-vs14.dll
%MYSQL_CONCPP_ROOT%\lib64\libcrypto-3-x64.dll
%MYSQL_CONCPP_ROOT%\lib64\libssl-3-x64.dll
```

자세한 설치 메모는 [docs/database-and-tests-setup.md](docs/database-and-tests-setup.md)를 봅니다.

## DB 초기화

MySQL에서 아래 SQL을 순서대로 실행합니다.

```text
Database/sql/001_schema.sql
Database/sql/002_procedures.sql
Database/sql/003_siege_reward_claim.sql
```

WorldServer 실행 전 환경 변수:

```powershell
$env:PORTFOLIO_DB_ENABLED = '1'
$env:PORTFOLIO_DB_HOST = '127.0.0.1'
$env:PORTFOLIO_DB_PORT = '3306'
$env:PORTFOLIO_DB_USER = 'portfolio_server'
$env:PORTFOLIO_DB_PASSWORD = 'local-password'
$env:PORTFOLIO_DB_SCHEMA = 'portfolio'
$env:PORTFOLIO_DB_CONNECTIONS = '2'
```

공성전 진행을 실제 클라이언트 없이 로그로 확인하려면 아래 옵션을 추가합니다.

```powershell
$env:PORTFOLIO_SIEGE_DEMO = '1'
```

이 옵션은 WorldServer 시작 후 짧은 데모 공성전을 예약하고, 상태 전환과 점령 결과를 `[SiegeWarSnapshot]`, `[SiegeDemo]` 로그로 출력합니다.

## 대표 실행 시나리오

1. `WorldServer`를 Release x64로 실행합니다.
2. `TestClient`를 여러 프로세스로 실행합니다. 기본값은 프로세스당 500 bot입니다.
3. 선택적으로 `Viewer`를 실행해 observer snapshot을 봅니다.
4. 선택적으로 `PORTFOLIO_SIEGE_DEMO=1`을 켜고 서버 로그에서 공성전 상태 전이를 확인합니다.
5. `metrics/server_metrics.csv`와 `logs/server_YYYYMMDD.log`를 확인합니다.

측정 narrative는 [measurements.md](measurements.md)에 정리되어 있습니다.

## 부하 측정 요약

- 1개 TestClient 프로세스에서 2000 bot을 몰면 client recv 처리량과 TCP backpressure 때문에 SendOverflow가 발생했습니다.
- TestClient를 여러 프로세스로 분산하면 3000 bot까지 game disconnect 없이 안정화되었습니다.
- 4000 bot에서는 CPU 전체 사용률이 낮아도 queue가 폭발했습니다.
- 원인은 모든 bot이 하나의 zone에 몰려 단일 zone worker가 병목이 된 것입니다.
- 다음 개선 방향은 zone 분할과 tick batching입니다.

## 구현 범위

구현 완료:

- IOCP session lifecycle, stream framing, packet dispatch
- key-serial task executor와 task type 분리
- Player / Zone actor 메시지화
- Zone enter permit, Open/Closing lifecycle
- observer snapshot cache와 metrics CSV
- MySQL character load/create, gold update repository
- DbCompletionTarget 기반 DB 완료 라우팅
- 공성전 상태 머신과 wake-up scheduling
- 클라이언트 없이 관찰 가능한 공성전 데모 로그
- 공성전 종료 후 winner reward job/claim 생성 로그
- 공성 reward claim SQL schema와 repository 저장 연결
- 길드 생성/가입/탈퇴/장 이전, 선포 공성 참가 lock
- 공성 선포 비용 결제 요청, timeout, 환불 메시지 흐름

다음 구현 단계:

- reward claim 실제 수령 request와 idempotent response
- 공성 보상 claim DB 저장
- reward claim insert/claim SP와 idempotent response
- 장애 후 미완료 reward job 재처리

이 범위는 [docs/db-and-reward.md](docs/db-and-reward.md)와 [docs/live-issue-case.md](docs/live-issue-case.md)에 별도로 정리했습니다.

## 면접 설명 포인트

- 왜 Zone 자료구조에 lock을 덧붙이지 않고 actor 소유권 모델로 정리했는가
- IOCP completion thread가 game logic을 직접 처리하지 않게 나눈 이유
- SendOverflow를 server buffer 문제가 아니라 TCP backpressure 관점에서 확인한 과정
- 단일 zone worker 병목을 queue size와 CPU 사용률로 분리해낸 방법
- actor memory 반영과 DB transaction/completion 경계를 분리한 이유
- 공성전 state와 보상 지급 state를 같은 lifecycle에 넣지 않는 이유

## 관련 문서

- [docs/architecture.md](docs/architecture.md)
- [docs/live-issue-case.md](docs/live-issue-case.md)
- [docs/siege-system.md](docs/siege-system.md)
- [docs/db-and-reward.md](docs/db-and-reward.md)
- [docs/database-and-tests-setup.md](docs/database-and-tests-setup.md)
- [docs/finalization-plan.md](docs/finalization-plan.md)
- [measurements.md](measurements.md)
