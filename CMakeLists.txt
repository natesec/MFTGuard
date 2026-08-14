cmake_minimum_required(VERSION 4.4)

project(MFTGuard
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(MFTGuard
    src/main.c
    src/mft.c
    src/attributes.c
    src/timestamps.c
    src/rules.c
    src/report.c
    src/utils.c
    third_party/cjson/cJSON.c
)

target_include_directories(MFTGuard PRIVATE
    include
    third_party/cjson
    third_party/uthash
)
