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

#ifndef TEE_RPC_H
#define TEE_RPC_H

/*
 * tee_rpc_invoke cmd definitions, keep in sync with tee-supplicant
 */
#define TEE_RPC_CMD_LOAD_TA		0
#define TEE_RPC_CMD_RPMB	    1
#define TEE_RPC_CMD_FS		    2
#define TEE_RPC_CMD_GET_TIME	3
#define TEE_RPC_CMD_WAIT_QUEUE  4
#define TEE_RPC_CMD_SUSPEND     5
#define TEE_RPC_CMD_SHM_ALLOC   6
#define TEE_RPC_CMD_SHM_FREE    7
#define TEE_RPC_CMD_SQL_FS	    8
#define TEE_RPC_CMD_GPROF	    9
#define TEE_RPC_CMD_SOCKET	    10
#define TEE_RPC_CMD_OCALL	    11

/* keep in sync with Linux driver */
#define TEE_RPC_WAIT_MUTEX	0x20000000
/* Values specific to TEE_RPC_WAIT_MUTEX */
#define TEE_WAIT_MUTEX_SLEEP	0
#define TEE_WAIT_MUTEX_WAKEUP	1
#define TEE_WAIT_MUTEX_DELETE	2
#define TEE_RPC_WAIT		0x30000000

/* Values specific to TEE_RPC_WAIT_QUEUE */
#define TEE_WAIT_QUEUE_SLEEP	0
#define TEE_WAIT_QUEUE_WAKEUP	1

/* RPMB Related defines */
#define TEE_RPC_RPMB_CMD_DATA_REQ       0x00
#define TEE_RPC_RPMB_CMD_GET_DEV_INFO   0x01

/* Error codes for get_dev_info request/response. */
#define TEE_RPC_RPMB_CMD_GET_DEV_INFO_RET_OK     0x00
#define TEE_RPC_RPMB_CMD_GET_DEV_INFO_RET_ERROR  0x01

/* Shared memory related defines */

/*
 * Notes on shared memory allocation types.
 * 
 * Windows shared memory semantics are as follows:
 * 1. TEE_RPC_SHM_ALLOC_TYPE_KERNEL:
 *    Share memory between a TA and the non-secure kernel.
 * 2. TEE_RPC_SHM_ALLOC_TYPE_APPL:
 *    Share memory between a TA and a non-secure user-space application.
 * 
 * Linux shared memory semantics are as follows:
 * 1. TEE_RPC_SHM_ALLOC_TYPE_KERNEL:
 *    Share memory between a TA and the non-secure kernel.
 * 2. TEE_RPC_SHM_ALLOC_TYPE_APPL:
 *    Share memory between a TA and the tee-supplicant.
 * 
 * Notice how TEE_RPC_SHM_ALLOC_TYPE_APPL means two different things depending
 * on the host OS. As part of our adding support to Linux to share memory
 * between a TA and a non-secure user-space application, a way was needed to
 * resolve this ambiguity. This way is a new shared memory allocation type,
 * namely TEE_RPC_SHM_ALLOC_TYPE_HOST.
 * 
 * On Windows, TEE_RPC_SHM_ALLOC_TYPE_HOST and TEE_RPC_SHM_ALLOC_TYPE_APPL are
 * equivalent. On Linux, TEE_RPC_SHM_ALLOC_TYPE_APPL continues to mean what was
 * outlined above, and TEE_RPC_SHM_ALLOC_TYPE_HOST means what it means on
 * Windows.
 * 
 */

/* Memory that can be shared with a non-secure user-space application */
#define TEE_RPC_SHM_ALLOC_TYPE_APPL	0
/* Memory only shared with the non-secure kernel */
#define TEE_RPC_SHM_ALLOC_TYPE_KERNEL 1
/* On Windows, same as TEE_RPC_SHM_ALLOC_TYPE_APPL */
#define TEE_RPC_SHM_ALLOC_TYPE_HOST 3

#endif
