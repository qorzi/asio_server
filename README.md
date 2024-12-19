# 비동기 이벤트 기반 서버 프로젝트

## 폴더 구조
```plain
asio_server/
├── CMakeLists.txt         # CMake 빌드 설정 파일
├── src/                   # 소스 코드 폴더
│   ├── main.cpp           # 엔트리 포인트
│   ├── reactor.cpp        # Reactor 구현
│   ├── connection.cpp     # Connection 구현
│   ├── thread_pool.cpp    # ThreadPool 구현
│   ├── parser.cpp         # Parser 구현
├── include/               # 헤더 파일 폴더
│   ├── reactor.hpp        # Reactor 헤더
│   ├── connection.hpp     # Connection 헤더
│   ├── thread_pool.hpp    # ThreadPool 헤더
│   ├── parser.hpp         # Parser 헤더
├── scripts/               # 스크립트 폴더
│   ├── setup.sh           # 개발 환경 세팅 스크립트
│   └── build.sh           # 빌드 스크립트
├── tests/                 # 테스트 코드 폴더
│   └── server_test.cpp    # 서버 테스트
└── build/                 # 빌드 아티팩트 생성 폴더 (빌드 시 자동 생성)
```
