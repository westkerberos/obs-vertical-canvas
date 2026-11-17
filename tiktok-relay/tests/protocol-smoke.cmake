if(NOT DEFINED RELAY_EXECUTABLE OR NOT DEFINED INPUT_FILE)
  message(FATAL_ERROR "RELAY_EXECUTABLE and INPUT_FILE are required")
endif()

execute_process(
  COMMAND "${RELAY_EXECUTABLE}"
  INPUT_FILE "${INPUT_FILE}"
  OUTPUT_VARIABLE relay_output
  ERROR_VARIABLE relay_error
  RESULT_VARIABLE relay_result
)

if(NOT relay_result EQUAL 0)
  message(FATAL_ERROR "Relay exited with ${relay_result}: ${relay_error}")
endif()

foreach(expected
    "\"relay\":\"west-tiktok-relay\""
    "\"protocol_version\":1"
    "\"rtmp_ingest\":false"
    "\"state\":\"idle\""
    "\"error\":\"not_implemented\""
    "\"error\":\"unknown_command\""
    "\"request_id\":\"hello-1\""
    "\"request_id\":\"caps-1\""
    "\"request_id\":\"status-1\""
    "\"request_id\":\"start-1\""
    "\"request_id\":\"stop-1\""
    "\"error\":\"protocol_version_mismatch\""
    "\"request_id\":\"version-1\""
    "\"error\":\"invalid_request_id\"")
  string(FIND "${relay_output}" "${expected}" match_index)
  if(match_index EQUAL -1)
    message(FATAL_ERROR "Expected protocol marker not found: ${expected}\nOutput: ${relay_output}")
  endif()
endforeach()
