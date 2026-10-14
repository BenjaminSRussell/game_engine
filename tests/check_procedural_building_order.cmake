# Issue #273: compile and run the city_generator.h declaration-order test.
# Does not link Engine (that target fails for pre-existing reasons).
if(NOT DEFINED HEADER_DIR)
    message(FATAL_ERROR "HEADER_DIR is required")
endif()
if(NOT DEFINED TEST_SRC)
    message(FATAL_ERROR "TEST_SRC is required")
endif()
if(NOT DEFINED C_COMPILER)
    message(FATAL_ERROR "C_COMPILER is required")
endif()
if(NOT DEFINED OUT_BIN)
    message(FATAL_ERROR "OUT_BIN is required")
endif()

execute_process(
    COMMAND "${C_COMPILER}" -std=c11 -Wall -Werror
            -I "${HEADER_DIR}"
            "${TEST_SRC}" -o "${OUT_BIN}"
    RESULT_VARIABLE _compile_rc
    OUTPUT_VARIABLE _compile_out
    ERROR_VARIABLE _compile_err
)
if(NOT _compile_rc EQUAL 0)
    message(FATAL_ERROR
        "ProceduralBuilding order compile failed (${_compile_rc}):\n${_compile_err}${_compile_out}")
endif()

execute_process(
    COMMAND "${OUT_BIN}"
    RESULT_VARIABLE _run_rc
    OUTPUT_VARIABLE _run_out
    ERROR_VARIABLE _run_err
)
if(NOT _run_rc EQUAL 0)
    message(FATAL_ERROR
        "ProceduralBuilding order test failed (${_run_rc}):\n${_run_out}${_run_err}")
endif()

message(STATUS "${_run_out}")
