/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TEE_RPC_TYPES_H
#define TEE_RPC_TYPES_H

#include "tee_api_types.h"

typedef struct _tee_rpc_rpmb_cmd {
    uint16_t cmd;
    uint16_t dev_id;
    uint16_t block_count;
	/* variable length of data */
	/* uint8_t data[]; REMOVED! */
} tee_rpc_rpmb_cmd_t;

#define RPMB_EMMC_CID_SIZE 16
typedef struct _tee_rpc_rpmb_dev_info {
    uint8_t cid[RPMB_EMMC_CID_SIZE];
    /* EXT CSD-slice 168 "RPMB Size" */
    uint8_t rpmb_size_mult;
    /* EXT CSD-slice 222 "Reliable Write Sector Count" */
    uint8_t rel_wr_sec_c;
    /* Check the ret code and accept the data only if it is OK. */
    uint8_t ret_code;
} tee_rpc_rpmb_dev_info;

#endif
