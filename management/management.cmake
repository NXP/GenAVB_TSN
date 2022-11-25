if(CONFIG_MANAGEMENT)

  genavb_conditional_include(${TARGET_OS}/management.cmake)
  genavb_add_prebuilt_library(LIB libmanagement-${TARGET_ARCH}.a)
  genavb_link_libraries(TARGET ${tsn} LIB libmanagement-${TARGET_ARCH}.a)

endif()
