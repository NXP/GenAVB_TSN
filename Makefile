# Force usage from command line
ifneq ("$(origin V)", "command line")
	V = 0
endif

ifeq ($(V), 1)
	quiet =
else
	quiet = quiet_
endif

include config.mk

VERSION_FILE:=common/version.h
SCR_FILE:=SCR_genavb-release.txt

TOP_DIR:=$(shell pwd)

ifeq ($(target),)

# Iterate over targets
all install clean stack stack-core stack-os stack-install stack-core-install stack-os-install stack-clean doc doc_clean devkit-install devkit-install2:
	for target in $(target_list); do $(MAKE) -f Makefile target=$${target} $@ || exit;done

else

include config_$(target).mk

ifeq ($(config),)

# Iterate over configurations
all install clean stack stack-core stack-os stack-install stack-core-install stack-os-install stack-clean devkit-install devkit-install2:
	for config in $(config_list); do $(MAKE) -f Makefile target=$(target) config=$${config} $@ || exit;done

else

# Backward compatibility
ifeq ($(config),endpoint)
	override config := endpoint_avb
endif

include rules.mk
include $(target_os)/osal/configs/config

config_file:=$(objdir)/config

config:=$(shell \
	mkdir -p $(objdir); \
	cp configs/config_$(config) $(config_file); \
	cat $(osal_config_file) >> $(config_file); \
	)

include $(config_file)

all: stack
install: stack-install

stack: stack-core stack-os
stack-install: stack-core-install stack-os-install
stack-core-install: stack-core
stack-os-install: stack-os

maindirs:= common
maindirs+= public
ifeq ($(CONFIG_API),y)
maindirs+= api
endif
maindirs+= avdecc
maindirs+= avtp
maindirs+= maap
maindirs+= srp
maindirs+= gptp
maindirs+= management

maindirs+= $(target_os)

$(foreach dir,$(maindirs),$(eval $(call all_bins_subdir_handler,$(dir))))
$(foreach dir,$(maindirs),$(eval $(call subdir_handler,$(dir))))

$(foreach lib,$(all-libs),$(eval $(call lib_handler,$(lib),$(lib)-major,$(lib)-minor)))
$(foreach exec,$(all-execs),$(eval $(call exec_handler,$(exec))))
$(foreach archive,$(all-archives),$(eval $(call archive_handler,$(dir $(archive)),$(notdir $(archive)))))

include Makefile.$(target_os)

stack-core-install: include-install config-install

ifeq ($(release_mode),1)
include-install: include/genavb/* include/genavb/os/*
	install -d $(INCLUDE_DIR)/genavb/os
	install -m ug+rw include/genavb/*.h $(INCLUDE_DIR)/genavb
	install -m ug+rw include/genavb/os/* $(INCLUDE_DIR)/genavb/os
else
include-install: include/genavb/* include/$(target_os)/os/* $(objdir)/autoconf.h
	install -d $(INCLUDE_DIR)/genavb/os
	install -m ug+rw include/genavb/* $(INCLUDE_DIR)/genavb
	install -m ug+rw include/$(target_os)/os/* $(INCLUDE_DIR)/genavb/os
	install -m ug+rw $(objdir)/autoconf.h $(INCLUDE_DIR)/genavb
endif

config-install: $(config_file)
	install -d $(PREFIX)
	install -m ug+rw $(config_file) $(PREFIX)/config

$(objdir)/autoconf.h:
	@echo Generating autoconf.h
	@echo "/*\n * Auto-generated file\n * Do not modify\n */" >$(objdir)/autoconf.h
	@grep ^CONFIG.*=y$ <$(config_file) | sed -e 's/CONFIG/#define &/' -e 's/=y/ 1/' >>$(objdir)/autoconf.h
	@echo "#define CONFIG_$(target) 1" >>$(objdir)/autoconf.h

stack-clean:
	rm -fr $(all-obj)

clean: stack-clean

.PHONY: $(target_os)

