define lib_handler

# $(1) lib name
# $(2) major version
# $(3) minor version

all-obj += $(objdir)/lib$(1).a $(objdir)/lib$(1).so.$$($2).$$($3)
stack-os:  $(objdir)/lib$(1).a $(objdir)/lib$(1).so.$$($2).$$($3)
stack-core:
stack-os-install: $(1)-install
stack-core-install:

$$($(1)-common-obj) $$($(1)-linux-obj): CFLAGS+=-fPIC

quiet_cmd_linux_ar = AR $$@
      cmd_linux_ar = $(AR) rcs $$@ $$^

$(objdir)/lib$(1).a: $$($(1)-common-obj) $$($(1)-linux-obj) $$($(1)-common-ar)
	$$(call cmd,linux_ar)

quiet_cmd_linux_solib = CC $$@
      cmd_linux_solib = $(CC) $(CFLAGS) $$($(1)_CFLAGS) -shared -Wl,$$($(1)_LDFLAGS),-soname,lib$(1).so.$$($2),--no-undefined -o $$@  $$^

$(objdir)/lib$(1).so.$$($2).$$($3): $$($(1)-common-obj) $$($(1)-linux-obj) $$($(1)-common-ar)
	$$(call cmd,linux_solib)
	ln -sf lib$(1).so.$$($2).$$($3) $(objdir)/lib$(1).so

$(1)-install: $(objdir)/lib$(1).so.$$($2).$$($3)
	install -D $(objdir)/lib$(1).so.$$($2).$$($3) $(LIB_DIR)/lib$(1).so.$$($2).$$($3)
	ln -sf lib$(1).so.$$($2).$$($3) $(LIB_DIR)/lib$(1).so

endef

define exec_handler
all-obj += $(objdir)/$(1)
stack-os: $(objdir)/$(1)
stack-core: $$($(1)-common-ar)
stack-os-install: $(1)-install
stack-core-install:

quiet_cmd_linux_cc = CC $$@
      cmd_linux_cc = $(CC) $(CFLAGS) $$($(1)_CFLAGS) -no-pie -pthread -Wl,--start-group $$^ -Wl,--end-group -o $$@

$(objdir)/$(1): $$($(1)-linux-obj) $$($(1)-common-obj) $$($(1)-common-ar)
	$$(call cmd,linux_cc)

$(1)-install: $(objdir)/$(1)
	install -D $(objdir)/$(1) $(BIN_DIR)/$(1)
endef

linux_CFLAGS +=-I$(STAGING_DIR)/usr/include
