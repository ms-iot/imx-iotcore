/*
 * Copyright (c) 2014, Linaro Limited
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
#ifndef TEESMC_H
#define TEESMC_H

#ifndef ASM
/*
 * This section depends on uint64_t, uint32_t uint8_t already being
 * defined. Since this file is used in several different environments
 * (secure world OS and normal world Linux kernel to start with) where
 * stdint.h may not be available it's the responsibility of the one
 * including this file to provide those types.
 */

/*
 * Trusted OS SMC interface.
 *
 * The SMC interface follows SMC Calling Convention
 * (ARM_DEN0028A_SMC_Calling_Convention).
 *
 * The primary objective of this API is to provide a transport layer on
 * which a Global Platform compliant TEE interfaces can be deployed. But the
 * interface can also be used for other implementations.
 *
 * This file is divided in two parts.
 * Part 1 deals with passing parameters to Trusted Applications running in
 * a trusted OS in secure world.
 * Part 2 deals with the lower level handling of the SMC.
 */

/*
 *******************************************************************************
 * Part 1 - passing parameters to Trusted Applications
 *******************************************************************************
 */

/*
 * Same values as TEE_PARAM_* from TEE Internal API
 */
#define TEESMC_ATTR_TYPE_NONE		    0x0
#define TEESMC_ATTR_TYPE_VALUE_INPUT	0x1
#define TEESMC_ATTR_TYPE_VALUE_OUTPUT	0x2
#define TEESMC_ATTR_TYPE_VALUE_INOUT	0x3
#define TEESMC_ATTR_TYPE_RMEM_INPUT	    0x5
#define TEESMC_ATTR_TYPE_RMEM_OUTPUT	0x6
#define TEESMC_ATTR_TYPE_RMEM_INOUT	    0x7
#define TEESMC_ATTR_TYPE_TMEM_INPUT	    0x9
#define TEESMC_ATTR_TYPE_TMEM_OUTPUT	0xA
#define TEESMC_ATTR_TYPE_TMEM_INOUT	    0xB

#define TEESMC_ATTR_TYPE_MASK		0xFF

/*
 * Meta parameter to be absorbed by the Secure OS and not passed
 * to the Trusted Application.
 *
 * Currently only used with OPTEE_MSG_CMD_OPEN_SESSION.
 */
#define TEESMC_ATTR_META		(1 << 8)

/*
 * The temporary shared memory object is not physically contiguous and this
 * temp memref is followed by another fragment until the last temp memref
 * that doesn't have this bit set.
 */
#define TEESMC_ATTR_FRAGMENT    (1 << 9)


/*
 * Used as an indication from normal world of compatible cache usage.
 * 'I' stands for inner cache and 'O' for outer cache.
 */
#define TEESMC_ATTR_CACHE_I_NONCACHE	0x0
#define TEESMC_ATTR_CACHE_I_WRITE_THR	0x1
#define TEESMC_ATTR_CACHE_I_WRITE_BACK	0x2
#define TEESMC_ATTR_CACHE_O_NONCACHE	0x0
#define TEESMC_ATTR_CACHE_O_WRITE_THR	0x4
#define TEESMC_ATTR_CACHE_O_WRITE_BACK	0x8

#define TEESMC_ATTR_CACHE_NONCACHE	0x0
#define TEESMC_ATTR_CACHE_DEFAULT	(TEESMC_ATTR_CACHE_I_WRITE_BACK | \
					 TEESMC_ATTR_CACHE_O_WRITE_BACK)

#define TEESMC_ATTR_CACHE_SHIFT		4
#define TEESMC_ATTR_CACHE_MASK		0xf

#define TEESMC_CMD_OPEN_SESSION		0
#define TEESMC_CMD_INVOKE_COMMAND	1
#define TEESMC_CMD_CLOSE_SESSION	2
#define TEESMC_CMD_CANCEL		3

