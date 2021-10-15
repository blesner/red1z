project(red1z)
cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_compile_options(-W -Wall -Wextra -pedantic)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(red1z ${SRC}/reply.cpp ${SRC}/socket.cpp ${SRC}/redis.cpp)


add_executable(demo examples/demo.cpp)
add_executable(stream examples/stream.cpp)

target_link_libraries(demo red1z pthread)
target_link_libraries(stream red1z)

install(TARGETS red1z
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )

install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/red1z"
        DESTINATION include
        FILES_MATCHING
        PATTERN "*.h"
)
