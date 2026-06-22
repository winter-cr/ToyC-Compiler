if(NOT DEFINED TOYC_EXECUTABLE)
    message(FATAL_ERROR "TOYC_EXECUTABLE is required")
endif()

if(NOT DEFINED TEST_INPUT)
    message(FATAL_ERROR "TEST_INPUT is required")
endif()

if(NOT DEFINED TEST_OUTPUT)
    message(FATAL_ERROR "TEST_OUTPUT is required")
endif()

execute_process(
    COMMAND "${TOYC_EXECUTABLE}"
    INPUT_FILE "${TEST_INPUT}"
    OUTPUT_FILE "${TEST_OUTPUT}"
    RESULT_VARIABLE compile_result
)

if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "compiler exited with ${compile_result}")
endif()

file(SIZE "${TEST_OUTPUT}" output_size)
if(output_size EQUAL 0)
    message(FATAL_ERROR "compiler produced empty output")
endif()
