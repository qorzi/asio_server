#!/bin/bash

# 개발 환경 세팅 스크립트

set -e  # 스크립트 실패 시 중단

# 1. Boost 설치
echo "Boost 설치 확인 중..."
if ! dpkg -l | grep -q libboost-all-dev; then
    echo "Boost 라이브러리를 설치합니다."
    sudo apt update
    sudo apt install -y libboost-all-dev
else
    echo "Boost 라이브러리가 이미 설치되어 있습니다."
fi

# 2. CMake 설치 확인
echo "CMake 설치 확인 중..."
if ! command -v cmake &> /dev/null; then
    echo "CMake가 설치되지 않았습니다. 설치를 진행합니다."
    sudo apt update
    sudo apt install -y cmake
else
    echo "CMake가 이미 설치되어 있습니다."
fi

# 3. Google Test 설치
echo "Google Test 설치 확인 중..."
if ! dpkg -l | grep -q libgtest-dev; then
    echo "Google Test를 설치합니다."
    sudo apt update
    sudo apt install -y libgtest-dev
else
    echo "Google Test가 이미 설치되어 있습니다."
fi

# 4. 프로젝트 디렉토리 정리
echo "빌드 폴더를 정리합니다..."
rm -rf build
mkdir build

echo "개발 환경 준비가 완료되었습니다!"
