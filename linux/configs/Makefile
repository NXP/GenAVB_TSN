ifndef CONFIG_DIR
$(error CONFIG_DIR is not set)
endif

system_cfg-install:
	install -d $(CONFIG_DIR)
	if [ -e $(target)_$(config)_system.cfg ]; then install -m ug+rw $(target)_$(config)_system.cfg $(CONFIG_DIR)/system.cfg; fi

define srp_cfg_install
	install -d $(CONFIG_DIR)
	if [ -e $(target)_$(config)_srp$(1).cfg ]; then \
		install -m ug+rw $(target)_$(config)_srp$(1).cfg $(CONFIG_DIR)/srp$(1).cfg; \
	else \
		install -m ug+rw srp$(1).cfg $(CONFIG_DIR)/; \
	fi
endef

endpoint_avb-install: system_cfg-install config_avb apps*.cfg genavb*.cfg fgptp.cfg* *srp.cfg
	install -d $(CONFIG_DIR)
	install -m ug+rw config_avb $(CONFIG_DIR)/config
	install -m ug+rw apps*.cfg $(CONFIG_DIR)/
	install -m ug+rw genavb*.cfg $(CONFIG_DIR)/
	install -m ug+rw fgptp.cfg* $(CONFIG_DIR)/
	$(call srp_cfg_install,)

endpoint_tsn-install: system_cfg-install config_tsn apps-tsn*.cfg genavb-tsn*.cfg fgptp.cfg* *srp.cfg
	install -d $(CONFIG_DIR)
	install -m ug+rw config_tsn $(CONFIG_DIR)/config
	install -m ug+rw apps-tsn*.cfg $(CONFIG_DIR)/
	install -m ug+rw genavb-tsn*.cfg $(CONFIG_DIR)/
	install -m ug+rw fgptp.cfg* $(CONFIG_DIR)/
	$(call srp_cfg_install,)

endpoint_gptp_only-install: system_cfg-install config_tsn fgptp.cfg*
	install -d $(CONFIG_DIR)
	install -m ug+rw config_tsn $(CONFIG_DIR)/config
	install -m ug+rw fgptp.cfg* $(CONFIG_DIR)/

bridge-install: system_cfg-install config_tsn genavb-tsn.cfg fgptp-br.cfg* *srp-br.cfg
	install -d $(CONFIG_DIR)
	install -m ug+rw config_tsn $(CONFIG_DIR)/config
	install -m ug+rw genavb-tsn.cfg $(CONFIG_DIR)/
	install -m ug+rw fgptp-br.cfg* $(CONFIG_DIR)/
	$(call srp_cfg_install,-br)

hybrid-install: endpoint_avb-install fgptp-br.cfg* *srp-br.cfg
	install -m ug+rw fgptp-br.cfg* $(CONFIG_DIR)/
	$(call srp_cfg_install,-br)

endpoint_1733-install:
