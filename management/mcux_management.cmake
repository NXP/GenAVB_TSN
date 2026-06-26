if (CONFIG_MCUX_COMPONENT_middleware.genavb.management)

    mcux_add_source(
        SOURCES
        config.h
        mac_service.c
        mac_service.h
        management.c
        management_entry.h
        management.h
        rtos/management_main.c
        TOOLCHAINS armgcc mcux
    )

endif()
