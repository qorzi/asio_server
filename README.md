# 비동기 이벤트 기반 서버 프로젝트
- boost.asio를 사용하여, 리액터 패턴 형태의 비동기 이벤트 기반 서버를 구축한다.

## 개발 환경
- Ubutu 22.04
### 개발 환경 구성
```bash
$ ./scripts/setup.sh
```
### 빌드 명령어
```bash
# 디버거 모드
$ ./scripts/build.sh -d

# 운영 모드
$ ./scripts/build.sh
```
### 테스트 명령어
```bash
# gtest app (빌드 후, 사용)
$ ./build/tests/tests
```
### 서버 실행 및 테스트 앱 실행 명령어
```bash
# 서버 실행
$ ./build/asio_server

# 클라이언트 실행
$ python3 ./client_test/client_test.py
```
## 폴더 구조
```plain
asio_server/
├── CMakeLists.txt         # CMake 빌드 설정 파일
├── src/
│   ├── main.cpp
│   ├── game_server_app.hpp
│   ├── game_server_app.cpp
│   ├── reactor.hpp
│   ├── reactor.cpp
│   ├── connection.hpp
│   ├── connection.cpp
│   ├── connection_manager.hpp  # connection과 player 관계 저장 (1대1)
│   ├── connection_manager.cpp
│   ├── thread_pool.hpp
│   ├── thread_pool.cpp
│   ├── header.hpp         # Header 헤더 (통신에 사용될 헤더)
│   ├── event.hpp
│   ├── game_manager.hpp
│   ├── game_manager.cpp
│   ├── game_result.hpp
│   ├── game_result.cpp
│   ├── room.hpp
│   ├── room.cpp
│   ├── map.hpp
│   ├── map.cpp
│   ├── player.hpp
│   ├── player.cpp
│   ├── point.hpp
│   ├── utils.hpp
│   ├── utils.cpp
│   └── handler/           # 핸들러 폴더 (이벤트 디스패치 용도)
│       ├── network_event_handler.hpp
│       ├── network_event_handler.cpp
│       ├── game_event_handler.hpp
│       └── game_event_handler.cpp
├── scripts/               # 스크립트 폴더
│   ├── setup.sh           # 개발 환경 세팅 스크립트
│   └── build.sh           # 빌드 스크립트
├── build/                 # 빌드 아티팩트 생성 폴더 (빌드 시 자동 생성)
├── tests/                 # 테스트 앱 폴더
│   ├── CMakeLists.txt     # 테스트 앱 전용 빌드 설정
│   ├── test_maze.cpp      # 미로 생성 테스트
│   └── test_packet.cpp    # 패킷 파싱 테스트
└── client_test/           # 클라이언트 접속 및 플레이 테스트
```

## 게임 시나리오
### 게임 개요
- 게임은 포탈로 인해 직렬로 연결된 3개의 맵(A-B-C)에서 진행된다.
- 각 맵은 10x10 크기로 구성된다.
### 맵 구조
- 각 맵은 다른 하나의 맵과 포탈로 이어져 있다.
- 시작 맵(A)에는 시작 위치가, 끝 맵(C)에는 도착 위치가 존재한다.'
- 플레이어는 시작 위치에서 출발하여 도착 위치까지 이동해야 한다.
### 게임 진행 방식
- 플레이어는 시작 위치에서 출발하여 포탈을 찾아 다음 맵으로 이동한다.
- 포탈의 정확한 위치는 플레이어가 알 수 없으며, 포탈이 위치한 공간에 도달해야 다음 맵으로 이동할 수 있다.
- 최종적으로 도착 위치에 도달하면 게임이 종료된다.
### 플레이 방식
- 플레이 방식은 아래 두 가지 중 하나로 설정 가능하다.
    - 자유 이동 방식:
        - 턴 제한 없이 플레이어는 원하는 만큼 자유롭게 이동할 수 있다.
    - 제한 이동 방식: (미지원)
        - 플레이어는 한 턴에 한 칸씩만 이동할 수 있다.
        - 모든 플레이어가 턴마다 순차적으로 이동을 선택하고, 모든 플레이어가 선택하면 이동 후, 다음 턴으로 넘어간다.
### 승리 조건
- 도착 위치에 먼저 도달한 플레이어가 높은 순위를 차지한다.
- 플레이어의 순위는 도착 위치에 도달한 순서대로 결정된다.
### 추가 조건
- 플레이어는 각 맵을 순서대로(A -> B -> C) 탐색하며 포탈을 찾아 이동해야 한다.
- 포탈을 찾지 못한 경우 다음 맵으로 이동할 수 없다.

## 서버 시나리오
### 게임 참가
- 서버에 연결된 플레이어가 게임 참가를 요청하면, 플레이어는 대기열에 등록된다.
    - 플레이어는 대기를 중단할 수 있다.
- 플레이어의 수가 2명 이상이라면, 즉시 방을 생성한다.
    - 플레이어가 2명 미만이라면, 방은 생성되지 않는다.
- 방 생성 후, 대기화면에서 카운트다운 5초 이후 게임이 시작된다.
- 게임 시작 후, 플레이어는 이동할 수 있다.