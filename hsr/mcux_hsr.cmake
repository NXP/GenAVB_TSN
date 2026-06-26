if (CONFIG_MCUX_COMPONENT_middleware.genavb.hsr)

    mcux_add_source(
        SOURCES
        hsr.c
        hsr.h
        rtos/hsr_main.c
        TOOLCHAINS armgcc mcux
    )

endif()
