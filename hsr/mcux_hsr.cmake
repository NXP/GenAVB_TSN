if (CONFIG_MCUX_COMPONENT_middleware.genavb.hsr)

    mcux_add_source(
        SOURCES
        hsr.c
        hsr.h
        rtos/main.c
        TOOLCHAINS armgcc mcux
    )

endif()
