cmake_minimum_required(VERSION 3.11)
project(gateway LANGUAGES C CXX)

# C와 C++ 표준 설정
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

# 빌드 타입 설정 (디버깅 활성화)
set(CMAKE_BUILD_TYPE Debug)

# OpenSSL 라이브러리 찾기
find_package(OpenSSL REQUIRED)

# pkg-config 및 MariaDB 라이브러리 설정
find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB REQUIRED mariadb)

# POSIX 스레드(Threads) 라이브러리 찾기 (스레드 사용을 위한 설정)
find_package(Threads REQUIRED)

# include 폴더 추가 (헤더 파일 경로)
include_directories(include)

# src 폴더 내의 모든 .c 파일 자동 검색
aux_source_directory(src SOURCES)

# 실행 파일 'gateway' 생성 (SOURCES 변수에 있는 파일들로 빌드)
add_executable(gateway ${SOURCES})

# 컴파일 옵션 설정 (-Wall, -Wextra, -Wpedantic: 모든 경고 출력)
target_compile_options(gateway PRIVATE -Wall -Wextra -Wpedantic)

# OpenSSL 라이브러리 링크 (OpenSSL::Crypto)
target_link_libraries(gateway OpenSSL::Crypto)

# MariaDB 라이브러리 링크
# 1. 라이브러리 디렉토리 추가
target_link_directories(gateway PRIVATE ${MARIADB_LIBRARY_DIRS})
# 2. 실제 MariaDB 라이브러리 링크 (pkg-config에서 가져온 변수 사용)
target_link_libraries(gateway ${MARIADB_LIBRARIES})

# POSIX 스레드 라이브러리 링크 추가 (Threads::Threads)
target_link_libraries(gateway Threads::Threads)
