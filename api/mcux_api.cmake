if (CONFIG_MCUX_COMPONENT_middleware.genavb.api)

    include(${CMAKE_CURRENT_LIST_DIR}/rtos/mcux_api.cmake)

    mcux_add_source(
        SOURCES
        clock.c
        clock.h
        config.h
        control.c
        control.h
        error.c
        init.h
        timer.h
        version.c
    )

endif()
