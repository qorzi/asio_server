#!/bin/bash

# 빌드 스크립트

set -e  # 스크립트 실패 시 중단

BUILD_DIR="build"
BUILD_TYPE="Release"  # 기본 빌드 타입은 Release

# 빌드 타입 설정
for arg in "$@"; do
    if [ "$arg" == "-d" ] || [ "$arg" == "--debug" ]; then
        BUILD_TYPE="Debug"
        break
    fi
done

# 1. 빌드 디렉토리 생성 및 이동
if [ ! -d "$BUILD_DIR" ]; then
    echo "빌드 디렉토리가 없습니다. 생성 중..."
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# 2. CMake 빌드 설정
echo "CMake 빌드 설정 중... (빌드 타입: $BUILD_TYPE)"
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

# 3. Make 빌드 실행
echo "빌드 중..."
make

echo "빌드 완료! 실행 파일은 build/ 디렉토리에 생성되었습니다."
