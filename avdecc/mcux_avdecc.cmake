if (CONFIG_MCUX_COMPONENT_middleware.genavb.avdecc)

    mcux_add_source(
        SOURCES
        acmp.c
        acmp.h
        acmp_ieee.c
        acmp_ieee.h
        acmp_milan.c
        acmp_milan.h
        adp.c
        adp.h
        adp_ieee.c
        adp_ieee.h
        adp_milan.c
        adp_milan.h
        aecp.c
        aecp.h
        aem.c
        aem.h
        avdecc.c
        avdecc_entry.h
        avdecc.h
        avdecc_ieee.c
        avdecc_ieee.h
        config.h
        entity.c
        entity.h
        rtos/main.c
        TOOLCHAINS armgcc mcux
    )

endif()
