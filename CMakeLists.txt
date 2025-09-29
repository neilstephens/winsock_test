cmake_minimum_required(VERSION 3.15)
project(tcp_connect VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable
add_executable(tcp_connect tcp_connect.cpp)

# Link Winsock library on Windows
if(WIN32)
    target_link_libraries(tcp_connect PRIVATE ws2_32)
endif()

# Set output directory
set_target_properties(tcp_connect PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Installation rules
install(TARGETS tcp_connect
    RUNTIME DESTINATION bin
)
