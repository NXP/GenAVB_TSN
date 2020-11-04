var group__socket =
[
    [ "net_address", "group__socket.html#structnet__address", [
      [ "ptype", "group__socket.html#a12fd7a3b7be8986404e7ed9f26520ae9", null ],
      [ "port", "group__socket.html#ad48e660c03cf1f4eb7d8838960dac91e", null ],
      [ "vlan_id", "group__socket.html#a297496ac20d1eb7a6a59c9ab68be3584", null ],
      [ "priority", "group__socket.html#acd9e23db3f7f377f72e97554a358a9e7", null ],
      [ "u", "group__socket.html#ac6c4296b674f971c348c2bc492a95cba", null ]
    ] ],
    [ "net_address.u", "group__socket.html#unionnet__address_8u", [
      [ "avtp", "group__socket.html#ab7f9406e198ff6d04aa24f68ed80a132", null ],
      [ "ptp", "group__socket.html#abe19d835ae8962460c01703fd05ad9ad", null ],
      [ "ipv4", "group__socket.html#a0485728ba5ed6951c7e858af6c1af7c3", null ],
      [ "ipv6", "group__socket.html#acc314cbc6ae71c0724390eb450bb969d", null ],
      [ "l2", "group__socket.html#abec25675775e9e0a0d783a5018b463e3", null ]
    ] ],
    [ "net_address.u.ptp", "group__socket.html#structnet__address_8u_8ptp", [
      [ "version", "group__socket.html#a2af72f100c356273d46284f6fd1dfc08", null ]
    ] ],
    [ "net_address.u.ipv4", "group__socket.html#structnet__address_8u_8ipv4", [
      [ "proto", "group__socket.html#a349c85aae1d71151c001702f17a2b5f0", null ],
      [ "saddr", "group__socket.html#a2c80f014204a9f5f63f4cd28bc0b6a93", null ],
      [ "daddr", "group__socket.html#a368f734539fba5966cd1c34b4afc8919", null ],
      [ "sport", "group__socket.html#a3823552b7a2b839259a831e3b7b349a3", null ],
      [ "dport", "group__socket.html#afc2e3570ae37b85374628155fefc806c", null ],
      [ "l5_proto", "group__socket.html#a35adb05bd6faeb0682c4504ad41695c4", null ],
      [ "u", "group__socket.html#a7b774effe4a349c6dd82ad4f4f21d34c", null ]
    ] ],
    [ "net_address.u.ipv4.u", "group__socket.html#unionnet__address_8u_8ipv4_8u", [
      [ "rtp", "group__socket.html#ac766e112de79c244fdaed0ed34ca5e12", null ],
      [ "rtcp", "group__socket.html#a975bfd49137cb34ef651cdb98c84995d", null ]
    ] ],
    [ "net_address.u.ipv4.u.rtp", "group__socket.html#structnet__address_8u_8ipv4_8u_8rtp", [
      [ "sr_class", "group__socket.html#ab03cd0f0730f6e0e21e215a5e988eded", null ],
      [ "stream_id", "group__socket.html#a166aef4d085109e4483f03666db05668", null ],
      [ "pt", "group__socket.html#afc9fdf084e290f26a270390dc49061a2", null ],
      [ "ssrc", "group__socket.html#a8ffb72be413ff21587b04f86a25926a0", null ]
    ] ],
    [ "net_address.u.ipv4.u.rtcp", "group__socket.html#structnet__address_8u_8ipv4_8u_8rtcp", [
      [ "pt", "group__socket.html#afc9fdf084e290f26a270390dc49061a2", null ],
      [ "ssrc", "group__socket.html#a8ffb72be413ff21587b04f86a25926a0", null ]
    ] ],
    [ "net_address.u.ipv6", "group__socket.html#structnet__address_8u_8ipv6", [
      [ "proto", "group__socket.html#a349c85aae1d71151c001702f17a2b5f0", null ],
      [ "saddr", "group__socket.html#a7a37f804a0fd43e6e2933b50f36d7950", null ],
      [ "daddr", "group__socket.html#aaf8f6f1044ecf987eb5880905293a968", null ],
      [ "sport", "group__socket.html#a3823552b7a2b839259a831e3b7b349a3", null ],
      [ "dport", "group__socket.html#afc2e3570ae37b85374628155fefc806c", null ],
      [ "l5_proto", "group__socket.html#a35adb05bd6faeb0682c4504ad41695c4", null ],
      [ "u", "group__socket.html#a7b774effe4a349c6dd82ad4f4f21d34c", null ]
    ] ],
    [ "net_address.u.ipv6.u", "group__socket.html#unionnet__address_8u_8ipv6_8u", [
      [ "rtp", "group__socket.html#ac766e112de79c244fdaed0ed34ca5e12", null ],
      [ "rtcp", "group__socket.html#a975bfd49137cb34ef651cdb98c84995d", null ]
    ] ],
    [ "net_address.u.ipv6.u.rtp", "group__socket.html#structnet__address_8u_8ipv6_8u_8rtp", [
      [ "sr_class", "group__socket.html#ab03cd0f0730f6e0e21e215a5e988eded", null ],
      [ "stream_id", "group__socket.html#a166aef4d085109e4483f03666db05668", null ],
      [ "pt", "group__socket.html#afc9fdf084e290f26a270390dc49061a2", null ],
      [ "ssrc", "group__socket.html#a8ffb72be413ff21587b04f86a25926a0", null ]
    ] ],
    [ "net_address.u.ipv6.u.rtcp", "group__socket.html#structnet__address_8u_8ipv6_8u_8rtcp", [
      [ "pt", "group__socket.html#afc9fdf084e290f26a270390dc49061a2", null ],
      [ "ssrc", "group__socket.html#a8ffb72be413ff21587b04f86a25926a0", null ]
    ] ],
    [ "net_address.u.l2", "group__socket.html#structnet__address_8u_8l2", [
      [ "protocol", "group__socket.html#a81788ba0d7d02d81c063dbca621ba11b", null ],
      [ "dst_mac", "group__socket.html#a80d68faf6cba0d0786852371c84c77a4", null ]
    ] ],
    [ "avtp_address.u.avtp", "group__socket.html#structnet__address_1_1avtp__address_8u_8avtp", [
      [ "subtype", "group__socket.html#a2e282b0d23d6ec55185caeb87b41c0e0", null ],
      [ "sr_class", "group__socket.html#ab03cd0f0730f6e0e21e215a5e988eded", null ],
      [ "stream_id", "group__socket.html#a166aef4d085109e4483f03666db05668", null ]
    ] ],
    [ "genavb_socket_rx_params", "group__socket.html#structgenavb__socket__rx__params", [
      [ "addr", "group__socket.html#aaf19e5b729aef38133e7bf03623bb4ab", null ]
    ] ],
    [ "genavb_socket_tx_params", "group__socket.html#structgenavb__socket__tx__params", [
      [ "addr", "group__socket.html#adae8d698e6bd65b8ea66e0ae25311b72", null ]
    ] ],
    [ "VLAN_VID_DEFAULT", "group__socket.html#ga6874980f64ff68cadfbb5abcd499b0c6", null ],
    [ "VLAN_VID_NONE", "group__socket.html#ga74cbe20746c49877523d1e9f241d13cc", null ],
    [ "NET_PTYPE", "group__socket.html#ga54a99f7b450267929505a25dafd7e29f", [
      [ "PTYPE_OTHER", "group__socket.html#gga54a99f7b450267929505a25dafd7e29fa617ea496cc731e1aa57b6b13b69dbd82", null ],
      [ "PTYPE_L2", "group__socket.html#gga54a99f7b450267929505a25dafd7e29fa037a73b1ddbdd1bb45d3271adb37537a", null ]
    ] ],
    [ "genavb_sock_f_t", "group__socket.html#gab5adeaf4eacc2cc7e7f649cf6456892b", [
      [ "GENAVB_SOCKF_NONBLOCK", "group__socket.html#ggab5adeaf4eacc2cc7e7f649cf6456892ba9b5002d8fc384b98c87fa827be8bbbf5", null ],
      [ "GENAVB_SOCKF_ZEROCOPY", "group__socket.html#ggab5adeaf4eacc2cc7e7f649cf6456892ba10dad4227260d40c55fbbffee2f249e7", null ],
      [ "GENAVB_SOCKF_RAW", "group__socket.html#ggab5adeaf4eacc2cc7e7f649cf6456892ba8c100f50a99899d6b1ecc9dd76179ff7", null ]
    ] ],
    [ "genavb_socket_rx_fd", "group__socket.html#ga6e528bea165f730f74efe126b7f6036e", null ],
    [ "genavb_socket_tx_fd", "group__socket.html#ga925100e75d1b9f4a65463156be02388d", null ],
    [ "genavb_socket_rx_open", "group__socket.html#ga993d76b4a4ce46cb8a9f8d4846b79440", null ],
    [ "genavb_socket_tx_open", "group__socket.html#gac3d1a2a1cbc3198f48c011d98da2e696", null ],
    [ "genavb_socket_tx", "group__socket.html#gad80ab211fb3f2a0c807926368d41d63b", null ],
    [ "genavb_socket_rx", "group__socket.html#ga9a53948cb83c2f8836022cb158ffa64c", null ],
    [ "genavb_socket_rx_close", "group__socket.html#ga0c98ea9974edededab7497f47f31c348", null ],
    [ "genavb_socket_tx_close", "group__socket.html#ga7397b58d8aa3c521d00513c02ecac215", null ]
];