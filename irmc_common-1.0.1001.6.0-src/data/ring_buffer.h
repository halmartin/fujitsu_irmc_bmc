/*****************************************************************************
*
*                               NOTICE
*
*                COPYRIGHT 2010 - 2018 FUJITSU LIMITED
*                         ALL RIGHTS RESERVED
*
* Any reproduction of this program without the express written consent
* of Fujitsu Limited is a violation of the copyright laws and may
* subject you to civil liability and criminal prosecution.
*
* This computer program is CONFIDENTIAL and contains TRADE SECRETS of
* Fujitsu Technology Solutions and/or Fujitsu Limited which must not be disclosed.
* The receipt or possession of this program does not convey any rights to reproduce
* or disclose its contents, or to manufacture, use, or sell anything that it may
* describe, in whole or in part.
*
*****************************************************************************/
/**
 @file ring_buffer.h
 @brief Character ring buffer with timestamps API
 @date 2010-01-20
 @author Vlad Shakhov
 */

#ifndef _CL_RING_BUFFER_H_
#define _CL_RING_BUFFER_H_

#include <linux/types.h>

/*
 * SRAM is allocated for 24k for PILOT_IV
 * SRAM is allocated for 12k for PILOT_III
 * TODO: If PILOT_III is changed to LTS12 kernel in future,
 * then size verification need to be done again
 */
#define RINGBUF_MEMORY_ALLOCATE_SIZE                 24576
#ifdef SOC_PILOT_IV
#define SRAM_BASE_ADDRESS   0x10001000 /* SRAM Base Address + 5k */
#define RING_BUFFER_BASEADDRESS		SRAM_BASE_ADDRESS
#else
#define DRAM_MEMORY_ALLOCATE_SIZE		28672
#define RAM_MEMORY_START    CONFIG_SPX_FEATURE_GLOBAL_MEMORY_START + CONFIG_SPX_FEATURE_GLOBAL_MEMORY_SIZE - CONFIG_SPX_FEATURE_GLOBAL_PCIE_FUNCTION1_SHARED_MEM_SIZE - DRAM_MEMORY_ALLOCATE_SIZE
#define RING_BUFFER_BASEADDRESS		RAM_MEMORY_START
#endif
#ifndef IRMC_DEBUG_UART_BUFFER_SIZE
#define IRMC_DEBUG_UART_BUFFER_SIZE               RINGBUF_MEMORY_ALLOCATE_SIZE
#endif

void	irmc_cl_ring_buffer_init(void);
void	irmc_cl_ring_buffer_exit(void);
void 	irmc_cl_ring_buffer_clear(void);

void	irmc_cl_ring_buffer_put_char(char c);

ssize_t	irmc_cl_ring_buffer_read(char *buf, size_t buf_size);
void	irmc_cl_ring_buffer_open(void);
void	irmc_cl_ring_buffer_release(void);

void irmc_debug_uart_ring_buffer_put_char(char c);
void irmc_debug_uart_ring_buffer_init(void);
void irmc_debug_uart_ring_buffer_exit(void);

void irmc_debug_uart_lock(void);
void irmc_debug_uart_unlock(void);

char irmc_debug_uart_ring_buffer_get_char(void);
int irmc_debug_uart_ring_buffer_count(void);

#endif // _CL_RING_BUFFER_H_

