if (CONFIG_MCUX_COMPONENT_middleware.genavb.avtp)

    mcux_add_source(
        SOURCES
        61883_iidc.c
        61883_iidc.h
        aaf.c
        aaf.h
        acf.c
        acf.h
        avtp.c
        avtp.cmake
        avtp_control.h
        avtp_entry.h
        avtp.h
        clock_domain.c
        clock_domain.h
        clock_grid.c
        clock_grid.h
        clock_source.c
        clock_source.h
        config.h
        crf.c
        crf.h
        cvf.c
        cvf.h
        media_clock.c
        media_clock.h
        mma.h
        stream.c
        stream.h
        rtos/main.c
        TOOLCHAINS armgcc mcux
    )

endif()
