/*
* Copyright 2019-2020 NXP
* 
* NXP Confidential. This software is owned or controlled by NXP and may only 
* be used strictly in accordance with the applicable license terms.  By expressly 
* accepting such terms or by downloading, installing, activating and/or otherwise 
* using the software, you are agreeing that you have read, and that you agree to 
* comply with and are bound by, such license terms.  If you do not agree to be 
* bound by the applicable license terms, then you may not retain, install, activate 
* or otherwise use the software.
*/

/**
 @file
 @brief AVB GPT driver
 @details
*/
#ifndef _GPT_REC_H_
#define _GPT_REC_H_

#include "fsl_gpt.h"
#include "gpt.h"
#include "media_clock.h"
#include "media_clock_rec_pll.h"

#define GPT_REC_FACTOR		16
#define GPT_REC_P_FACTOR	1
#define GPT_REC_I_FACTOR	3
#define GPT_REC_MAX_ADJUST	100
#define GPT_REC_NB_START	10

#define GPT_REC_INSTANCE	1

#ifdef CONFIG_AVTP
int gpt_rec_init(struct gpt_rec *rec);
void gpt_rec_exit(struct gpt_rec *rec);
#else
static inline int gpt_rec_init(struct gpt_rec *rec)
{
	return -1;
}
static inline void gpt_rec_exit(struct gpt_rec *rec)
{
	return;
}
#endif

#endif /* _GPT_REC_H_ */
