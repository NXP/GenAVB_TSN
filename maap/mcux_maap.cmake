if (CONFIG_MCUX_COMPONENT_middleware.genavb.maap)

    mcux_add_source(
        SOURCES
        config.h
        maap.c
        maap_entry.h
        maap.h
        rtos/main.c
        TOOLCHAINS armgcc mcux
    )

endif()
