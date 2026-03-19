/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific clock and time service implementation
 @details
*/

#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <linux/ptp_clock.h>
#include <sys/ioctl.h>

#include <genavb/helpers.h>

#include "common/log.h"
#include "common/clock.h"
#include "os/clock.h"

#include "clock.h"
#include "net_logical_port.h"
#include "headers_extensions.h"

#ifndef ADJ_SETOFFSET
#define ADJ_SETOFFSET 0x0100
#endif

#define CLOCKFD 3
#define FD_TO_CLOCKID(fd)	((~(clockid_t) (fd) << 3) | CLOCKFD)
#define CLOCKID_TO_FD(clk)	((unsigned int) ~((clk) >> 3))

#define OS_CLOCK_SHARE_SAME_PARENT(c1, c2) \
			((c1)->parent_id == (c2)->parent_id)

#define for_each_sw_clock_with_same_parent(clock, sibling) \
	for (int __i = 0; __i < OS_CLOCK_MAX; __i++) \
		/* Skip ourself, non-software clocks and sw clock with different parent */ \
		if ((((sibling) = &os_clock[__i]) != (clock)) && \
			((sibling)->type == CLOCK_TYPE_SW) && \
			((sibling)->parent_id == (clock)->parent_id))

#define OS_CLOCK_IS_PHC(c) \
			(((c)->type == CLOCK_TYPE_PHC))

#define OS_CLOCK_IS_VIRTUAL_PHC(c) \
			(OS_CLOCK_IS_PHC(c) && ((c)->flags & OS_CLOCK_FLAGS_PHC_VIRTUAL))

#define OS_CLOCK_IS_PHYSICAL_PHC(c) \
			(OS_CLOCK_IS_PHC(c) && !((c)->flags & OS_CLOCK_FLAGS_PHC_VIRTUAL))

#define OS_CLOCK_SUPPORTS_REMAP(c) \
			((c)->flags & OS_CLOCK_FLAGS_SUPPORTS_REMAP)

static struct os_clock os_clock[OS_CLOCK_MAX] = {
	[OS_CLOCK_SYSTEM_MONOTONIC] = {
		.type = CLOCK_TYPE_SYSTEM,
		.id = CLOCK_MONOTONIC_RAW,
		.enabled = true,
		.flags = 0,
	},

	/*
	 * All other (phc) clocks configured at init time:
	 * see os_clock_config_init()
	 */
};

static os_clock_id_t clk_id_to_clk_id[OS_CLOCK_MAX] = {
	[OS_CLOCK_SYSTEM_MONOTONIC] = OS_CLOCK_SYSTEM_MONOTONIC,
	[OS_CLOCK_SYSTEM_MONOTONIC_COARSE] = OS_CLOCK_SYSTEM_MONOTONIC_COARSE,
	[OS_CLOCK_MEDIA_HW_0] = OS_CLOCK_MEDIA_HW_0,
	[OS_CLOCK_MEDIA_HW_1] = OS_CLOCK_MEDIA_HW_1,
	[OS_CLOCK_MEDIA_REC_0] = OS_CLOCK_MEDIA_REC_0,
	[OS_CLOCK_MEDIA_REC_1] = OS_CLOCK_MEDIA_REC_1,
	[OS_CLOCK_MEDIA_PTP_0] = OS_CLOCK_MEDIA_PTP_0,
	[OS_CLOCK_MEDIA_PTP_1] = OS_CLOCK_MEDIA_PTP_1,
	[OS_CLOCK_GPTP_EP_0_0] = OS_CLOCK_GPTP_EP_0_0,
	[OS_CLOCK_GPTP_EP_0_1] = OS_CLOCK_GPTP_EP_0_1,
	[OS_CLOCK_GPTP_EP_1_0] = OS_CLOCK_GPTP_EP_1_0,
	[OS_CLOCK_GPTP_EP_1_1] = OS_CLOCK_GPTP_EP_1_1,
	[OS_CLOCK_GPTP_BR_0_0] = OS_CLOCK_GPTP_BR_0_0,
	[OS_CLOCK_GPTP_BR_0_1] = OS_CLOCK_GPTP_BR_0_1,
	[OS_CLOCK_LOCAL_EP_0] = OS_CLOCK_LOCAL_EP_0,
	[OS_CLOCK_LOCAL_EP_1] = OS_CLOCK_LOCAL_EP_1,
	[OS_CLOCK_LOCAL_BR_0] = OS_CLOCK_LOCAL_BR_0,
	[OS_CLOCK_SYSTEM_MONOTONIC_1] = OS_CLOCK_SYSTEM_MONOTONIC_1,
	[OS_CLOCK_AVTP_MEDIA_0] = OS_CLOCK_GPTP_EP_0_0, /* AVTP media clock 0 is by default mapped to interface 0 target gPTP clock 0 */
	[OS_CLOCK_AVTP_MEDIA_1] = OS_CLOCK_GPTP_EP_1_0, /* AVTP media clock 1 is by default mapped to interface 1 target gPTP clock 1 */
};

