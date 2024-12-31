# Google Test 설정
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ../src)

# 테스트 소스 파일
set(TEST_SOURCES
    test_room.cpp
)

# 필요한 소스 파일 추가
set(SOURCES
    ../src/server.cpp
    ../src/room.cpp
    ../src/player.cpp
    ../src/map.cpp
    ../src/thread_pool.cpp
    ../src/connection.cpp
    ../src/reactor.cpp
    ../src/connection_manager.cpp
)

# 테스트 타겟 생성
add_executable(tests ${TEST_SOURCES} ${SOURCES})

# 타겟에 라이브러리 링크
target_link_libraries(tests PRIVATE ${Boost_LIBRARIES} pthread GTest::gtest GTest::gtest_main nlohmann_json::nlohmann_json)

# 테스트 추가
add_test(NAME AllTests COMMAND tests)