/**
 * struct teesmc_param_tmem - temporary memory reference
 * @buf_ptr:	Address of the buffer
 * @size:	Size of the buffer
 * @shm_ref:	Temporary shared memory reference, pointer to a struct tee_shm
 *
 * Secure and normal world communicates pointers as physical address
 * instead of the virtual address. This is because secure and normal world
 * have completely independent memory mapping. Normal world can even have a
 * hypervisor which need to translate the guest physical address (AKA IPA
 * in ARM documentation) to a real physical address before passing the
 * structure to secure world.
 */
struct teesmc_param_tmem {
    uint64_t buf_ptr;
    uint64_t size;
    uint64_t shm_ref;
};

/**
* struct teesmc_param_rmem - registered memory reference
* @offs:	Offset into shared memory reference
* @size:	Size of the buffer
* @shm_ref:	Shared memory reference, pointer to a struct tee_shm
*/
struct teesmc_param_rmem {
    uint64_t offs;
    uint64_t size;
    uint64_t shm_ref;
};


/**
 * struct teesmc_param_value - values
 * @a: first value
 * @b: second value
 * @c: third value
 */
struct teesmc_param_value {
    uint64_t a;
    uint64_t b;
    uint64_t c;
};

 /**
  * struct teesmc_param - parameter
  * @attr: attributes
  * @memref: a memory reference
  * @value: a value
  *
  * @attr & TEESMC_ATTR_TYPE_MASK indicates if tmem, rmem or value is used in
  * the union.OPTEE_MSG_ATTR_TYPE_VALUE_* indicates value,
  * TEESMC_ATTR_TYPE_TMEM_* indicates tmem and
  * TEESMC_ATTR_TYPE_RMEM_* indicates rmem.
  * TEESMC_ATTR_TYPE_NONE indicates that none of the members are used.
  */
struct teesmc_param {
	uint32_t attr;
	union {
        struct teesmc_param_tmem tmem;
        struct teesmc_param_rmem rmem;
        struct teesmc_param_value value;
	} u;
};

 /**
 * struct teesmc32_arg - SMC argument for Trusted OS
 * @cmd: Command, one of TEESMC_CMD_*
 * @func: Trusted Application function, specific to the Trusted Application,
 *	     used if cmd == TEESMC_CMD_INVOKE_COMMAND
 * @session: In parameter for all TEESMC_CMD_* except
 *	     TEESMC_CMD_OPEN_SESSION where it's an output parameter instead
 * @cancel_id: Cancellation id, a unique value to identify this request
 * @ret: return value
 * @ret_origin: origin of the return value
 * @num_params: number of parameters supplied to the OS Command
 * @params: the parameters supplied to the OS Command
 *
 * All normal calls to Trusted OS uses this struct. If cmd requires further
 * information than what these fields hold it can be passed as a parameter
 * tagged as meta (setting the OPTEE_MSG_ATTR_META bit in corresponding
 * attrs field). All parameters tagged as meta have to come first.
 *
 * Temp memref parameters can be fragmented if supported by the Trusted OS
 * (when optee_smc.h is bearer of this protocol this is indicated with
 * OPTEE_SMC_SEC_CAP_UNREGISTERED_SHM). If a logical memref parameter is
 * fragmented then all but the last fragment have the
 * OPTEE_MSG_ATTR_FRAGMENT bit set in attrs. Even if a memref is fragmented
 * it will still be presented as a single logical memref to the Trusted
 * Application.
 */
struct teesmc_arg {
    uint32_t cmd;
    uint32_t func;
    uint32_t session;
    uint32_t cancel_id;
    uint32_t pad;
    uint32_t ret;
    uint32_t ret_origin;
    uint32_t num_params;

    /*
    * this struct is 8 byte aligned since the 'struct optee_msg_param'
    * which follows requires 8 byte alignment.
    *
    * Commented out element used to visualize the layout dynamic part
    * of the struct. This field is not available at all if
    * num_params == 0.
    *
    * params is accessed through the macro TEESMC_GET_PARAMS
    *
    * struct teesmc_param params[num_params];
    */
};

/**
 * TEESMC_GET_PARAMS - return pointer to union teesmc_param *
 *
 * @x: Pointer to a struct teesmc32_arg
 *
 * Returns a pointer to the params[] inside a struct teesmc32_arg.
 */
