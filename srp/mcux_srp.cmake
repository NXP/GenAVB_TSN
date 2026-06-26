if (CONFIG_MCUX_COMPONENT_middleware.genavb.srp)

    mcux_add_source(
        SOURCES
        config.h
        mmrp.c
        mmrp.h
        mrp.c
        mrp.h
        msrp.c
        msrp.h
        msrp_map.c
        msrp_map.h
        mvrp.c
        mvrp.h
        mvrp_map.c
        mvrp_map.h
        rtos
        srp.c
        srp_entry.h
        srp.h
        srp_managed_objects.c
        srp_managed_objects.h
        rtos/srp_main.c
        TOOLCHAINS armgcc mcux
    )

endif()
