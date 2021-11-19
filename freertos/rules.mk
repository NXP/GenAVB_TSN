define lib_handler
all-obj += $(objdir)/lib$(1).a

stack-core: $(objdir)/lib$(1)-core.a
stack-os: $(objdir)/lib$(1)-freertos.a

stack-core-install devkit-install2: $(1)-core-install
stack-os-install: $(1)-os-install

$(1)-core-install: $(objdir)/lib$(1)-core.a
	install -D $$^ $(LIB_DIR)/lib$(1)-core.a

$(1)-os-install: $(objdir)/lib$(1)-freertos.a
	install -D $$^ $(LIB_DIR)/lib$(1)-freertos.a

quiet_cmd_freertos_ar = AR $$@
      cmd_freertos_ar = $(AR) crT $$@ $$^; echo "create $$@\naddlib $$@\nsave\nend" | $(AR) -M

ifeq ($(release_mode),1)
$(objdir)/lib$(1)-core.a: $(TOP_DIR)/lib/lib$(1)-core.a
	cp $$^ $$@
else
$(objdir)/lib$(1)-core.a: $$($(1)-common-obj) $$($(1)-common-ar)
	$$(call cmd,freertos_ar)
endif

$(objdir)/lib$(1)-freertos.a: $$($(1)-freertos-obj)
	$$(call cmd,freertos_ar)

endef