/*
 * Mutex lock to protect concurrent access to entire clock layer
 *
 * Offset/frequency computation must be done atomically to avoid bad (old) values
 * when calling into ->gettime_sw(), ->setfreq(), ->setoffset() and so on...
 */
static pthread_mutex_t os_clock_mutex;

static int clock_gettime64_hw(struct os_clock *c, u64 *ns);

os_clock_id_t logical_port_to_local_clock(unsigned int port_id)
{
	if (!logical_port_valid(port_id))
		goto err;

	if (logical_port_is_endpoint(port_id))
		return OS_CLOCK_LOCAL_EP_0 + logical_port_endpoint_id(port_id);
	else
		return OS_CLOCK_LOCAL_BR_0 + logical_port_bridge_id(port_id);

err:
	return OS_CLOCK_MAX;
}

os_clock_id_t logical_port_to_gptp_clock(unsigned int port_id, unsigned int domain)
{
	if (!logical_port_valid(port_id) || domain >= CFG_MAX_GPTP_DOMAINS)
		goto err;

	if (logical_port_is_endpoint(port_id))
		return OS_CLOCK_GPTP_EP_0_0 + logical_port_endpoint_id(port_id) * CFG_MAX_GPTP_DOMAINS + domain;
	else
		return OS_CLOCK_GPTP_BR_0_0 + logical_port_bridge_id(port_id) * CFG_MAX_GPTP_DOMAINS + domain;

err:
	return OS_CLOCK_MAX;
}

os_clock_id_t logical_port_to_avtp_clock(unsigned int port_id)
{
	if (!logical_port_valid(port_id) || !logical_port_is_endpoint(port_id))
		goto err;

	return OS_CLOCK_AVTP_MEDIA_0 + port_id;

err:
	return OS_CLOCK_MAX;
}

os_clock_id_t clock_id(os_clock_id_t clk_id)
{
	return clk_id_to_clk_id[clk_id];
}

static bool clock_id_supports_remap(os_clock_id_t clk_id)
{
	struct os_clock *c;

	if (clk_id >= OS_CLOCK_MAX)
		goto invalid;

	c = &os_clock[clk_id];
	if (!OS_CLOCK_SUPPORTS_REMAP(c))
		goto invalid;

	return true;

invalid:
	return false;
}

#define TIME_BASE_UPDATE_TRESHOLD (2000000000ULL)

static inline void clock_time_base_update(struct os_clock *c, uint64_t ns, uint64_t ns_hw)
{
	uint64_t delta_hw = ns_hw - c->sw_clk.hw.t0;

	/*
	 * The below functions clock_time_from_hw/clock_time_to_hw overflow
	 * if the delta between current time and t0 is greater than ~4 seconds.
	 * In general t0 is updated when frequency is adjusted but if not it's
	 * done here.
	 */
	if (delta_hw > TIME_BASE_UPDATE_TRESHOLD) {
		c->sw_clk.sw.t0 = ns;
		c->sw_clk.hw.t0 = ns_hw;
	}
}

/*
 * Note:
 * - os_clock_mutex must be held before entering this function
 */
static inline uint64_t __clock_time_from_hw(struct os_clock *c, uint64_t ns_hw)
{
	uint64_t ns = ns_hw;

	if (c->type != CLOCK_TYPE_SW)
		goto pass_through;

	if (c->sw_clk.sw.mul) {
		if (ns_hw > c->sw_clk.hw.t0)
			ns = c->sw_clk.sw.t0 + (((ns_hw - c->sw_clk.hw.t0) * c->sw_clk.sw.mul) >> c->sw_clk.sw.shift);
		else
			ns = c->sw_clk.sw.t0 - (((c->sw_clk.hw.t0 - ns_hw) * c->sw_clk.sw.mul) >> c->sw_clk.sw.shift);
	} else {
		ns = c->sw_clk.sw.t0 + (ns_hw - c->sw_clk.hw.t0);
	}

pass_through:
	return ns;
}

/*
 * Note:
 * - os_clock_mutex must be held before entering this function
 */
static uint64_t __clock_time_to_hw(struct os_clock *c, uint64_t ns)
{
	uint64_t ns_hw = ns;

	if (c->type != CLOCK_TYPE_SW)
		goto pass_through;

	if (c->sw_clk.sw.mul) {
		if (ns > c->sw_clk.sw.t0)
			ns_hw = c->sw_clk.hw.t0 + (((ns - c->sw_clk.sw.t0) * c->sw_clk.hw.mul) >> c->sw_clk.hw.shift);
		else
			ns_hw = c->sw_clk.hw.t0 - (((c->sw_clk.sw.t0 - ns) * c->sw_clk.hw.mul) >> c->sw_clk.hw.shift);
	} else {
		ns_hw = c->sw_clk.hw.t0 + (ns - c->sw_clk.sw.t0);
	}

pass_through:
	return ns_hw;
}

