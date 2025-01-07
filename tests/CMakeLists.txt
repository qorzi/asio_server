# Google Test 설정
find_package(GTest REQUIRED)

# 테스트 전용 소스 파일
set(TEST_SOURCES
    # ${CMAKE_CURRENT_SOURCE_DIR}/test_room.cpp
)

# 상위 디렉터리에서 SOURCES 가져오기
set(SOURCES ${SOURCES} PARENT_SCOPE)

# 테스트 타겟 생성
add_executable(tests ${TEST_SOURCES} ${SOURCES})

# 타겟에 라이브러리 링크
target_link_libraries(tests PRIVATE ${Boost_LIBRARIES} pthread GTest::gtest GTest::gtest_main nlohmann_json::nlohmann_json)

# 테스트 등록
add_test(NAME AllTests COMMAND tests)
