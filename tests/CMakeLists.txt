# Google Test 설정
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ${SRC_DIR} ${HANDLER_DIR})

# 테스트 소스 파일
set(TEST_SOURCES
    test_packet.cpp
    test_maze.cpp
)

# 필요한 소스 파일 추가
set(SOURCES
${SRC_DIR}/game_server_app.cpp
${SRC_DIR}/reactor.cpp
${SRC_DIR}/connection.cpp
${SRC_DIR}/thread_pool.cpp
${SRC_DIR}/connection_manager.cpp
${SRC_DIR}/game_manager.cpp
${SRC_DIR}/room.cpp
${SRC_DIR}/map.cpp
${SRC_DIR}/player.cpp
${SRC_DIR}/utils.cpp
${HANDLER_DIR}/network_event_handler.cpp
${HANDLER_DIR}/game_event_handler.cpp
)

# 테스트 타겟 생성
add_executable(tests ${TEST_SOURCES} ${SOURCES})

# 타겟에 라이브러리 링크
target_link_libraries(tests PRIVATE ${Boost_LIBRARIES} pthread GTest::gtest GTest::gtest_main nlohmann_json::nlohmann_json)

# 테스트 추가
add_test(NAME AllTests COMMAND tests)
