# Mandatory variables for this cmake file: TOPDIR and VERSION_FILE

if(NOT DEFINED TOPDIR)
  message(FATAL_ERROR "TOPDIR not defined")
endif()

if(NOT DEFINED VERSION_FILE)
  message(FATAL_ERROR "VERSION_FILE not defined")
endif()

if(NOT DEFINED genavb_git_version)
  set(genavb_git_version "unknown")
endif()

if(EXISTS ${TOPDIR}/.git)
  execute_process(COMMAND git describe --tags --exact-match ERROR_QUIET WORKING_DIRECTORY ${TOPDIR} RESULT_VARIABLE exact_match_result OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(exact_match_result EQUAL 0)
    # This is a tag directly referencing HEAD
    execute_process(COMMAND git describe --always --tags --dirty WORKING_DIRECTORY ${TOPDIR} RESULT_VARIABLE decribe_tag_result OUTPUT_VARIABLE genavb_git_version OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    # This is not an exact match to a tag, get the branch and commit.
    set(postfix "")
    # Add dirty postfix if there is some modified tracked files.
    execute_process(COMMAND git diff-index --quiet HEAD WORKING_DIRECTORY ${TOPDIR} RESULT_VARIABLE diff_head_tracked)
    if(NOT diff_head_tracked EQUAL 0)
      set(postfix "-dirty")
    endif()
    execute_process(COMMAND git rev-parse --abbrev-ref HEAD WORKING_DIRECTORY ${TOPDIR} RESULT_VARIABLE branch_result OUTPUT_VARIABLE branch OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND git rev-parse --verify --short HEAD WORKING_DIRECTORY ${TOPDIR} RESULT_VARIABLE commit_result OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(genavb_git_version ${branch}-${commit}${postfix})
  endif()
endif()

file(WRITE ${VERSION_FILE} "/* Auto-generated file. Do not edit !*/\n")
file(APPEND ${VERSION_FILE} "#ifndef _VERSION_H_\n")
file(APPEND ${VERSION_FILE} "#define _VERSION_H_\n\n")
file(APPEND ${VERSION_FILE} "#define GENAVB_VERSION \"${genavb_git_version}\"\n\n")
file(APPEND ${VERSION_FILE} "#endif /* _VERSION_H_ */\n")
message(STATUS "generated ${VERSION_FILE} with version ${genavb_git_version}")