static int clock_gettime64_sw(struct os_clock *c, u64 *ns)
{
	int err;
	struct timespec now;
	uint64_t ns_hw;

	pthread_mutex_lock(&os_clock_mutex);

	err = clock_gettime(c->id, &now);
	if (err) {
		os_log(LOG_ERR, "clock(%p) clock_gettime failed: %s\n", c, strerror(errno));
		goto unlock;
	}

	ns_hw = (u64)now.tv_sec*NSECS_PER_SEC + now.tv_nsec;
	*ns = __clock_time_from_hw(c, ns_hw);

	if (c->sw_clk.sw.mul)
		clock_time_base_update(c, *ns, ns_hw);

unlock:
	pthread_mutex_unlock(&os_clock_mutex);

	return err;
}

static int clock_gettime32_sw(struct os_clock *c, u32 *ns)
{
	u64 val;
	int err;

	err = clock_gettime64_sw(c, &val);
	if (!err)
		*ns = (u32)val;

	return err;
}

int clock_setoffset_sw(struct os_clock *c, s64 offset)
{
	pthread_mutex_lock(&os_clock_mutex);

	c->sw_clk.sw.t0 += offset;

	pthread_mutex_unlock(&os_clock_mutex);

	return 0;
}

/*
 * Note:
 * - os_clock_mutex must be held before entering this function
 */
static void __clock_setfreq_sw(struct os_clock *c, int32_t ppb, uint64_t t0_hw)
{
	c->sw_clk.sw.t0 = __clock_time_from_hw(c, t0_hw);
	c->sw_clk.hw.t0 = t0_hw;

	if (ppb) {
		c->sw_clk.sw.shift = 32;
		c->sw_clk.sw.mul = ((1000000000ULL + ppb) << c->sw_clk.sw.shift) / 1000000000ULL;

		c->sw_clk.hw.shift = 32;
		c->sw_clk.hw.mul = (1000000000ULL << c->sw_clk.sw.shift) / (1000000000ULL + ppb);
	} else {
		c->sw_clk.hw.mul = 0;
		c->sw_clk.sw.mul = 0;
	}

	c->ppb = ppb;
}

static int clock_setfreq_sw(struct os_clock *c, int32_t ppb)
{
	uint64_t t0_hw;
	int ret = 0;

	pthread_mutex_lock(&os_clock_mutex);

	ret = clock_gettime64_hw(c, &t0_hw);
	if (ret)
		goto unlock;

	__clock_setfreq_sw(c, ppb + c->ppb_internal, t0_hw);

unlock:
	pthread_mutex_unlock(&os_clock_mutex);

	return ret;
}

static int clock_gettime32_hw(struct os_clock *c, u32 *ns)
{
	int err = 0;
	struct timespec now;

	err = clock_gettime(c->id, &now);
	if (err) {
		os_log(LOG_ERR, "clock(%p) clock_gettime failed: %s\n", c, strerror(errno));
		return err;
	}

	*ns = (u64)now.tv_sec*NSECS_PER_SEC + now.tv_nsec;

	return 0;
}

static int clock_gettime64_hw(struct os_clock *c, u64 *ns)
{
	int err = 0;
	struct timespec now;

	err = clock_gettime(c->id, &now);
	if (err) {
		os_log(LOG_ERR, "clock(%p) clock_gettime failed: %s\n", c, strerror(errno));
		return err;
	}

	*ns = (u64)now.tv_sec*NSECS_PER_SEC + now.tv_nsec;

	return 0;
}

static long ppb_to_scaled_ppm(int ppb)
{
	/*
	* The 'freq' field in the 'struct timex' is in parts per
	* million, but with a 16 bit binary fractional field.
	* Instead of calculating either one of
	*
	* scaled_ppm = (ppb / 1000) << 16 [1]
	* scaled_ppm = (ppb << 16) / 1000 [2]
	*
	* we simply use double precision math, in order to avoid the
	* truncation in [1] and the possible overflow in [2].
	*/
	return (long) (ppb * 65.536);
}

