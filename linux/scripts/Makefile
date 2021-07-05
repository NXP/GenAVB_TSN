ifndef BIN_DIR
$(error BIN_DIR is not set)
endif

ifndef INITSCRIPT_DIR
$(error INITSCRIPT_DIR is not set)
endif

define avb_script_install
	install -D avb.sh $(BIN_DIR)/avb.sh; sed -i '/PACKAGE=/cPACKAGE=$1' $(BIN_DIR)/avb.sh
	install -D fgptp.sh $(BIN_DIR)/fgptp.sh; sed -i '/PACKAGE=/cPACKAGE=$1' $(BIN_DIR)/fgptp.sh
endef

endpoint_tsn-install endpoint_avb-install: avb.sh fgptp.sh init.sh
	$(call avb_script_install,ENDPOINT)
	install -D init.sh $(INITSCRIPT_DIR)

endpoint_gptp_only-install: avb.sh fgptp.sh init.sh
	$(call avb_script_install,ENDPOINT_GPTP)
	install -D init.sh $(INITSCRIPT_DIR)

endpoint_1733-install: 1733.sh
	install -D 1733.sh $(BIN_DIR)/1733.sh

bridge-install: avb.sh fgptp.sh init.sh
	$(call avb_script_install,BRIDGE)
	install -D init.sh $(INITSCRIPT_DIR)

hybrid-install: avb.sh fgptp.sh
	$(call avb_script_install,HYBRID)
	install -D init.sh $(INITSCRIPT_DIR)
