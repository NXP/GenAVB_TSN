find_package(Doxygen
             OPTIONAL_COMPONENTS dot mscgen)

if(DOXYGEN_FOUND AND TARGET Doxygen::dot AND TARGET Doxygen::mscgen)
  set(DOXYGEN_COMMON_INPUT_FILES
    ${CMAKE_CURRENT_LIST_DIR}/../include/genavb
    ${CMAKE_CURRENT_LIST_DIR}/init.md
    ${CMAKE_CURRENT_LIST_DIR}/streaming.md
    ${CMAKE_CURRENT_LIST_DIR}/stream_formats.md
    ${CMAKE_CURRENT_LIST_DIR}/control.md
    ${CMAKE_CURRENT_LIST_DIR}/socket.md
    ${CMAKE_CURRENT_LIST_DIR}/clock.md)

  if("${TARGET_OS}" STREQUAL "linux")
    set(DOXYGEN_ENABLED_SECTIONS LINUX)
    set(DOXYGEN_OS_INPUT_FILES
      ${CMAKE_CURRENT_LIST_DIR}/../include/linux
      ${CMAKE_CURRENT_LIST_DIR}/gptp.md
      ${CMAKE_CURRENT_LIST_DIR}/config.md
      ${CMAKE_CURRENT_LIST_DIR}/platform_linux.md
      ${CMAKE_CURRENT_LIST_DIR}/mainpage_linux.md)
  elseif("${TARGET_OS}" STREQUAL "freertos")
    set(DOXYGEN_ENABLED_SECTIONS FREERTOS)
    set(DOXYGEN_OS_INPUT_FILES
      ${CMAKE_CURRENT_LIST_DIR}/../include/freertos
      ${CMAKE_CURRENT_LIST_DIR}/timer.md
      ${CMAKE_CURRENT_LIST_DIR}/scheduled_traffic.md
      ${CMAKE_CURRENT_LIST_DIR}/frame_preemption.md
      ${CMAKE_CURRENT_LIST_DIR}/mainpage_freertos.md)
  endif()

  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_WARN_AS_ERROR NO)
  set(DOXYGEN_MARKDOWN_SUPPORT YES)
  set(DOXYGEN_GENERATE_TREEVIEW YES)
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
  set(DOXYGEN_PREDEFINED "__attribute__(x)= static inline")
  set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
  set(DOXYGEN_FILE_PATTERNS *.h *.md *.dox *.png *.jpg *.gif)
  set(DOXYGEN_PROJECT_LOGO ${CMAKE_CURRENT_LIST_DIR}/help_template/nxp_logo.png)
  set(DOXYGEN_LAYOUT_FILE ${CMAKE_CURRENT_LIST_DIR}/help_template/HTML_PageLayout.xml)
  set(DOXYGEN_HTML_HEADER ${CMAKE_CURRENT_LIST_DIR}/help_template/html_header.html)
  set(DOXYGEN_HTML_FOOTER ${CMAKE_CURRENT_LIST_DIR}/help_template/html_footer.html)
  set(DOXYGEN_HTML_EXTRA_STYLESHEET ${CMAKE_CURRENT_LIST_DIR}/help_template/html_custom.css)
  set(DOXYGEN_HTML_OUTPUT html)
  if(DOC_OUTPUT_DIR)
    set(DOXYGEN_OUTPUT_DIRECTORY ${DOC_OUTPUT_DIR}/${TARGET_OS})
  else()
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/${TARGET_OS})
  endif()
  set(DOXYGEN_IMAGE_PATH ${DOXYGEN_OUTPUT_DIRECTORY})

  doxygen_add_docs(doc_doxygen
    ${DOXYGEN_COMMON_INPUT_FILES} ${DOXYGEN_OS_INPUT_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Generating ${TARGET_OS} HTML docs")

  add_custom_target(doc_dot_gen
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DOXYGEN_OUTPUT_DIRECTORY}/${DOXYGEN_HTML_OUTPUT}/
    COMMAND Doxygen::dot -T png ${CMAKE_CURRENT_LIST_DIR}/avdecc_control.dot -o ${DOXYGEN_OUTPUT_DIRECTORY}/${DOXYGEN_HTML_OUTPUT}/avdecc_control.png
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generate PNG from dot")

  add_dependencies(doc_doxygen doc_dot_gen)

  add_custom_target(doc_msc_gen
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DOXYGEN_OUTPUT_DIRECTORY}/${DOXYGEN_HTML_OUTPUT}/
    COMMAND Doxygen::mscgen -Tpng -F arial -i ${CMAKE_CURRENT_LIST_DIR}/acmp_connect.msc -o ${DOXYGEN_OUTPUT_DIRECTORY}/${DOXYGEN_HTML_OUTPUT}/acmp_connect.png
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generate PNG from MSC")
  add_dependencies(doc_doxygen doc_msc_gen)
else()
  message(WARNING "Doxygen tools not found: Documentation generation won't be possible.")
endif()
