
.SECONDEXPANSION:

# PREFIX must be an absolute path
export PREFIX?=$(TOP_DIR)/build/$(target)/$(config)/target

# The variables below will use the updated PREFIX
export BIN_DIR
export CONFIG_DIR

# echo command.
# Short version is used, if $(quiet) equals `quiet_', otherwise full one.
echo-cmd =  $(if $($(quiet)cmd_$(1)),\
	echo $($(quiet)cmd_$(1));)

# printing commands
cmd = @$(echo-cmd) $(cmd_$(1))

objdir:=build/$(target)/$(config)
arch_objdir:=build/$(target_arch)
srcdir:=.

CFLAGS+= -iquote .
CFLAGS+= -iquote include
CFLAGS+= -iquote include/$(target_os)
CFLAGS+= -iquote $(target_os) $($(config)_CFLAGS)
CFLAGS+= -DTARGET_$(target)

all-obj:=

all-execs:=
all-libs:=
all-archives:=
_all-archives=$(notdir $(all-archives))

define archive_handler
all-obj += $(arch_objdir)/$(1)$(2).a

stack-core: $(arch_objdir)/$(1)$(2).a
devkit-install2: $(2)-devkit-install

quiet_cmd_linux_ar = AR $$@
      cmd_linux_ar = $(AR) rcs $$@ $$^

ifdef release_mode
$(arch_objdir)/$(1)$(2).a: $(TOP_DIR)/$(1)$(2)-$(target_arch).a
	cp $$^ $$@
else
$(arch_objdir)/$(1)$(2).a: $$($(2)-common-obj)
	$$(call cmd,linux_ar)
endif

$(2)-devkit-install: $(arch_objdir)/$(1)$(2).a
	install -D $$^ $(DEVKIT_DIR)/$(1)$(2)-$(target_arch).a

endef

define dep_include

-include $(addprefix $(1), $(2:%.o=%.d))

endef

#
# OS specific object handler
#
# $(1): path/to/os_specific_code
# $(2): os name
define os_obj_handler
$$(foreach bin,$$(all-execs) $$(all-libs),$$(eval $$(bin)-obj:=))

include $(1)/Makefile

$(shell mkdir -p $(objdir)/$(1))

# Collect platform specific object list (e.g., linux-obj+=, ...)
$$(foreach bin,$$(all-execs) $$(all-libs),$$(eval $$(bin)-$(2)-obj+=$$(addprefix $(objdir)/$(1)/,$$($$(bin)-obj))))
$$(foreach bin,$$(all-execs) $$(all-libs),$$(eval all-obj+=$$(addprefix $(objdir)/$(1)/,$$($$(bin)-obj))))
$$(foreach bin,$$(all-execs) $$(all-libs),$$(eval $$(call dep_include, $(objdir)/$(1)/,$$($$(bin)-obj))))

ifeq ($(V), 1)
$$(info os_obj_handler $(1) $(2) $(2)-dir $$($(2)-dir) $(2)-obj $$($(2)-obj))
$$(foreach bin,$$(all-execs) $$(all-libs),$$(warning DEBUG os_obj_handler current $$(bin)-obj $$($$(bin)-obj)))
$$(foreach bin,$$(all-execs) $$(all-libs),$$(warning DEBUG os_obj_handler global $(1) $(2) $$($$(bin)-$(2)-obj)))
endif

endef


#
# Subdirectory binaries handler
# $(1): name of component (1st-level folder)
# $(2): name of subdir (2nd-level folder, e.g. linux, ...)
define os_bins_handler
execs:=
libs:=

include $(1)/$(2)/Makefile

# Collect object list from each subdirectory
all-execs+=$$(execs)
all-libs+=$$(libs)
endef


#
# Subdirectory binaries handler
# $(1): name of component (1st-level folder)
define all_bins_subdir_handler
os_subdirs:=
execs:=
libs:=
archives:=

include $(1)/Makefile

# Collect object list from each subdirectory
all-execs+=$$(execs)
all-libs+=$$(libs)
all-archives+=$$(addprefix $(1)/,$$(archives))
$$(foreach dir,$$(filter $(target_os),$$(os_subdirs)),$$(eval $$(call os_bins_handler,$(1),$$(dir))))

