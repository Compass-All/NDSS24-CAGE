/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* The test engine supports numerous frames but we only use a few */
#define FRAME_COUNT	(2U)
#define FRAME_SIZE	(0x80U) /* 128 bytes */
#define F_IDX(n)	(n * FRAME_SIZE)

/* Commands supported by SMMUv3TestEngine built into the AEM */
#define ENGINE_NO_FRAME	(0U)
#define ENGINE_HALTED	(1U)

/* Base address of user and PRIV frames in SMMUv3TestEngine */
#define USR_BASE_FRAME		ULL(0x2BFE0000)
#define PRIV_BASE_FRAME		ULL(0x2BFF0000)


#define NO_SUBSTREAMID	(0xFFFFFFFFU)

/*
 * ENGINE_MEMCPY: Read and Write transactions
 * ENGINE_RAND48: Only Write transactions: Source address not required
 * ENGINE_SUM64: Only read transactions: Target address not required
 */
#define ENGINE_MEMCPY	(2U)
#define ENGINE_RAND48	(3U)
#define ENGINE_SUM64	(4U)
#define ENGINE_ERROR	(0xFFFFFFFFU)
#define ENGINE_MIS_CFG	(ENGINE_ERROR - 1)

/*
 * Refer to:
 * https://developer.arm.com/documentation/100964/1111-00/Trace-components/SMMUv3TestEngine---trace
 */

/* Offset of various control fields belonging to User Frame */
#define CMD_OFF		(0x0U)
#define UCTRL_OFF	(0x4U)
#define SEED_OFF	(0x24U)
#define BEGIN_OFF	(0x28U)
#define END_CTRL_OFF	(0x30U)
#define STRIDE_OFF	(0x38U)
#define UDATA_OFF	(0x40U)

/* Offset of various control fields belonging to PRIV Frame */
#define PCTRL_OFF		(0x0U)
#define DOWNSTREAM_PORT_OFF	(0x4U)
#define STREAM_ID_OFF		(0x8U)
#define SUBSTREAM_ID_OFF	(0xCU)