#define TEESMC_GET_PARAMS(x) \
	(struct teesmc_param *)(((struct teesmc_arg *)(x)) + 1)

/**
 * TEESMC32_GET_ARG_SIZE - return size of struct teesmc32_arg
 *
 * @num_params: Number of parameters embedded in the struct teesmc32_arg
 *
 * Returns the size of the struct teesmc32_arg together with the number
 * of embedded paramters.
 */
#define TEESMC_GET_ARG_SIZE(num_params) \
	(sizeof(struct teesmc_arg) + \
	 sizeof(struct teesmc_param) * (num_params))

#define TEESMC_UUID_LEN	16

/**
 * struct teesmc_meta_open_session - additional parameters for
 *				     TEESMC32_CMD_OPEN_SESSION and
 *				     TEESMC64_CMD_OPEN_SESSION
 * @uuid: UUID of the Trusted Application
 * @clnt_uuid: UUID of client
 * @clnt_login: Login class of client, TEE_LOGIN_* if being Global Platform
 *		compliant
 *
 * This struct is passed in the first parameter as an input memref tagged
 * as meta on an TEESMC{32,64}_CMD_OPEN_SESSION cmd. It's important
 * that it really is the first parameter to make it easy for an eventual
 * hypervisor to inspect and possibly update clnt_* values.
 */
struct teesmc_meta_open_session {
	uint8_t uuid[TEESMC_UUID_LEN];
	uint8_t clnt_uuid[TEESMC_UUID_LEN];
	uint32_t clnt_login;
};


#endif /*!ASM*/

/*
 *******************************************************************************
 * Part 2 - low level SMC interaction
 *******************************************************************************
 */

#define TEESMC_32			0
#define TEESMC_64			0x40000000
#define TEESMC_FAST_CALL		0x80000000
#define TEESMC_STD_CALL			0

#define TEESMC_OWNER_MASK		0x3F
#define TEESMC_OWNER_SHIFT		24

#define TEESMC_FUNC_MASK		0xFFFF

#define TEESMC_IS_FAST_CALL(smc_val)	((smc_val) & TEESMC_FAST_CALL)
#define TEESMC_IS_64(smc_val)		((smc_val) & TEESMC_64)
#define TEESMC_FUNC_NUM(smc_val)	((smc_val) & TEESMC_FUNC_MASK)
#define TEESMC_OWNER_NUM(smc_val)	(((smc_val) >> TEESMC_OWNER_SHIFT) & \
					 TEESMC_OWNER_MASK)

#define TEESMC_CALL_VAL(type, calling_convention, owner, func_num) \
			((type) | (calling_convention) | \
			(((owner) & TEESMC_OWNER_MASK) << TEESMC_OWNER_SHIFT) |\
			((func_num) & TEESMC_FUNC_MASK))

#define TEESMC_OWNER_ARCH		0
#define TEESMC_OWNER_CPU		1
#define TEESMC_OWNER_SIP		2
#define TEESMC_OWNER_OEM		3
#define TEESMC_OWNER_STANDARD		4
#define TEESMC_OWNER_TRUSTED_APP	48
#define TEESMC_OWNER_TRUSTED_OS		50

#define TEESMC_OWNER_TRUSTED_OS_OPTEED	62
#define TEESMC_OWNER_TRUSTED_OS_API	63

/*
 * Function specified by SMC Calling convention.
 */
#define TEESMC32_FUNCID_CALLS_COUNT	0xFF00
#define TEESMC32_CALLS_COUNT \
	TEESMC_CALL_VAL(TEESMC_32, TEESMC_FAST_CALL, \
			TEESMC_OWNER_TRUSTED_OS_API, \
			TEESMC32_FUNCID_CALLS_COUNT)

/*
 * Function specified by SMC Calling convention
 *
 * Return one of the following UIDs if using API specified in this file
 * without further extentions:
 * 65cb6b93-af0c-4617-8ed6-644a8d1140f8 : Only 32 bit calls are supported
 * 65cb6b93-af0c-4617-8ed6-644a8d1140f9 : Both 32 and 64 bit calls are supported
 */