static int clock_setfreq_hw(struct os_clock *c, s32 ppb)
{
	struct timex t;
	struct os_clock *_c;
	uint64_t t0_hw;
	int ret = -1;
	s32 delta_ppb;

	memset(&t, 0, sizeof(t));

	t.modes = ADJ_FREQUENCY;
	t.freq = ppb_to_scaled_ppm(ppb);

	pthread_mutex_lock(&os_clock_mutex);

	if (clock_adjtime(c->id, &t) < 0) {
		os_log(LOG_ERR, "clock_id(0x%x) failed adjusting frequency\n", c->id);
		goto unlock;
	}

	os_log(LOG_DEBUG, "clock_id(0x%x) adjusted frequency by %d ppb\n", c->id, ppb);

	/* for all other clocks, with the same parent/clock device, adjust by -delta_ppb */
	delta_ppb = ppb - c->ppb;

	ret = clock_gettime64_hw(c, &t0_hw);
	if (ret) {
		os_log(LOG_ERR, "clock_id(0x%x) failed to get time\n", c->id);
		ret = -1;
		goto unlock;
	}

	for_each_sw_clock_with_same_parent(c, _c) {

		_c->ppb_internal -= delta_ppb;
		__clock_setfreq_sw(_c, _c->ppb - delta_ppb, t0_hw);

		os_log(LOG_DEBUG, "clock_id(0x%x) adjusted sw frequency by %d ppb\n",
				 _c->id, _c->ppb - delta_ppb);
	}

	c->ppb = ppb;
unlock:
	pthread_mutex_unlock(&os_clock_mutex);

	return ret;
}

static int clock_setoffset_hw(struct os_clock *c, s64 offset)
{
	struct timex t;
	struct os_clock *_c;
	int err = 0;

	memset(&t, 0, sizeof(t));

	t.modes = ADJ_SETOFFSET | ADJ_NANO;
	t.time.tv_sec = offset / (s64)NSECS_PER_SEC;
	t.time.tv_usec = offset % (s64)NSECS_PER_SEC;

	if (offset < 0) {
		t.time.tv_sec -= 1;
		t.time.tv_usec += (s64)NSECS_PER_SEC;
	}

	pthread_mutex_lock(&os_clock_mutex);

	if (clock_adjtime(c->id, &t) < 0) {
		os_log(LOG_ERR, "clock_id(0x%x) failed adjusting offset\n", c->id);
		err = -1;
		goto unlock;
	}

	os_log(LOG_DEBUG, "clock_id(0x%x) offset clock by %"PRId64" ns\n", c->id, offset);

	/* for all other clocks, with the same parent/clock device, adjust by -offset */

	for_each_sw_clock_with_same_parent(c, _c) {

		_c->sw_clk.hw.t0 += offset;

		os_log(LOG_DEBUG, "clock_id(0x%x) adjusted hw.t0 offset by %"PRId64" ns\n",
				 _c->id, offset);
	}

unlock:
	pthread_mutex_unlock(&os_clock_mutex);

	return err;
}

static struct os_clock *clock_id_to_clock(os_clock_id_t clk_id)
{
	if ((clk_id < 0) || (clk_id >= OS_CLOCK_MAX))
		goto err;

	if (!os_clock[clock_id(clk_id)].enabled)
		goto err;

	return &os_clock[clock_id(clk_id)];

err:
	return NULL;
}

static bool clock_id_remapped(os_clock_id_t clk_id)
{
	return (clock_id(clk_id) != clk_id);
}

int clock_id_remap(os_clock_id_t clk_id, os_clock_id_t dst_clk_id)
{
	if (!clock_id_supports_remap(clk_id) || !clock_id_supports_remap(dst_clk_id))
		goto err;

	if (clock_id_remapped(dst_clk_id))
		goto err;

	clk_id_to_clk_id[clk_id] = dst_clk_id;

	return 0;

err:
	return -1;
}

bool os_clock_is_virtual_phc(os_clock_id_t clk_id)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c || !OS_CLOCK_IS_VIRTUAL_PHC(c))
		return false;

	return true;
}

bool os_clock_is_match(os_clock_id_t clk_id_0, os_clock_id_t clk_id_1)
{
	struct os_clock *c_0, *c_1;

	c_0 = clock_id_to_clock(clk_id_0);
	c_1 = clock_id_to_clock(clk_id_1);

	if (!c_0 || !c_1 || (c_0 != c_1))
		return false;

	return true;
}

/*
 * - For PHC based clocks (physical or virtual), return the device's phc index
 * - For SW clocks based on PHC devices, return the phc index of the parent device.
 */
int os_clock_to_phc_index(os_clock_id_t clk_id)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c || (!OS_CLOCK_IS_PHC(c) && (c->type != CLOCK_TYPE_SW)))
		return -1;

	return c->phc_index;
}

static const char *os_clock_get_type(struct os_clock *c)
{
	switch (c->type) {
	case CLOCK_TYPE_SYSTEM:
	{
		return "System";
	}
	case CLOCK_TYPE_SW:
	{
		return "Software";
	}
	case CLOCK_TYPE_PHC:
	{
		if (OS_CLOCK_IS_VIRTUAL_PHC(c))
			return "Virtual PHC";
		else
			return "Physical PHC";
	}
	default:
		return "";
	}
}

static int _os_clock_init(os_clock_id_t clk_id)
{
	struct os_clock *c;

	if (clock_id_remapped(clk_id)) {
		os_log(LOG_INIT, "clock(%s) remapped to (%s)\n",
			os_clock_id2string(clk_id), os_clock_id2string(clk_id_to_clk_id[clk_id]));
		goto exit;
	}

	c = clock_id_to_clock(clk_id);
	if (!c) {
		os_log(LOG_ERR, "clock ID invalid: %d\n", clk_id);
		goto err;
	}

	switch (c->type) {
	case CLOCK_TYPE_SYSTEM:
		c->gettime32 = &clock_gettime32_hw;
		c->gettime64 = &clock_gettime64_hw;
		c->setfreq = &clock_setfreq_hw;
		c->setoffset = &clock_setoffset_hw;
		c->id = CLOCK_MONOTONIC_RAW;
		c->flags |= OS_CLOCK_FLAGS_HW_OFFSET;
		break;

	case CLOCK_TYPE_PHC:
		c->gettime32 = &clock_gettime32_hw;
		c->gettime64 = &clock_gettime64_hw;
		c->setfreq = &clock_setfreq_hw;
		c->setoffset = &clock_setoffset_hw;
		/* Physical PHCs offset adjustments affect hardware clock (while frequency adjustements
		 * are properly tracked in the stack as local clock is forced to CLOCK_TYPE_SW
		 * in os_clock_config_init() when it has a child physical PHC)
		 */
		if (OS_CLOCK_IS_PHYSICAL_PHC(c))
			c->flags |= OS_CLOCK_FLAGS_HW_OFFSET;
		break;

	case CLOCK_TYPE_SW:
		c->gettime32 = &clock_gettime32_sw;
		c->gettime64 = &clock_gettime64_sw;
		c->setfreq = &clock_setfreq_sw;
		c->setoffset = &clock_setoffset_sw;
		break;

	default:
		os_log(LOG_ERR, "clock id: %d not supported\n", clk_id);
		goto err;
		break;
	}

	if (c->clk_device) {
		c->fd = open(c->clk_device, O_RDWR);
		if (c->fd < 0) {
			os_log(LOG_ERR, "clock(%p) %s: couldn't open clock char device: %s error: %s\n",
								c, os_clock_id2string(clk_id), c->clk_device, strerror(errno));

			c->enabled = false;

			goto err;
		}
		c->id = FD_TO_CLOCKID(c->fd);

		os_log(LOG_INIT, "clock(%p) %s: success, linux clockid (0x%x) fd (0x%x) type (%s) %s(clk_device (%s), phc_index (%d)) parent (%p)\n",
							c, os_clock_id2string(clk_id),
							c->id, c->fd, os_clock_get_type(c), (c->type == CLOCK_TYPE_SW) ? "parent " : "",
							c->clk_device, c->phc_index, c->parent_id);
	} else {
		os_log(LOG_INIT, "clock(%p) %s: success, linux clockid (0x%x)\n",
							c, os_clock_id2string(clk_id), c->id);
	}

exit:
	return 0;

err:
	return -1;
}

static int _os_clock_exit(os_clock_id_t clk_id)
{
	struct os_clock *c;

	if (clock_id_remapped(clk_id))
		goto exit;

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	if (c->fd >= 0)
		close(c->fd);

exit:
	return 0;

err:
	return -1;
}

#define LINUX_SYSFS_PTP_CLOCK_NAME_LEN		128
#define LINUX_SYSFS_FILENMAME_LEN		128
#define LINUX_SYSFS_PTP_CLOCK_NAME_PATTERN	"/sys/class/ptp/ptp%d/clock_name"

