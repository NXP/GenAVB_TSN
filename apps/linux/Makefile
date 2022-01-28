subdirs:= management-app

ifeq ($(CONFIG_SOCKET),y)
subdirs+= tsn-app
endif

ifeq ($(CONFIG_SRP),y)
subdirs+= msrp-ctrl-app
endif

ifeq ($(CONFIG_MAAP),y)
subdirs+= maap-ctrl-app
endif

ifeq ($(CONFIG_AVDECC),y)
subdirs+= genavb-gui-controller genavb-controls-app genavb-controller-app aem-manager
endif

ifeq ($(CONFIG_AVTP),y)
subdirs+= simple-audio-app alsa-audio-app genavb-multi-stream-app genavb-video-player-app genavb-video-server-app genavb-media-app salsacamctrl simple-acf-app
endif

ifeq ($(CONFIG_1733),y)
subdirs:= 1733_app
endif