ifndef release_mode
version:= $(shell \
	GENAVB_GIT_VERSION="$${GENAVB_GIT_VERSION:-unknown}"; \
	if [ -d .git ]; then  \
		if [ ! -z "`git describe --tags --exact-match 2>/dev/null`" ]; then \
			GENAVB_GIT_VERSION="`git describe --always --tags --dirty`" ;\
		else \
			postfix=""; \
			if ! git diff-index --quiet HEAD ; then \
				postfix="-dirty";\
			fi;\
			branch="`git rev-parse --abbrev-ref HEAD`" \
			commit="`git rev-parse --verify --short HEAD`" \
			GENAVB_GIT_VERSION=$$branch-$$commit$$postfix ; \
		fi; \
	fi; \
	VERSION="/* Auto-generated file. Do not edit !*/\n\#ifndef _VERSION_H_\n\#define _VERSION_H_\n\n\#define GENAVB_VERSION \"$${GENAVB_GIT_VERSION}\"\n\n\#endif /* _VERSION_H_ */\n"; \
	printf "$${VERSION}" | diff -qN - $(VERSION_FILE) > /dev/null ; \
	if [ $$? -ne 0 ]; then  \
		printf "$${VERSION}" > $(VERSION_FILE) ; \
		echo "generated $(VERSION_FILE) with version $${GENAVB_GIT_VERSION}" ; \
	fi; \
)
endif

endif

src_package: dist_clean
#	usage example: make target=linux_imx6 config=bridge src_package_dir=tmp src_package
	$(eval src_package_dir := genavb-$(config)-$(target_os)-src-release)
	@echo packaging $(config) sources into $(src_package_dir) for target $(target_os) / $(target)
	@mkdir $(src_package_dir)
#	collect all stack components for the selected OS and package
	$(foreach dir,$(maindirs),cp --parents `find $(dir) -maxdepth 1 -type f \( -name "*.c" -o -name "*.h" -o -name Makefile \)` $(src_package_dir)/.;)
	$(foreach dir,$(maindirs),if [ -d $(dir)/$(target_os) ]; then cp -r $(dir)/$(target_os) $(src_package_dir)/$(dir)/.; fi;)
	@cp -r $(target_os) os doc apps $(src_package_dir)/.
	@cp config_$(target_os)* config.mk Makefile Makefile.$(target_os) README* rules.mk $(src_package_dir)/.
	@mkdir -p $(src_package_dir)/common
	@cp -r common/os $(src_package_dir)/common/.
	@mkdir -p $(src_package_dir)/include
	@cp -r include/genavb include/$(target_os) $(src_package_dir)/include/.
	@mkdir -p $(src_package_dir)/configs
	@cp configs/config_$(config) $(src_package_dir)/configs/.
#	remove useless files
	@rm -fr `find $(src_package_dir) -name .gitignore`

doc:
	dot -T png doc/avdecc_control.dot > doc/avdecc_control.png
	mscgen -Tpng -F arial -i doc/acmp_connect.msc -o doc/acmp_connect.png
	cd doc/_CONFIG_/ ; ./Generate_API_Reference.sh $(target_os)
	mkdir -p build/$(target_os)/docs/html/
	cp -ax doc/_OUTPUT_/$(target_os)/HTML_Help/* build/$(target_os)/docs/html/
#	cp -ax doc/_OUTPUT_/*pdf build/docs/

doc_clean:
	rm -rf doc/acmp_connect.png
	rm -rf doc/avdecc_control.png
	rm -rf doc/_CONFIG_/doxylog.txt
	rm -rf build/$(target_os)/docs
endif


version_clean:
ifndef release_mode
	rm -f $(VERSION_FILE)
endif

dist_clean: version_clean
#	clean up source tree
	find . -name "*~" | xargs rm -f
	find . -name "*.o" | xargs rm -f
#	clean release sdk
	rm -rf genavb-*-sdk*
	rm -rf genavb-linux-*
	rm -rf genavb-freertos-*
#	cleanup build folder
	rm -rf build

clean: doc_clean

.PHONY: clean doc linux
