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

7. **공성전 콘텐츠** (예정)

8. **상위 서버군** (예정)
   - World ↔ Universe 서버 간 통신, 이벤트 전파

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
