project(red1z)
cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_compile_options(-W -Wall -Wextra -pedantic)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(red1z ${SRC}/reply.cpp ${SRC}/socket.cpp)
#target_compile_features(red1z PUBLIC cxx_std_17)
#set_target_properties(red1z PROPERTIES CXX_EXTENSIONS OFF)

add_executable(test1 redis2.cpp)
#target_compile_features(test1 PUBLIC cxx_std_17)
#set_target_properties(test1 PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(test1 red1z)

install(TARGETS red1z
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
  )
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/red1z" # source directory
        DESTINATION include # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h" # select header files
)