static void os_clock_get_phc_clock_type(struct os_clock *c)
{
	char sysfs_attribute[LINUX_SYSFS_FILENMAME_LEN];
	char clock_name[LINUX_SYSFS_PTP_CLOCK_NAME_LEN];
	int phc_physical;
	FILE *fPTP;

	if (h_snprintf_strict(sysfs_attribute, LINUX_SYSFS_FILENMAME_LEN, LINUX_SYSFS_PTP_CLOCK_NAME_PATTERN, c->phc_index) < 0 ) {
		os_log(LOG_ERR, "Unable to create sysfs attribute filename for /dev/ptp%d\n", c->phc_index);
		goto err;
	}

	fPTP = fopen(sysfs_attribute, "r");
	if (fPTP == NULL) {
		os_log(LOG_ERR, "fopen(%s) failed: %s\n", sysfs_attribute, strerror(errno));
		goto err;
	}

	if (fgets(clock_name, LINUX_SYSFS_PTP_CLOCK_NAME_LEN, fPTP) == NULL) {
		os_log(LOG_ERR, "fgets(%s) failed: %s\n", sysfs_attribute, strerror(errno));
		goto err_fgets;
	}

	/* virtual clock names have the following pattern: ptpX_virt where X is the parent/physical phc device. */
	if (sscanf(clock_name, "ptp%d_virt", &phc_physical) == 1) {
		os_log(LOG_INFO, "/dev/ptp%d is a virtual PHC with parent %d\n", c->phc_index, phc_physical);
		c->flags |= OS_CLOCK_FLAGS_PHC_VIRTUAL;
	} else {
		os_log(LOG_INFO, "/dev/ptp%d is a physical PHC\n", c->phc_index);
	}

	if (fclose(fPTP))
		os_log(LOG_ERR, "Unable to close %s stream: %s\n", sysfs_attribute, strerror(errno));

	return;

err_fgets:
	if (fclose(fPTP))
		os_log(LOG_ERR, "Unable to close %s stream: %s\n", sysfs_attribute, strerror(errno));
err:
	/* On detection failure, always assume the default (physical PHC) */
	return;
}

static void os_clock_get_config(struct os_clock *c, const char *clk_device)
{
	if (!strlen(clk_device) || !strcmp(clk_device, "off")) {
		c->enabled = false;
	} else if (!strncmp(clk_device, "sw_clock", sizeof("sw_clock"))) {
		c->type = CLOCK_TYPE_SW;
		c->enabled = true;
	} else {
		c->type = CLOCK_TYPE_PHC;
		c->clk_device = clk_device;
		c->enabled = true;

		/* phc_index is valid only for PHC type clocks (physical and virtual). */
		if (sscanf(clk_device, "/dev/ptp%d", &c->phc_index) != 1) {
			os_log(LOG_ERR, "clock(%p): failed to detect PHC index for %s\n", c, clk_device);
			c->phc_index = -1;
		} else {
			os_clock_get_phc_clock_type(c);
		}
	}
}

/*
 * PHC & software clocks configuration
 *
 * Set clocks' config parameters based on the config file
 * - force the local clock to be a software clock (only if it has a physical phc child)
 * - use the local clock name to define the hardware clock for the whole domain
 * - use that hw clock device as a parent ID for all software clocks
 * - all (and only) PHC clocks supports remapping.
 */
