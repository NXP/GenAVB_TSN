var group__aem =
[
    [ "aecp_pdu", "group__aem.html#structaecp__pdu", [
      [ "entity_id", "group__aem.html#a26c626daa5962edfb212afabd7f48b79", null ],
      [ "controller_entity_id", "group__aem.html#a63b4b597840f92816e511c30ef4dfb66", null ],
      [ "sequence_id", "group__aem.html#a9e38b922a840a8cd7ecad5f75ba09454", null ]
    ] ],
    [ "aecp_aem_pdu", "group__aem.html#structaecp__aem__pdu", [
      [ "entity_id", "group__aem.html#a985653c15ea71b51a25a407f0f74919d", null ],
      [ "controller_entity_id", "group__aem.html#ac5267fdb2c2434519508c910c8ab1d58", null ],
      [ "sequence_id", "group__aem.html#af3b6c87993b9536ec6f16cffe4d5e995", null ],
      [ "u_command_type", "group__aem.html#a2453ca57616674f45f6e32646d58ab74", null ]
    ] ],
    [ "aecp_aem_read_desc_cmd_pdu", "group__aem.html#structaecp__aem__read__desc__cmd__pdu", [
      [ "configuration_index", "group__aem.html#a25a2d606a5ab34ab79f7f73972490553", null ],
      [ "reserved", "group__aem.html#a791fec4203427b9dcf682c7fbfc761b5", null ],
      [ "descriptor_type", "group__aem.html#a94a9b628d9c18c05124ed2e7910556c4", null ],
      [ "descriptor_index", "group__aem.html#adfc62913231bb48cc61f03dffa78ad8f", null ]
    ] ],
    [ "aecp_aem_read_desc_rsp_pdu", "group__aem.html#structaecp__aem__read__desc__rsp__pdu", [
      [ "configuration_index", "group__aem.html#a4a3c9fe6566a64c639d7d8c00486a3f9", null ],
      [ "reserved", "group__aem.html#a6c62a5c89512fb446ddcb6511ddd757c", null ]
    ] ],
    [ "aecp_aem_acquire_entity_pdu", "group__aem.html#structaecp__aem__acquire__entity__pdu", [
      [ "flags", "group__aem.html#a1df874e286e0023f731be615296cce2d", null ],
      [ "owner_id", "group__aem.html#a3e457423916c92c261d758d4ab8a8936", null ],
      [ "descriptor_type", "group__aem.html#a21f63b747eefc97927194b0a579f0408", null ],
      [ "descriptor_index", "group__aem.html#add7473b8849c5bf1ba6642bf6ce4b5da", null ]
    ] ],
    [ "aecp_aem_start_streaming_cmd_pdu", "group__aem.html#structaecp__aem__start__streaming__cmd__pdu", [
      [ "descriptor_type", "group__aem.html#a0b5e8a853ebdfe59d3343b122887ccea", null ],
      [ "descriptor_index", "group__aem.html#ac5f52946d5f84817f194aa22dc3b87fa", null ]
    ] ],
    [ "aecp_aem_stop_streaming_cmd_pdu", "group__aem.html#structaecp__aem__stop__streaming__cmd__pdu", [
      [ "descriptor_type", "group__aem.html#a43b7e868a8bf7749b10bda717cca9616", null ],
      [ "descriptor_index", "group__aem.html#a076c3e5dc77db4a1ee7bf2e910d9da74", null ]
    ] ],
    [ "aecp_aem_set_get_control_pdu", "group__aem.html#structaecp__aem__set__get__control__pdu", [
      [ "descriptor_type", "group__aem.html#affbeb9599914bd7a99fa23e712d6ad7b", null ],
      [ "descriptor_index", "group__aem.html#a9e7313cbe48f8a5a6af93ad5112efd73", null ]
    ] ],
    [ "aecp_aem_get_counters_cmd_pdu", "group__aem.html#structaecp__aem__get__counters__cmd__pdu", [
      [ "descriptor_type", "group__aem.html#a5533a189b7d27c8905b11b6244426c0a", null ],
      [ "descriptor_index", "group__aem.html#a469ce8e8517e7343ed47b6525d4e40c8", null ]
    ] ],
    [ "aecp_aem_get_counters_rsp_pdu", "group__aem.html#structaecp__aem__get__counters__rsp__pdu", [
      [ "descriptor_type", "group__aem.html#a14b2227481c5a48fa3d49060b797059d", null ],
      [ "descriptor_index", "group__aem.html#a372edb4fc7fb8cd0208f373528ea2a3c", null ],
      [ "counters_valid", "group__aem.html#a83b370793a4934f0e16da0ddab616d83", null ],
      [ "counters_block", "group__aem.html#a693b70b6c9032110f09f6c4003ca647c", null ]
    ] ],
    [ "AECP_AEM_GET_CMD_TYPE", "group__aem.html#ga74b331ed6e461ef64245eef8ec19d862", null ],
    [ "AECP_AEM_GET_U", "group__aem.html#gafee9b710526c80d58fc2722cc58bf36c", null ],
    [ "AECP_AEM_SET_U_CMD_TYPE", "group__aem.html#ga79d25e40e209e19a3cd3b747c15bf34c", null ],
    [ "AECP_AEM_ACQUIRE_PERSISTENT", "group__aem.html#ga184d72e48c3e64da9807c6fab98f3a53", null ],
    [ "AECP_AEM_ACQUIRE_RELEASE", "group__aem.html#gade22f6ab6f1e5948679bdb96bc327198", null ],
    [ "aecp_message_type_t", "group__aem.html#ga1a819286f45ec4d69fb995627756d840", [
      [ "AECP_AEM_COMMAND", "group__aem.html#gga1a819286f45ec4d69fb995627756d840a7698d47e4fbc8d3e54b8bd2648c5c1a2", null ],
      [ "AECP_AEM_RESPONSE", "group__aem.html#gga1a819286f45ec4d69fb995627756d840ac45526acdf58341f0ce63e36a7f0c09d", null ]
    ] ],
    [ "aecp_aem_command_type_t", "group__aem.html#gae99a25757035d67d1e38e288f857ff41", [
      [ "AECP_AEM_CMD_ACQUIRE_ENTITY", "group__aem.html#ggae99a25757035d67d1e38e288f857ff41aec8db2c2f1546a02a7ad30c42740a992", null ],
      [ "AECP_AEM_CMD_READ_DESCRIPTOR", "group__aem.html#ggae99a25757035d67d1e38e288f857ff41af118b9c06a4fb73e7d1fa85406478571", null ],
      [ "AECP_AEM_CMD_SET_CONTROL", "group__aem.html#ggae99a25757035d67d1e38e288f857ff41ad2319757d74d77aad53043e893c96f83", null ],
      [ "AECP_AEM_CMD_GET_CONTROL", "group__aem.html#ggae99a25757035d67d1e38e288f857ff41aae582b03e74b1c7cc03b18a577212ae8", null ],
      [ "AECP_AEM_CMD_START_STREAMING", "group__aem.html#ggae99a25757035d67d1e38e288f857ff41ab6fc010d780e35c79f1c36756239e4be", null ],
      [ "AECP_AEM_CMD_STOP_STREAMING", "group__aem.html#ggae99a25757035d67d1e38e288f857ff41a143f813047089bae80d7e5a1c17e82db", null ],
      [ "AECP_AEM_CMD_REGISTER_UNSOLICITED_NOTIFICATION", "group__aem.html#ggae99a25757035d67d1e38e288f857ff41ac1c176e117445da5a74c0245c0b670a3", null ],
      [ "AECP_AEM_CMD_DEREGISTER_UNSOLICITED_NOTIFICATION", "group__aem.html#ggae99a25757035d67d1e38e288f857ff41ac6ef530440a0e54b7a56ecea4b5412a6", null ]
    ] ],
    [ "aecp_status_t", "group__aem.html#ga28c8fb55a44ef0fa9474dadb507180b7", [
      [ "AECP_SUCCESS", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7aa4a36d9de6e4983e5125c766baab10df", null ],
      [ "AECP_AEM_SUCCESS", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7a68d0c6724ca69737507f1f7b07afd4c3", null ],
      [ "AECP_NOT_IMPLEMENTED", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7a7028507a64165b2561e1e0196399aa61", null ],
      [ "AECP_AEM_NOT_IMPLEMENTED", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7ace7d625da977bcf2046cdca39d4472f5", null ],
      [ "AECP_AEM_NO_SUCH_DESCRIPTOR", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7a83173bb7eb99789af9377b7e3b4df524", null ],
      [ "AECP_AEM_ENTITY_LOCKED", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7afd342da8eaa33eda857e5be979837424", null ],
      [ "AECP_AEM_ENTITY_ACQUIRED", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7adb5df93cdcf6f9f99431cda5de0204d4", null ],
      [ "AECP_AEM_NOT_AUTHENTICATED", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7a430eaac0ffe419ea5c52d30577cf07ff", null ],
      [ "AECP_AEM_AUTHENTICATION_DISABLED", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7ae6e9c4da59d123581cbb018fd22067a2", null ],
      [ "AECP_AEM_BAD_ARGUMENTS", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7a37a901df96893ae253bbeffe686f7618", null ],
      [ "AECP_AEM_NO_RESOURCES", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7a57307e874f86360517bd5901fdd2da96", null ],
      [ "AECP_AEM_IN_PROGRESS", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7a46ce58104c1d800962ed91c0d0f0373b", null ],
      [ "AECP_AEM_ENTITY_MISBEHAVING", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7aff0597a79dab385decda7d2a89937700", null ],
      [ "AECP_AEM_NOT_SUPPORTED", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7a843881802a73fb831f7b9549773e053a", null ],
      [ "AECP_AEM_STREAM_IS_RUNNING", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7ac5547417f2d51ff4973b01f1e5e5f517", null ],
      [ "AECP_AEM_TIMEOUT", "group__aem.html#gga28c8fb55a44ef0fa9474dadb507180b7ab6a2dee78f29ebfea79c149a160300ec", null ]
    ] ]
];