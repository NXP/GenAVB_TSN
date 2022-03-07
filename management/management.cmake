if(CONFIG_MANAGEMENT)

  genavb_conditional_include(${TARGET_OS}/management.cmake)
  genavb_link_libraries(TARGET ${fgptp} LIB libmanagement-${TARGET_ARCH}.a)

endif()
