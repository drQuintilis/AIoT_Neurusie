cmake_minimum_required(VERSION 3.20)
project(AIoT_Neurusie C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

# Core backend modules — pure C, no hardware dependencies.
set(GUARDIAN_SOURCES
        src/soft_timer.c
        src/intersection_fsm.c
        src/traffic_logic.c
        src/event_logger.c
        src/protocol.c
)

add_library(guardian STATIC ${GUARDIAN_SOURCES})
target_include_directories(guardian PUBLIC include)

# HAL stub backend for PC. On Arduino, swap this for hal_arduino.c.
add_library(hal_stub STATIC src/hal_stub.c)
target_include_directories(hal_stub PUBLIC include)

# Demo binary wiring all modules together — runs on PC for testing.
add_executable(guardian_demo src/main.c)
target_link_libraries(guardian_demo PRIVATE guardian)

# --- Tests ---------------------------------------------------------------
enable_testing()

function(add_guardian_test name source)
    add_executable(${name} ${source})
    target_include_directories(${name} PRIVATE test)
    target_link_libraries(${name} PRIVATE guardian)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_guardian_test(test_soft_timer    test/test_soft_timer.c)
add_guardian_test(test_fsm           test/test_fsm.c)
add_guardian_test(test_traffic_logic test/test_traffic_logic.c)
add_guardian_test(test_event_logger  test/test_event_logger.c)
add_guardian_test(test_protocol      test/test_protocol.c)

# HAL test also needs the stub backend.
add_executable(test_hal_stub test/test_hal_stub.c)
target_include_directories(test_hal_stub PRIVATE test)
target_link_libraries(test_hal_stub PRIVATE hal_stub)
add_test(NAME test_hal_stub COMMAND test_hal_stub)
