
#Set those below to match your environment
# OBJDIR: path where generated binaries should be stored
# GENAVB_INCLUDE: path to the public genavb includes files

OBJDIR?=
GENAVB_INCLUDE?=../../../include
APP_NAME=aem-manager


HOSTCC=gcc

CUSTOM_CFLAGS:=$(addprefix -D, $(CUSTOM_DEFINES))
CFLAGS= $(CUSTOM_CFLAGS) -O2 -Wall -Werror -g -I$(GENAVB_INCLUDE)

ENTITIES= \
	listener_audio_single.c \
	listener_talker_audio_single.c \
	listener_video_multi.c \
	listener_video_single.c \
	talker_audio_single.c \
	talker_audio_video.c \
	talker_video_multi.c \
	talker_video_single.c \
	talker_listener_audio_multi.c \
	talker_listener_audio_multi_aaf.c \
	talker_listener_audio_multi_format.c \
	controller.c \
	avnu_certification.c

$(OBJDIR)$(APP_NAME): main.c $(ENTITIES) ../../../public/aem_helpers.c ../../../public/linux/aem_helpers.c ../../../public/helpers.c
	$(HOSTCC) $(CFLAGS) -o $@ $^

install: $(OBJDIR)$(APP_NAME)
	$(OBJDIR)$(APP_NAME) -c
	install -d $(CONFIG_DIR)
	install -m ug+rw *.aem  -t $(CONFIG_DIR)/
	rm -fr *.aem

clean:
	rm -rf $(OBJDIR)$(APP_NAME) $(ENTITIES:.c=.aem)
