.SECONDEXPANSION:

objdir?= build/


# Add simple rule to call apps Makefiles
# $(1): name of current os
# $(2): name of apps subdir
define apps_handler
bindir:=$$(os_objdir)/$(2)

$$(bindir):
	mkdir -p $$@

$(2): $$(bindir)
$(2):
	+ $(MAKE) -C $(1)/$(2) OBJDIR=../../$(os_objdir)/$(2)/
$(1): $(2)

$(2)-install: $(2)
	+ $(MAKE) -C $(1)/$(2) install OBJDIR=../../$(os_objdir)/$(2)/
$(1)-install: $(2)-install

$(2)-devkit-install:
	cp -r $(1)/$(2) $(DEVKIT_DIR)
	rm -f $(DEVKIT_DIR)/$(2)/.gitignore
devkit-install: $(2)-devkit-install

$(2)-clean:
	$(MAKE) -C $(1)/$(2) clean OBJDIR=../../$(os_objdir)/$(2)/
clean: $(2)-clean
endef


# Step through a given OS subdir
# $(1): os subdir to parse
#
define apps_os_handler
os_objdir:= $$(objdir)$(1)
$(if $(wildcard $(1)/Makefile), include $(1)/Makefile, subdirs= )

#$$(info RULES.MK os=$(1) makefile=$(1)/Makefile objdir=$$(objdir) os_objdir=$$(os_objdir) subdirs=$$(subdirs))
$$(foreach dir,$$(subdirs),$$(eval $$(call apps_handler,$(1),$$(dir))))
$$(os_objdir):
	mkdir -p $$@

$(1): $$(os_objdir)
apps: $(1)

devkit-install:
	cp -r $(1)/common $(DEVKIT_DIR)
	cp $(1)/Makefile $(DEVKIT_DIR)

$(1)_clean:
	rm -rf $$(objdir)$(1)
	
clean: $(1)_clean
endef
