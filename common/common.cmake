genavb_target_add_srcs(TARGET genavb SRCS 61883_iidc.c aaf.c avdecc.c log.c srp.c)
genavb_link_libraries(TARGET ${avb} LIB libcommon-${TARGET_ARCH}.a)
genavb_link_libraries(TARGET ${tsn} LIB libcommon-${TARGET_ARCH}.a)