static void os_clock_config_init(struct os_clock_config *config)
{
	struct os_clock *hw_clock, *local, *c;
	bool local_has_physical_phc_child;
	int i, j;

	for (i = 0; i < CFG_MAX_ENDPOINTS; i++) {
		local_has_physical_phc_child = false;

		local = &os_clock[OS_CLOCK_LOCAL_EP_0 + i];
		local->flags = OS_CLOCK_FLAGS_SUPPORTS_REMAP;
		local->phc_index = -1;

		hw_clock = local;
		os_clock_get_config(local, config->endpoint_local[i]);
		local->parent_id = hw_clock;

		for (j = 0; j < CFG_MAX_GPTP_DOMAINS; j++) {
			c = &os_clock[OS_CLOCK_GPTP_EP_0_0 + i * CFG_MAX_GPTP_DOMAINS + j];
			c->flags = OS_CLOCK_FLAGS_SUPPORTS_REMAP;
			c->phc_index = -1;

			os_clock_get_config(c, config->endpoint_gptp[j][i]);

			if (c->type == CLOCK_TYPE_SW) {
				c->clk_device = hw_clock->clk_device;
				c->phc_index = hw_clock->phc_index;
			}

			if (OS_CLOCK_IS_PHYSICAL_PHC(c) && !OS_CLOCK_IS_VIRTUAL_PHC(local))
				local_has_physical_phc_child = true;

			c->parent_id = hw_clock;
		}

		/* If one of the target clocks is a physical PHC and local is not virtual (i.e local and
		 * target point to the same physical PHC device), force local to be a SW clock in order 
		 * to have a more stable local clock when target clock adjustements affect hardware clock.
		 */
		if (local_has_physical_phc_child)
			local->type = CLOCK_TYPE_SW;

		c = &os_clock[OS_CLOCK_AVTP_MEDIA_0 + i];
		c->flags = OS_CLOCK_FLAGS_SUPPORTS_REMAP;
		c->phc_index = -1;
	}

	for (i = 0; i < CFG_MAX_BRIDGES; i++) {
		local_has_physical_phc_child = false;

		local = &os_clock[OS_CLOCK_LOCAL_BR_0 + i];
		local->flags = OS_CLOCK_FLAGS_SUPPORTS_REMAP;
		local->phc_index = -1;

		hw_clock = local;
		os_clock_get_config(local, config->bridge_local[i]);
		local->parent_id = hw_clock;

		for (j = 0; j < CFG_MAX_GPTP_DOMAINS; j++) {
			c = &os_clock[OS_CLOCK_GPTP_BR_0_0 + i * CFG_MAX_GPTP_DOMAINS + j];
			c->flags = OS_CLOCK_FLAGS_SUPPORTS_REMAP;
			c->phc_index = -1;

			os_clock_get_config(c, config->bridge_gptp[j][i]);

			if (c->type == CLOCK_TYPE_SW) {
				c->clk_device = hw_clock->clk_device;
				c->phc_index = hw_clock->phc_index;
			}

			if (OS_CLOCK_IS_PHYSICAL_PHC(c) && !OS_CLOCK_IS_VIRTUAL_PHC(local))
				local_has_physical_phc_child = true;

			c->parent_id = hw_clock;
		}

		/* If one of the target clocks is a physical PHC and local is not virtual (i.e local and
		 * target point to the same physical PHC device), force local to be a SW clock in order 
		 * to have a more stable local clock when target clock adjustements affect hardware clock.
		 */
		if (local_has_physical_phc_child)
			local->type = CLOCK_TYPE_SW;
	}

	/* The AVTP media clock on a specific interface should be mapped to:
	 * - Target gPTP clock domain 0 if it (the target gPTP clock) is a physical PHC
	 * - local gPTP clock if target gPTP clock is a virtual PHC.
	 */
	for (i = 0; i < CFG_MAX_ENDPOINTS; i++) {
		c = &os_clock[OS_CLOCK_GPTP_EP_0_0 + i * CFG_MAX_GPTP_DOMAINS];

		if (i) {
			/* For ports with virtual clocks as target gPTP and a physical
			 * local clock having the same PHC index as the first port's local
			 * clock (setup with redundant ports), map all AVTP media clocks to
			 * the same local clock 0 for the sake of os_clock_is_match()
			 */
			struct os_clock *local_0 = &os_clock[OS_CLOCK_LOCAL_EP_0];

			local = &os_clock[OS_CLOCK_LOCAL_EP_0 + i];

			if (OS_CLOCK_IS_VIRTUAL_PHC(c) && OS_CLOCK_IS_PHYSICAL_PHC(local)
				&& OS_CLOCK_IS_PHYSICAL_PHC(local_0) && local->phc_index == local_0->phc_index) {

				clock_id_remap(OS_CLOCK_AVTP_MEDIA_0 + i, OS_CLOCK_LOCAL_EP_0);
				continue;
			}
		}

		clock_id_remap(OS_CLOCK_AVTP_MEDIA_0 + i,
			       OS_CLOCK_IS_VIRTUAL_PHC(c) ? (OS_CLOCK_LOCAL_EP_0 + i) : (OS_CLOCK_GPTP_EP_0_0 + i * CFG_MAX_GPTP_DOMAINS));
	}
}

int os_clock_init(struct os_clock_config *config)
{
	int i;

	pthread_mutex_init(&os_clock_mutex, NULL);

	os_clock_config_init(config);

	for (i = 0; i < OS_CLOCK_MAX; i++)
		_os_clock_init(i);

	return 0;
}

void os_clock_exit(void)
{
	int i;

	for (i = 0; i < OS_CLOCK_MAX; i++)
		_os_clock_exit(i);

	pthread_mutex_destroy(&os_clock_mutex);
}

int os_clock_gettime32(os_clock_id_t clk_id, u32 *ns)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	if (c->gettime32)
		return c->gettime32(c, ns);

err:
	return -1;
}

int os_clock_gettime64(os_clock_id_t clk_id, u64 *ns)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	if (c->gettime64)
		return c->gettime64(c, ns);

err:
	return -1;
}

/**
 * Get time in nanoseconds of the parent (hardware) clock.
 * \param id	clock id.
 * \param ns	pointer to u64 variable that will hold the result.
 * \return	0 on success, or negative value on error.
 */
int os_clock_gettime64_of_parent(os_clock_id_t clk_id, u64 *ns)
{
	struct timespec now;
	struct os_clock *c;
	struct os_clock *parent;
	int err = 0;

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	parent = c->parent_id;
	if (!parent)
		goto err;

	err = clock_gettime(parent->id, &now);
	if (err) {
		os_log(LOG_ERR, "clock(%p) clock_gettime failed: %s\n", c, strerror(errno));
		goto err;
	}

	*ns = (u64)now.tv_sec*NSECS_PER_SEC + now.tv_nsec;

	return 0;
err:
	return -1;
}

