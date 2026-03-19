if(CONFIG_MCUX_COMPONENT_middleware.genavb.apps.aem_manager)
    mcux_add_iar_configuration(
        CC "\
        --diag_suppress=Pe068 \
        --diag_suppress=Pe084 \
    ")

    include(${CMAKE_CURRENT_LIST_DIR}/../../common/aem-manager/mcux_aem_audio_entities.cmake)

    mcux_add_include(
        INCLUDES
        .
    )

    mcux_add_source(
        SOURCES
        aem_manager_rtos.c
        aem_manager_rtos.h
    )

    mcux_add_source(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../common/aem-manager
        SOURCES
        aem_manager_helpers.c
        aem_manager_helpers.h
    )

    mcux_add_include(
        BASE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../common/aem-manager
        INCLUDES
        .
    )
endif()