#define TEESMC_UID_R0			0x65cb6b93
#define TEESMC_UID_R1			0xaf0c4617
#define TEESMC_UID_R2			0x8ed6644a
#define TEESMC_UID32_R3			0x8d1140f8
#define TEESMC_UID64_R3			0x8d1140f9
#define TEESMC32_FUNCID_CALLS_UID	0xFF01
#define TEESMC32_CALLS_UID \
	TEESMC_CALL_VAL(TEESMC_32, TEESMC_FAST_CALL, \
			TEESMC_OWNER_TRUSTED_OS_API, \
			TEESMC32_FUNCID_CALLS_UID)

/*
 * Function specified by SMC Calling convention
 *
 * Returns 1.0 if using API specified in this file without further extentions.
 */
#define TEESMC_REVISION_MAJOR	1
#define TEESMC_REVISION_MINOR	0
#define TEESMC32_FUNCID_CALLS_REVISION	0xFF03
#define TEESMC32_CALLS_REVISION \
	TEESMC_CALL_VAL(TEESMC_32, TEESMC_FAST_CALL, \
			TEESMC_OWNER_TRUSTED_OS_API, \
			TEESMC32_FUNCID_CALLS_REVISION)

/*
 * Get UUID of Trusted OS.
 *
 * Used by non-secure world to figure out which Trusted OS is installed.
 * Note that returned UUID is the UUID of the Trusted OS, not of the API.
 *
 * Returns UUID in r0-4/w0-4 in the same way as TEESMC32_CALLS_UID
 * described above.
 */
#define TEESMC_FUNCID_GET_OS_UUID	0
#define TEESMC32_CALL_GET_OS_UUID \
	TEESMC_CALL_VAL(TEESMC_32, TEESMC_FAST_CALL, TEESMC_OWNER_TRUSTED_OS, \
			TEESMC_FUNCID_GET_OS_UUID)

/*
 * Get revision of Trusted OS.
 *
 * Used by non-secure world to figure out which version of the Trusted OS
 * is installed. Note that the returned revision is the revision of the
 * Trusted OS, not of the API.
 *
 * Returns revision in r0-1/w0-1 in the same way as TEESMC32_CALLS_REVISION
 * described above.
 */
#define TEESMC_FUNCID_GET_OS_REVISION	1
#define TEESMC32_CALL_GET_OS_REVISION \
	TEESMC_CALL_VAL(TEESMC_32, TEESMC_FAST_CALL, TEESMC_OWNER_TRUSTED_OS, \
			TEESMC_FUNCID_GET_OS_REVISION)

 /*
 * Call with struct optee_msg_arg as argument
 *
 * Call register usage:
 * a0	SMC Function ID, OPTEE_SMC*CALL_WITH_ARG
 * a1	Upper 32 bits of a 64-bit physical pointer to a struct optee_msg_arg
 * a2	Lower 32 bits of a 64-bit physical pointer to a struct optee_msg_arg
 * a3	Cache settings, not used if physical pointer is in a predefined shared
 *	memory area else per OPTEE_SMC_SHM_*
 * a4-6	Not used
 * a7	Hypervisor Client ID register
 *
 * Normal return register usage:
 * a0	Return value, TEESMC_RETURN_*
 * a1-3	Not used
 * a4-7	Preserved
 *
 * TEESMC_RETURN_ETHREAD_LIMIT return register usage:
 * a0	Return value, TEESMC_RETURN_ETHREAD_LIMIT
 * a1-3	Preserved
 * a4-7	Preserved
 *
 * RPC return register usage:
 * a0	Return value, TEESMC_RETURN_IS_RPC(val)
 * a1-2	RPC parameters
 * a3-7	Resume information, must be preserved
 *
 * Possible return values:
 * TEESMC_RETURN_UNKNOWN_FUNCTION	Trusted OS does not recognize this
 *					function.
 * TEESMC_RETURN_OK			Call completed, result updated in
 *					the previously supplied struct
 *					optee_msg_arg.
 * TEESMC_RETURN_ETHREAD_LIMIT	Number of Trusted OS threads exceeded,
 *					try again later.
 * TEESMC_RETURN_EBADADDR		Bad physical pointer to struct
 *					optee_msg_arg.
 * TEESMC_RETURN_EBADCMD		Bad/unknown cmd in struct optee_msg_arg
 * TEESMC_RETURN_IS_RPC()		Call suspended by RPC call to normal
 *					world.
 */
