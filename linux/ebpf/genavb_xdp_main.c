/* Copyright 2019-2021 NXP
 * NXP Confidential. This software is owned or controlled by NXP and may only be used strictly in
 * accordance with the applicable license terms.  By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that
 * you have read, and that you agree to comply with and are bound by, such license terms.  If you
 * do not agree to be bound by the applicable license terms, then you may not retain, install,
 * activate or otherwise use the software.
 */

#include "genavb/net_types.h"
#include "ebpf.h"
#include "genavb/ether.h"

struct bpf_elf_map __section("maps") genavb_xdpkey = {
	.type		= BPF_MAP_TYPE_HASH,
	.size_key	= sizeof(struct genavb_xdp_key),
	.size_value	= sizeof(uint32_t),
	.pinning	= PIN_GLOBAL_NS,
	.max_elem	= MAX_SOCKETS,
};

struct bpf_elf_map __section("maps") genavb_xskmap = {
	.type		= BPF_MAP_TYPE_XSKMAP,
	.size_key	= sizeof(uint32_t),
	.size_value	= sizeof(uint32_t),
	.pinning	= PIN_GLOBAL_NS,
	.max_elem	= MAX_SOCKETS,
};

__section("prog")
static int genavb_xdp_prog(struct xdp_ctx *ctx)
{
	void *data = (void *)(long)ctx->data;
	void *data_end = (void *)(long)ctx->data_end;
	struct eth_hdr *eth = (struct eth_hdr *)data;
	struct genavb_xdp_key key = {};
	uint64_t xsk;
	uintptr_t *pxsk;

	if ((void *)(eth + 1) > data_end)
		return XDP_PASS;

	key.protocol = eth->type;
	__builtin_memcpy(key.dst_mac, eth->dst, 6);

	if (key.protocol == htons(ETHERTYPE_VLAN)) {
		struct vlanhdr *vlan = (struct vlanhdr *)(eth + 1);

		if ((void *)(vlan + 1) > data_end)
			return XDP_PASS;

		key.protocol = vlan->type;
		key.vlan_id = htons(VLAN_VID(vlan));
	} else {
		key.vlan_id = VLAN_VID_NONE;
	}

	pxsk = bpf_map_lookup_elem(&genavb_xdpkey, &key);
	if (!pxsk)
		return XDP_PASS;
	xsk = *pxsk;
	return bpf_redirect_map(&genavb_xskmap, (void *)xsk, XDP_PASS);
}

char _license[] __section("license") = "Proprietary";