endef



#
# Subdirectory object handler
# $(1): name of component (1st-level folder)
define subdir_handler
os_subdirs:=

$$(foreach bin,$$(all-execs) $$(all-libs) $$(_all-archives),$$(eval $$(bin)-obj:=))
$$(foreach bin,$$(all-execs) $$(all-libs) $$(_all-archives),$$(eval $$(bin)-ar:=))

include $(1)/Makefile

$(shell mkdir -p $(objdir)/$(1))
$(shell mkdir -p $(arch_objdir)/$(1))

# Collect object/archive lists from each os-independent subdirectory
# Collect object list per executable/library
$$(if $$(filter-out $(1), $(target_os)), $$(foreach bin,$$(all-execs) $$(all-libs),$$(eval $$(bin)-common-obj+=$$(addprefix $(objdir)/$(1)/,$$($$(bin)-obj)))))

# Collect archive list per executable/library
$$(if $$(filter-out $(1), $(target_os)), $$(foreach bin,$$(all-execs) $$(all-libs),$$(eval $$(bin)-common-ar+=$$(addprefix $(arch_objdir)/$(1)/,$$($$(bin)-ar)))))

# Collect object list per archive
$$(if $$(filter-out $(1), $(target_os)), $$(foreach bin,$$(_all-archives),$$(eval $$(bin)-common-obj+=$$(addprefix $(arch_objdir)/$(1)/,$$($$(bin)-obj)))))

$$(foreach bin,$$(all-execs) $$(all-libs),$$(eval all-obj+=$$(addprefix $(objdir)/$(1)/,$$($$(bin)-obj))))
$$(foreach bin,$$(_all-archives),$$(eval all-obj+=$$(addprefix $(arch_objdir)/$(1)/,$$($$(bin)-obj))))

$$(foreach bin,$$(all-execs) $$(all-libs),$$(eval $$(call dep_include, $(objdir)/$(1)/,$$($$(bin)-obj))))
$$(foreach bin,$$(_all-archives),$$(eval $$(call dep_include, $(arch_objdir)/$(1)/,$$($$(bin)-obj))))

# Os specific code
$$(foreach dir,$$(filter $(target_os),$$(os_subdirs)),$$(eval $$(call os_obj_handler,$(1)/$$(dir),$$(dir))))
$$(if $$(filter $(1), $(target_os)),$$(eval $$(call os_obj_handler,$(1),$(1))))

ifeq ($(V), 1)
$$(info subdir_handler $(1) $$(all-bins) $$(bins))
$$(foreach bin,$$(all-execs) $$(all-libs),$$(warning DEBUG subdir_handler current $$(bin)-obj $$($$(bin)-obj)))
$$(foreach bin,$$(all-execs) $$(all-libs),$$(warning DEBUG subdir_handler global $(1) $$($$(bin)-common-obj)))
endif

# Per target variables
$(objdir)/$(1)/%.o $(arch_objdir)/$(1)/%.o: CFLAGS+=$$($(1)_CFLAGS) -D'_COMPONENT_=$(1)_' -D'_COMPONENT_STR_="$(1)"' -D'_COMPONENT_ID_=$(1)_COMPONENT_ID' -include $(1)/config.h

$(objdir)/$(1)/%.o: CFLAGS+= -include $(objdir)/autoconf.h

quiet_cmd_cc = CC $$@
      cmd_cc = $(CC) -MD -MQ $$@ $$(CFLAGS) -c $$< -o $$@
      cmd_objcopy = $(OBJCOPY) $$@ --rename-section .text=.text.$$(2) --rename-section .data=.data.$$(2) --rename-section .bss=.bss.$$(2)

$(arch_objdir)/$(1)/%.o:$(srcdir)/$(1)/%.c
	$$(call cmd,cc)
	$$(call cmd,objcopy,$(1))

$(objdir)/$(1)/%.o:$(srcdir)/$(1)/%.c $(objdir)/autoconf.h
	$$(call cmd,cc)
	$$(call cmd,objcopy,$(1))

endef

include $(target_os)/rules.mk
