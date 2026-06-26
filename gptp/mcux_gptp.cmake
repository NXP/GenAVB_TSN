if (CONFIG_MCUX_COMPONENT_middleware.genavb.gptp)

    mcux_add_source(
        SOURCES
        bmca.h
        bmca.c
        clock_ms_fsm.c
        clock_ms_fsm.h
        clock_sl_fsm.c
        clock_sl_fsm.h
        config.h
        gptp.c
        gptp_entry.h
        gptp.h
        gptp_managed_objects.c
        gptp_managed_objects.h
        md_fsm_802_3.c
        md_fsm_802_3.h
        port_fsm.c
        port_fsm.h
        ptp.h
        ptp_time_ops.c
        ptp_time_ops.h
        rtos
        site_fsm.c
        site_fsm.h
        target_clock_adj.c
        target_clock_adj.h
        rtos/gptp_main.c
        TOOLCHAINS armgcc mcux
    )

endif ()