#define TEESMC_FUNCID_CALL_WITH_ARG	4
#define TEESMC32_CALL_WITH_ARG \
	TEESMC_CALL_VAL(TEESMC_32, TEESMC_STD_CALL, TEESMC_OWNER_TRUSTED_OS, \
	TEESMC_FUNCID_CALL_WITH_ARG)
/* Same as TEESMC32_CALL_WITH_ARG but a "fast call". */
#define TEESMC32_FASTCALL_WITH_ARG \
	TEESMC_CALL_VAL(TEESMC_32, TEESMC_FAST_CALL, TEESMC_OWNER_TRUSTED_OS, \
	TEESMC_FUNCID_CALL_WITH_ARG)

/*
 * Call with struct teesmc64_arg as argument
 *
 * See description of TEESMC32_CALL_WITH_ARG above, uses struct
 * teesmc64_arg in x1 instead.
 */
#define TEESMC64_CALL_WITH_ARG \
	TEESMC_CALL_VAL(TEESMC_64, TEESMC_STD_CALL, TEESMC_OWNER_TRUSTED_OS, \
	TEESMC_FUNCID_CALL_WITH_ARG)
/* Same as TEESMC64_CALL_WITH_ARG but a "fast call". */
#define TEESMC64_FASTCALL_WITH_ARG \
	TEESMC_CALL_VAL(TEESMC_64, TEESMC_FAST_CALL, TEESMC_OWNER_TRUSTED_OS, \
	TEESMC_FUNCID_CALL_WITH_ARG)

 /*
 * Resume from RPC (for example after processing an IRQ)
 *
 * Call register usage:
 * a0	SMC Function ID, TEESMC32_CALL_RETURN_FROM_RPC
 * a1-3	Value of a1-3 when TEESMC32_CALL_WITH_ARG returned
 *	TEESMC_RETURN_RPC in a0
 *
 * Return register usage is the same as for TEESMC32_CALL_WITH_ARG above.
 *
 * Possible return values
 * TEESMC_RETURN_UNKNOWN_FUNCTION	Trusted OS does not recognize this
 *					function.
 * TEESMC_RETURN_OK			Original call completed, result
 *					updated in the previously supplied.
 *					struct optee_msg_arg
 * TEESMC_RETURN_RPC			Call suspended by RPC call to normal
 *					world.
 * TEESMC_RETURN_ERESUME		Resume failed, the opaque resume
 *					information was corrupt.
 */
#define TEESMC_FUNCID_RETURN_FROM_RPC	3
#define TEESMC32_CALL_RETURN_FROM_RPC \
	TEESMC_CALL_VAL(TEESMC_32, TEESMC_STD_CALL, TEESMC_OWNER_TRUSTED_OS, \
			TEESMC_FUNCID_RETURN_FROM_RPC)
/* Same as TEESMC32_CALL_RETURN_FROM_RPC but a "fast call". */
#define TEESMC32_FASTCALL_RETURN_FROM_RPC \
	TEESMC_CALL_VAL(TEESMC_32, TEESMC_STD_CALL, TEESMC_OWNER_TRUSTED_OS, \
			TEESMC_FUNCID_RETURN_FROM_RPC)

/*
 * Resume from RPC (for example after processing an IRQ)
 *
 * See description of TEESMC32_CALL_RETURN_FROM_RPC above, used when
 * it's a 64bit call that has returned.
 */
#define TEESMC64_CALL_RETURN_FROM_RPC \
	TEESMC_CALL_VAL(TEESMC_64, TEESMC_STD_CALL, TEESMC_OWNER_TRUSTED_OS, \
			TEESMC_FUNCID_RETURN_FROM_RPC)
/* Same as TEESMC64_CALL_RETURN_FROM_RPC but a "fast call". */
#define TEESMC64_FASTCALL_RETURN_FROM_RPC \
	TEESMC_CALL_VAL(TEESMC_64, TEESMC_STD_CALL, TEESMC_OWNER_TRUSTED_OS, \
			TEESMC_FUNCID_RETURN_FROM_RPC)

