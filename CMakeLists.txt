cmake_minimum_required(VERSION 3.10)
project(asio_server)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)

# Boost 라이브러리 사용 설정
find_package(Boost REQUIRED COMPONENTS system)

include_directories(${Boost_INCLUDE_DIRS} src)

# nlohmann/json 추가
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2 # 원하는 버전 지정
)
FetchContent_MakeAvailable(json)

# 소스 파일
set(SOURCES
    src/main.cpp
    # 통신 처리
    src/reactor.cpp
    src/connection.cpp
    src/thread_pool.cpp
    # 게임 처리
    src/server.cpp
    src/room.cpp
    src/map.cpp
    src/player.cpp
)

# 타겟 생성
add_executable(asio_server ${SOURCES})

# Boost, pthread, nlohmann/json 링크
target_link_libraries(asio_server PRIVATE ${Boost_LIBRARIES} pthread nlohmann_json::nlohmann_json)