int os_clock_convert_multi(os_clock_id_t clk_id_src, u64 *ns_src, unsigned int num_ts, os_clock_id_t clk_id_dst, u64 *ns_dst)
{
	struct os_clock *c_src;
	struct os_clock *c_dst;
	int i;

	if (!num_ts)
		goto pass_through;

	c_src = clock_id_to_clock(clk_id_src);
	if (!c_src) {
		os_log(LOG_ERR, " Unsupported source clk_id %d\n", clk_id_src);
		goto err;
	}

	c_dst = clock_id_to_clock(clk_id_dst);
	if (!c_dst) {
		os_log(LOG_ERR, " Unsupported destination clk_id %d\n", clk_id_dst);
		goto err;
	}

	if (c_src == c_dst) {
		memcpy(ns_dst, ns_src, num_ts * sizeof(u64));
		goto pass_through;
	}

	/* If the source or the destination is a virtual phc, use the ioctl to convert */
	if (OS_CLOCK_IS_VIRTUAL_PHC(c_src) || OS_CLOCK_IS_VIRTUAL_PHC(c_dst)) {
		struct ptp_convert_timestamps convert_ts;
		int err;

		if (num_ts > PTP_MAX_CONVERT_TS_NUM) { /*FIXME have multiple calls with PTP_MAX_CONVERT_TS_NUM batches */
			os_log(LOG_ERR, "clock source(%p) num_ts(%u) exceeds max supported number of timestamps (%u)\n", c_src, num_ts, PTP_MAX_CONVERT_TS_NUM);
			goto err;
		}

		convert_ts.dst_phc_index = c_dst->phc_index;
		for (i = 0; i < num_ts; i++) {
			convert_ts.src_ts[i].sec = ns_src[i] / NSECS_PER_SEC;
			convert_ts.src_ts[i].nsec = ns_src[i] - convert_ts.src_ts[i].sec * NSECS_PER_SEC;
		}

		convert_ts.n_ts = num_ts;

		err = ioctl(c_src->fd, PTP_CONVERT_TIMESTAMPS, &convert_ts);
		if (err) {
			os_log(LOG_ERR, "clock source(%p) ioctl(PTP_CONVERT_TIMESTAMPS) failed: %s\n", c_src, strerror(errno));
			goto err;
		}

		for (i = 0; i < num_ts; i++)
			ns_dst[i] = convert_ts.dst_ts[i].nsec + convert_ts.dst_ts[i].sec * NSECS_PER_SEC;

	} else {
		if (!OS_CLOCK_SHARE_SAME_PARENT(c_src, c_dst)) {
			os_log(LOG_ERR, " Incompatible hardware clocks source(%p) destination(%p)\n", c_src, c_dst);
			goto err;
		}

		pthread_mutex_lock(&os_clock_mutex);

		for (i = 0; i < num_ts; i++) {
			ns_dst[i] = __clock_time_to_hw(c_src, ns_src[i]);
			ns_dst[i] = __clock_time_from_hw(c_dst, ns_dst[i]);
		}

		pthread_mutex_unlock(&os_clock_mutex);
	}

pass_through:
	return 0;

err:
	*ns_dst = 0;
	return -1;
}

int os_clock_setfreq(os_clock_id_t clk_id, s32 ppb)
{
	struct os_clock *c;

	os_log(LOG_DEBUG, "clock %d ppb %d\n", clk_id, ppb);

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	if (c->setfreq)
		return c->setfreq(c, ppb);

err:
	return -1;
}

int os_clock_setoffset(os_clock_id_t clk_id, s64 offset)
{
	struct os_clock *c;

	os_log(LOG_DEBUG, "clock %d offset %"PRId64"\n", clk_id, offset);

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	if (c->setoffset)
		return c->setoffset(c, offset);

err:
	return -1;
}

unsigned int os_clock_adjust_mode(os_clock_id_t clk_id)
{
	struct os_clock *c;
	unsigned int mode = 0;

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto exit;

	if (c->flags & OS_CLOCK_FLAGS_HW_OFFSET)
		mode |= OS_CLOCK_ADJUST_MODE_HW_OFFSET;

exit:
	return mode;
}

/**
 * Convert hw clock time to sw clock time.
 * The time must come from the root hw clock of the sw clock
 * given as argument.
 * \param id		clock id.
 * \param hw_ns		hw time to convert
 * \param ns		pointer to u64 variable that will hold the result.
 * \return		0 on success, or negative value on error.
 */
int clock_time_from_hw(os_clock_id_t clk_id, uint64_t hw_ns, uint64_t *ns)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c || !c->parent_id)
		goto err;

	pthread_mutex_lock(&os_clock_mutex);

	*ns = __clock_time_from_hw(c, hw_ns);

	pthread_mutex_unlock(&os_clock_mutex);

	return 0;

err:
	return -1;
}
