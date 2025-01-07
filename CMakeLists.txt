cmake_minimum_required(VERSION 3.10)
project(asio_server)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)

# Boost 라이브러리 사용 설정
find_package(Boost REQUIRED COMPONENTS system)

# nlohmann/json 추가
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2 # 원하는 버전 지정
)
FetchContent_MakeAvailable(json)

# 소스 경로 설정
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HANDLER_DIR ${SRC_DIR}/handler)

# 메인 소스 파일
set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/game_server_app.cpp
    ${SRC_DIR}/reactor.cpp
    ${SRC_DIR}/connection.cpp
    ${SRC_DIR}/thread_pool.cpp
    ${SRC_DIR}/connection_manager.cpp
    ${SRC_DIR}/game_manager.cpp
    ${SRC_DIR}/room.cpp
    ${SRC_DIR}/map.cpp
    ${SRC_DIR}/player.cpp
    ${HANDLER_DIR}/network_event_handler.cpp
    ${HANDLER_DIR}/game_event_handler.cpp
)

# 타겟 생성
add_executable(asio_server ${SOURCES})

# Boost, pthread, nlohmann/json 링크
target_link_libraries(asio_server PRIVATE ${Boost_LIBRARIES} pthread nlohmann_json::nlohmann_json)

# 테스트 추가
enable_testing()
add_subdirectory(tests)