#define TEESMC_RETURN_RPC_PREFIX_MASK	0xFFFF0000
#define TEESMC_RETURN_RPC_PREFIX	0xFFFF0000
#define TEESMC_RETURN_RPC_FUNC_MASK	0x0000FFFF

#define TEESMC_RETURN_GET_RPC_FUNC(ret)	((ret) & TEESMC_RETURN_RPC_FUNC_MASK)

#define TEESMC_RPC_VAL(func)		((func) | TEESMC_RETURN_RPC_PREFIX)

/*
 * Allocate memory for RPC parameter passing. The memory is used to hold a
 * struct optee_msg_arg.
 *
 * "Call" register usage:
 * a0	This value, TEESMC_RETURN_RPC_ALLOC
 * a1	Size in bytes of required argument memory
 * a2	Not used
 * a3	Resume information, must be preserved
 * a4-5	Not used
 * a6-7	Resume information, must be preserved
 *
 * "Return" register usage:
 * a0	SMC Function ID, TEESMC32_CALL_RETURN_FROM_RPC.
 * a1	Upper 32 bits of 64-bit physical pointer to allocated
 *	memory, (a1 == 0 && a2 == 0) if size was 0 or if memory can't
 *	be allocated.
 * a2	Lower 32 bits of 64-bit physical pointer to allocated
 *	memory, (a1 == 0 && a2 == 0) if size was 0 or if memory can't
 *	be allocated
 * a3	Preserved
 * a4	Upper 32 bits of 64-bit Shared memory cookie used when freeing
 *	the memory or doing an RPC
 * a5	Lower 32 bits of 64-bit Shared memory cookie used when freeing
 *	the memory or doing an RPC
 * a6-7	Preserved
 */
#define TEESMC_RPC_FUNC_ALLOC_ARG	0
#define TEESMC_RETURN_RPC_ALLOC_ARG	\
	TEESMC_RPC_VAL(TEESMC_RPC_FUNC_ALLOC_ARG)

/*
 * Allocate payload memory for RPC parameter passing.
 * Payload memory is used to hold the memory referred to by struct
 * teesmc32_param_memref.
 *
 * "Call" register usage:
 * r0/x0	This value, TEESMC_RETURN_RPC_ALLOC
 * r1/x1	Size in bytes of required payload memory
 * r2-7/x2-7	Resume information, must be preserved
 *
 * "Return" register usage:
 * r0/x0	SMC Function ID, TEESMC32_CALL_RETURN_FROM_RPC if it was an
 *		AArch32 SMC return or TEESMC64_CALL_RETURN_FROM_RPC for
 *		AArch64 SMC return
 * r1/x1	Physical pointer to allocated payload memory, 0 if size
 *		was 0 or if memory can't be allocated
 * r2-7/x2-7	Preserved
 */
#define TEESMC_RPC_FUNC_ALLOC_PAYLOAD	1
#define TEESMC_RETURN_RPC_ALLOC_PAYLOAD	\
	TEESMC_RPC_VAL(TEESMC_RPC_FUNC_ALLOC_PAYLOAD)

/*
 * Free memory previously allocated by TEESMC_RETURN_RPC_ALLOC_ARG
 *
 * "Call" register usage:
 * a0	This value, TEESMC_RETURN_RPC_FREE
 * a1	Upper 32 bits of 64-bit shared memory cookie belonging to this
 *	argument memory
 * a2	Lower 32 bits of 64-bit shared memory cookie belonging to this
 *	argument memory
 * a3-7	Resume information, must be preserved
 *
 * "Return" register usage:
 * a0	SMC Function ID, OPTEE_SMC_CALL_RETURN_FROM_RPC.
 * a1-2	Not used
 * a3-7	Preserved
 */
#define TEESMC_RPC_FUNC_FREE_ARG	2
#define TEESMC_RETURN_RPC_FREE_ARG	TEESMC_RPC_VAL(TEESMC_RPC_FUNC_FREE_ARG)

