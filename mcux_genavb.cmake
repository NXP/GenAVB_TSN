cmake_minimum_required(VERSION 3.22)

set(TOPDIR ${CMAKE_CURRENT_LIST_DIR})
set(VERSION_FILE ${TOPDIR}/common/version.h)

if (CONFIG_MCUX_COMPONENT_middleware.genavb)

    include(${CMAKE_CURRENT_LIST_DIR}/common/gen_version.cmake OPTIONAL)

    mcux_add_include(
        INCLUDES
        .
        rtos/include
        include
        include/rtos
        TOOLCHAINS armgcc mcux
    )

    mcux_add_source(
        SOURCES
        rtos/include/*.h
        rtos/include/osal/*.h
        include/genavb/*.h
        include/rtos/_os/*.h
        os/*.h
        TOOLCHAINS armgcc mcux
    )

    mcux_add_source(
        SOURCES
        common/version.h
        TOOLCHAINS armgcc mcux
    )

    include(${CMAKE_CURRENT_LIST_DIR}/api/mcux_api.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/avdecc/mcux_avdecc.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/avtp/mcux_avtp.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/common/mcux_common.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/gptp/mcux_gptp.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/hsr/mcux_hsr.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/maap/mcux_maap.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/management/mcux_management.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/public/mcux_public.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/rtos/mcux_rtos.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/srp/mcux_srp.cmake)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/apps/rtos/aem-manager/mcux_aem_manager_rtos.cmake)

if (CONFIG_MCUX_COMPONENT_middleware.genavb.lib)
    mcux_add_source(
        SOURCES
        include/genavb/*.h
        include/rtos/_os/*.h
        TOOLCHAINS iar
    )

    mcux_add_include(
        INCLUDES
        include
        include/rtos
        TOOLCHAINS iar
    )

    mcux_add_library(
        BASE_PATH ${SdkRootDirPath}/gen_avb-libs/${lib_target}/${lib_config}
        LIBS
        "libstack-core.a"
        TOOLCHAINS iar
    )

    mcux_add_library(
        BASE_PATH ${SdkRootDirPath}/gen_avb-libs/${lib_target}/${lib_config}
        LIBS
        "libstack-rtos.a"
        TOOLCHAINS iar
    )
endif()
