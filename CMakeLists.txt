cmake_minimum_required(VERSION 3.10)
project(asio_server)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)

# Boost 라이브러리 사용 설정
find_package(Boost REQUIRED COMPONENTS system)

include_directories(${Boost_INCLUDE_DIRS} include)

# 소스 파일
set(SOURCES
    src/main.cpp
    src/reactor.cpp
    src/connection.cpp
    src/thread_pool.cpp
    src/parser.cpp
)

# 타겟 생성
add_executable(asio_server ${SOURCES})

# Boost, pthead 라이브러리 링크
target_link_libraries(asio_server ${Boost_LIBRARIES} pthread)
