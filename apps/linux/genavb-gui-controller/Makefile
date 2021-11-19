
#Set those below to match your environment
# CROSS_COMPILE: path to cross-compiler
# OBJDIR: path where generated binaries should be stored

CROSS_COMPILE?=/home/user/dev/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin/arm-linux-gnueabihf-
OBJDIR?=
GENAVB_PATH?=../../../../lib
GENAVB_INCLUDE?=../../../../include
APP_NAME=genavb-gui-controller
APP_DIR=src

# Evaluate if CC has been set or not:
ifeq "$(origin CC)" "default"
CC:=$(CROSS_COMPILE)gcc
endif

# Evaluate if CXX has been set or not:
ifeq "$(origin CXX)" "default"
CXX:=$(CROSS_COMPILE)g++
endif

STAGING_DIR?=/home/user/dev/buildroot/output/staging
# Normally, qmake should be found in the PATH. However on some build systems
# it may be required to explicitely define its entire path if it is placed
# in the staging dir.
QMAKE?=qmake

$(OBJDIR)$(APP_NAME): $(OBJDIR)/$(APP_DIR)/Makefile
	$(MAKE) -C $(OBJDIR)/$(APP_DIR) "CXX=${CXX}" "CC=${CC}" "AR=${AR}"
	mv $(OBJDIR)/$(APP_DIR)/$(APP_NAME) $(OBJDIR)$(APP_NAME)

$(OBJDIR)/$(APP_DIR)/Makefile:
	$(QMAKE) DEFINES+="$(CUSTOM_DEFINES)" QMAKE_CFLAGS_ISYSTEM=" " -o $(OBJDIR)/$(APP_DIR)/Makefile "LIBS += -L$(GENAVB_PATH) -lgenavb" "INCLUDEPATH += $(GENAVB_INCLUDE) $(STAGING_DIR)/usr/include" $(APP_DIR)/$(APP_NAME).pro

install: $(OBJDIR)$(APP_NAME)
	install -D $(OBJDIR)$(APP_NAME) $(BIN_DIR)/$(APP_NAME)

clean:
ifneq ($(wildcard $(APP_DIR)/Makefile),)
	$(MAKE) -C $(APP_DIR) clean "CXX=${CXX}" "CC=${CC}" "AR=${AR}"
endif
	rm -rf $(OBJDIR)$(APP_NAME)
	rm -f $(APP_DIR)/$(APP_NAME)
	rm -f $(APP_DIR)/Makefile
