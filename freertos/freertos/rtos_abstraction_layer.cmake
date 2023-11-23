message("rtos_abstraction_layer (freertos) headers are included for target ${RTOS_ABSTRACTION_LAYER_TARGET}")

target_include_directories(${RTOS_ABSTRACTION_LAYER_TARGET} PRIVATE
  ${FREERTOS_DIR}/include
  ${FREERTOS_DIR}/portable/${FREERTOS_PORT}
  ${CMAKE_CURRENT_LIST_DIR}
)

include_guard(GLOBAL)
message("rtos_abstraction_layer (freertos) sources are included for target ${RTOS_ABSTRACTION_LAYER_TARGET}")

target_sources(${RTOS_ABSTRACTION_LAYER_TARGET} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/atomic.c
)
