#!/bin/bash

# 빌드 스크립트

set -ex  # 실패 시 명령 출력 및 종료
trap 'echo "Error on line $LINENO"' ERR  # 에러 발생 시 라인 출력

BUILD_TYPE="Release"  # 기본 빌드 타입은 Release

# 스크립트의 위치를 기준으로 프로젝트 루트 설정
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(dirname "$SCRIPT_DIR")"  # scripts/의 상위 디렉토리인 프로젝트 루트

BUILD_DIR="${SOURCE_DIR}/build"  # 프로젝트 루트의 build 디렉토리

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
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# 2. CMake 빌드 설정
echo "CMake 빌드 설정 중... (빌드 타입: $BUILD_TYPE)"
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE "$SOURCE_DIR"

# 3. Make 빌드 실행
echo "빌드 중..."
make -j$(nproc)  # 병렬 빌드

echo "빌드 완료! 실행 파일은 build/ 디렉토리에 생성되었습니다."
