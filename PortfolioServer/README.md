# PortfolioServer

MMORPG 라이브 서버에서 자주 마주치는 문제를 개인 프로젝트로 재구성한 C++ 서버 포트폴리오입니다.

## 다루는 범위

- Windows IOCP 기반 비동기 TCP 서버
- IO completion / packet / game logic / DB / timer worker 분리
- Player / Zone / World / SiegeWar 단위 키 직렬 actor 실행
- Zone 내부 공유 상태를 worker 단일 소유로 제한
- MySQL 캐릭터 생성 / 골드 저장 / 공성 보상 claim 영속화
- 공성전 상태 머신, 선포 잠금, 결제·환불 흐름
- pendingSendBytes, queueSize, CPU, packet rate 측정
- SendOverflow / Zone 큐 병목 관찰과 개선 후보 정리

## 프로젝트 구성

```text
Core                 IOCP, session, stream/packet, task dispatcher, timer, metrics
Common               packet schema, actor base, state machine, CMS csv loader
Database             MySQL connection pool, character repository, schema/procedure
Server/WorldServer   Player, Zone, Guild, SiegeWar, Reward, observer metrics
Client/TestClient    부하 테스트용 봇 클라이언트
Client/Viewer        raylib 기반 observer viewer
Tests/UnitTests      StateMachine, SiegeWar, SiegeRewardPlanner, GuildManagerSiegeLock 단위 테스트
output/pdf           제출용 이력서·경력기술서·포트폴리오 PDF
```

## 실행 환경

- Windows 10 / 11
- Visual Studio 2022, MSVC v143
- MySQL Server 8.x, MySQL Connector/C++ 9.x
- raylib (Viewer 빌드 시)
- GoogleTest NuGet package (UnitTests 빌드 시)

## 세팅

MySQL Connector/C++ 루트를 환경 변수로 지정합니다.

```powershell
setx MYSQL_CONCPP_ROOT "C:\path\to\mysql-connector-c++"
```

MySQL 초기화:

```text
Database/sql/001_schema.sql
Database/sql/002_procedures.sql
Database/sql/003_siege_reward_claim.sql
```

WorldServer 환경 변수:

```powershell
$env:PORTFOLIO_DB_ENABLED = '1'
$env:PORTFOLIO_DB_HOST = '127.0.0.1'
$env:PORTFOLIO_DB_PORT = '3306'
$env:PORTFOLIO_DB_USER = 'portfolio_server'
$env:PORTFOLIO_DB_PASSWORD = 'local-password'
$env:PORTFOLIO_DB_SCHEMA = 'portfolio'
$env:PORTFOLIO_DB_CONNECTIONS = '2'
```

공성전 데모 로그를 보려면 아래 옵션을 켭니다.

```powershell
$env:PORTFOLIO_SIEGE_DEMO = '1'
```

## 실행 순서

1. `WorldServer` Release x64 실행
2. `TestClient` 여러 프로세스 실행 (기본 500 bot/프로세스)
3. 선택: `Viewer` 실행해 observer snapshot 관찰
4. `metrics/server_metrics.csv`, `logs/server_YYYYMMDD.log` 확인

## 부하 측정 요약

- 단일 TestClient 프로세스에서 2000 bot을 몰면 client recv/TCP backpressure로 SendOverflow 발생
- 프로세스 분산 후 3000 bot까지 안정
- 4000 bot에서 CPU가 낮아도 단일 zone worker queue 폭발 → 병목이 zone actor lane에 있음
- 대응 방향: zone 분할과 tick batching

## 참고

개인 포트폴리오·학습 목적 프로젝트입니다. IOCP 네트워크 코어는 학생 시절부터 작성해 온 개인 코드를 현재 지식 수준에 맞게 리팩터링한 것이며, 재직 중인 회사의 소스나 내부 데이터를 포함하지 않습니다.