/*
 * Free memory previously allocated by TEESMC_RETURN_RPC_ALLOC_PAYLOAD.
 *
 * "Call" register usage:
 * r0/x0	This value, TEESMC_RETURN_RPC_FREE
 * r1/x1	Physical pointer to previously allocated payload memory
 * r3-7/x3-7	Resume information, must be preserved
 *
 * "Return" register usage:
 * r0/x0	SMC Function ID, TEESMC32_CALL_RETURN_FROM_RPC if it was an
 *		AArch32 SMC return or TEESMC64_CALL_RETURN_FROM_RPC for
 *		AArch64 SMC return
 * r1-2/x1-2	Not used
 * r3-7/x3-7	Preserved
 */
#define TEESMC_RPC_FUNC_FREE_PAYLOAD	3
#define TEESMC_RETURN_RPC_FREE_PAYLOAD	\
	TEESMC_RPC_VAL(TEESMC_RPC_FUNC_FREE_PAYLOAD)

/*
 * Deliver an IRQ in normal world.
 *
 * "Call" register usage:
 * a0	OPTEE_SMC_RETURN_RPC_IRQ
 * a1-7	Resume information, must be preserved
 *
 * "Return" register usage:
 * a0	SMC Function ID, OPTEE_SMC_CALL_RETURN_FROM_RPC.
 * a1-7	Preserved
 */
#define TEESMC_RPC_FUNC_IRQ		4
#define TEESMC_RETURN_RPC_IRQ	TEESMC_RPC_VAL(TEESMC_RPC_FUNC_IRQ)

/*
 * Do an RPC request. The supplied struct optee_msg_arg tells which
 * request to do and the parameters for the request. The following fields
 * are used (the rest are unused):
 * - cmd		the Request ID
 * - ret		return value of the request, filled in by normal world
 * - num_params		number of parameters for the request
 * - params		the parameters
 * - param_attrs	attributes of the parameters
 *
 * "Call" register usage:
 * a0	OPTEE_SMC_RETURN_RPC_CMD
 * a1	Upper 32 bits of a 64-bit Shared memory cookie holding a
 *	struct optee_msg_arg, must be preserved, only the data should
 *	be updated
 * a2	Lower 32 bits of a 64-bit Shared memory cookie holding a
 *	struct optee_msg_arg, must be preserved, only the data should
 *	be updated
 * a3-7	Resume information, must be preserved
 *
 * "Return" register usage:
 * a0	SMC Function ID, OPTEE_SMC_CALL_RETURN_FROM_RPC.
 * a1-2	Not used
 * a3-7	Preserved
 */
#define TEESMC_RPC_FUNC_CMD		5
#define TEESMC_RETURN_RPC_CMD   TEESMC_RPC_VAL(TEESMC_RPC_FUNC_CMD)

/* Returned in a0 */
#define TEESMC_RETURN_UNKNOWN_FUNCTION 0xFFFFFFFF

/* Returned in a0 only from Trusted OS functions */
#define TEESMC_RETURN_OK		0x0
#define TEESMC_RETURN_ETHREAD_LIMIT	0x1
#define TEESMC_RETURN_EBUSY		0x2
#define TEESMC_RETURN_ERESUME	0x3
#define TEESMC_RETURN_EBADADDR	0x4
#define TEESMC_RETURN_EBADCMD	0x5
#define TEESMC_RETURN_ENOMEM	0x6
#define TEESMC_RETURN_ENOTAVAIL	0x7
#define TEESMC_RETURN_IS_RPC(ret) \
	(((ret) != TEESMC_RETURN_UNKNOWN_FUNCTION) && \
	((((ret) & TEESMC_RETURN_RPC_PREFIX_MASK) == \
		TEESMC_RETURN_RPC_PREFIX)))


// It's a little be archiac to not define proper types for these so we'll
// define them here to negate the need to drag "struct" around everywhere.
//
typedef struct teesmc_arg               t_teesmc_arg;
typedef struct teesmc_param             t_teesmc_param;
typedef struct teesmc_meta_open_session t_teesmc_meta_open_session;


#endif /* TEESMC_H */
