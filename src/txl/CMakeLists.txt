add_library(txl expect.cpp)
target_include_directories(txl PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)
