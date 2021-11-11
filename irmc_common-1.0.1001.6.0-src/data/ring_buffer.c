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
 @file ring_buffer.c
 @brief Character ring buffer with timestamps implementation
 @date 2010-01-20
 @author Vlad Shakhov
 */
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <asm/io.h>
#include "filter.h"
#include "ring_buffer.h"
#define IRMC_SRAM_MAGIC_WORD       "TESTSRAM"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
#define init_MUTEX(sem)	 sema_init(sem, 1)
#endif

#define IRMC_CL_CHUNKS 2048
#define IRMC_CL_CHUNK_SIZE 16

typedef struct {
	char data[IRMC_CL_CHUNK_SIZE];
	size_t size;
	struct timeval tv;
	size_t read_pos;
} irmc_cl_chunk_t;

static irmc_cl_chunk_t chunks[IRMC_CL_CHUNKS];
static unsigned write_index;
static unsigned read_index;
static int write_enabled;
/*
 * Structure define to use SRAM
 */
typedef struct {
	char irmc_magic_word [9];
	char data [IRMC_DEBUG_UART_BUFFER_SIZE];
	unsigned int uart_read_index;
	unsigned int uart_write_index;
} irmc_debug_buffer;

irmc_debug_buffer *irmc_sram_buffer;
/*
 * SRAM Pointer to copy UART's Tx/Rx
 */
char *irmc_debug_uart_buffer = NULL;
static unsigned debug_uart_write_index = 0;
static unsigned debug_uart_read_index = 0;
static int debug_uart_write_enabled = 0;
static struct 	semaphore debug_uart_sem;

void irmc_cl_ring_buffer_clear(void)
{
	memset(&chunks, 0, sizeof(chunks));
	write_index = 0;
	read_index = 0;
}

static void clear_chunk(irmc_cl_chunk_t *chunk)
{
	memset(chunk, 0, sizeof(*chunk));
}

static void irmc_cl_ring_buffer_put_char_raw(char c)
{
	irmc_cl_chunk_t *chunk;

	chunk = &chunks[write_index];

	if (chunk->size == IRMC_CL_CHUNK_SIZE) {
		++write_index;
		if (write_index == IRMC_CL_CHUNKS)
			write_index = 0;
		chunk = &chunks[write_index];
		clear_chunk(chunk);
	}

	chunk->data[chunk->size] = c;
	chunk->size++;
	do_gettimeofday(&chunk->tv);
}

void irmc_cl_ring_buffer_init(void)
{
	filter_init(irmc_cl_ring_buffer_put_char_raw);
	irmc_cl_ring_buffer_clear();
	write_enabled = 1;
}

void irmc_cl_ring_buffer_exit(void)
{
	write_enabled = 0;
	filter_free();
}

/**
 @brief Put (write) symbol to log
 @param ch character to store

 Also check for the escape sequences.

 It's wrapper around put_char_raw()
 to allow the recursive using put_char_raw()
 in escape sequences filter.
 */
void irmc_cl_ring_buffer_put_char(char ch)
{
	if (!write_enabled)
		return;

	if (!detect_exclude_pattern(ch))
		irmc_cl_ring_buffer_put_char_raw(ch);
}

#define TTYREC_HEADER_SIZE 12

static ssize_t do_read(char *buf, size_t buf_size, struct timeval *tv, char *src, size_t size)
{
	uint32_t header[3];
	int i;

	if (size == 0)
		return 0;

	if ((TTYREC_HEADER_SIZE + size) > buf_size) {
		printk(KERN_ERR "%s() buffer is not enough, need at least %u bytes\n",
			__FUNCTION__, TTYREC_HEADER_SIZE + size);
		return -ENOMEM;
	}

	// prepare ttyrec header:
	header[0] = tv->tv_sec;
	header[1] = tv->tv_usec;
	header[2] = size;
	for (i = 0; i < 3; ++i)
		header[i] = cpu_to_le32(header[i]);

	memcpy(buf, header, TTYREC_HEADER_SIZE);
	memcpy(buf + TTYREC_HEADER_SIZE, src, size);

	return TTYREC_HEADER_SIZE + size;
}

ssize_t irmc_cl_ring_buffer_read(char *buf, size_t buf_size)
{
	irmc_cl_chunk_t *chunk;
	ssize_t ret;
	char *src;
	size_t size;

	chunk = &chunks[read_index];

	src = chunk->data + chunk->read_pos;
	size = chunk->size - chunk->read_pos;

	ret = do_read(buf, buf_size, &chunk->tv, src, size);
	if (ret > 0) {
		chunk->read_pos += size;
		if ((chunk->read_pos == IRMC_CL_CHUNK_SIZE) && (read_index != write_index)) {
			++read_index;
			if (read_index == IRMC_CL_CHUNKS)
				read_index = 0;
		}
	}

	return ret;
}

// find oldest chunk with data
void irmc_cl_ring_buffer_open(void)
{
	unsigned i;

	write_enabled = 0;

	read_index = write_index;

	do {
		++read_index;
		if (read_index == IRMC_CL_CHUNKS)
			read_index = 0;
		if (read_index == write_index)
			break;
	} while (chunks[read_index].size == 0);

	for (i = 0; i < IRMC_CL_CHUNKS; ++i)
		chunks[i].read_pos = 0;
}

void irmc_cl_ring_buffer_release(void)
{
	write_enabled = 1;
}

/**
 * @brief put character into ring buffer for iRMC Debug Console logging
 * @param	c - character to be added
 */
void irmc_debug_uart_ring_buffer_put_char(char c)
{
	if(debug_uart_write_enabled){

		if(!down_trylock(&debug_uart_sem)){
			irmc_debug_uart_buffer[debug_uart_write_index] = c;

			if(++debug_uart_write_index >= IRMC_DEBUG_UART_BUFFER_SIZE){
				debug_uart_write_index = 0;
			}

			if(debug_uart_write_index == debug_uart_read_index){
				if(++debug_uart_read_index >= IRMC_DEBUG_UART_BUFFER_SIZE){
					debug_uart_read_index = 0;
				}
			}
			irmc_sram_buffer->uart_read_index = debug_uart_read_index;
			irmc_sram_buffer->uart_write_index = debug_uart_write_index;
			irmc_debug_uart_unlock();
		}
	}
}

/// @brief enter critical section
void irmc_debug_uart_lock(void)
{
	down(&debug_uart_sem);
}

///  @brief leave critical section
void irmc_debug_uart_unlock(void)
{
	up(&debug_uart_sem);
}

/**
 * @brief initialize ring buffer, semaphore and variables used
 *			for iRMC Debug Console logging
 *	ring buffer is initialized from SRAM memory (Replaced Array)
 *	MAGIC WORD is used to identify whether 
 *	read/write ptrs are retained or initialized to defaults
 */
void irmc_debug_uart_ring_buffer_init(void)
{
	void __iomem *sram_virtual_memory = NULL;
	
	/* Using IOremap no cache to get SRAM memory */
	sram_virtual_memory = ioremap_nocache (RING_BUFFER_BASEADDRESS, sizeof(irmc_debug_buffer));
	if (sram_virtual_memory == NULL)
	{
		printk(KERN_EMERG "Failed to map SRAM/RAM %08x to virtual", RING_BUFFER_BASEADDRESS);
		return;
	}
	
	irmc_sram_buffer = (irmc_debug_buffer *)sram_virtual_memory;

	/* Check for MAGIC Word (TESTRAM) present */	
	if ( strcmp(irmc_sram_buffer->irmc_magic_word, IRMC_SRAM_MAGIC_WORD) == 0)
	{
		/* If Present, Retrieve read/write index from memory */
		debug_uart_read_index = irmc_sram_buffer->uart_read_index;
		debug_uart_write_index = irmc_sram_buffer->uart_write_index;
		printk (KERN_INFO "%s : SRAM/RAM Magicword found - Read Index - %d - Write Index - %d", __FUNCTION__, debug_uart_read_index, debug_uart_write_index);
	}
	else
	{
		/* If not present, Prepare to start using memory */
		printk (KERN_INFO "%s : SRAM/RAM Magic word not found in SRAM\n", __FUNCTION__);
		printk (KERN_INFO "%s : Flushing SRAM/RAM memory before using for logging\n", __FUNCTION__);
		memset ((char *) irmc_sram_buffer, 0, sizeof (irmc_debug_buffer));
		strcpy (irmc_sram_buffer->irmc_magic_word, IRMC_SRAM_MAGIC_WORD);
	}
	/* SRAM Address Data pointer for iRMC debug logs */
	irmc_debug_uart_buffer = (char *)irmc_sram_buffer->data;
	
	/* Initialize the mutex */
	init_MUTEX(&debug_uart_sem);
	
	/* Enable uart logging */
	debug_uart_write_enabled = 1;
}

// @brief exit function for iRMC Debug Console logging
void irmc_debug_uart_ring_buffer_exit(void)
{
	debug_uart_write_enabled = 0;
	iounmap(irmc_sram_buffer);
	printk (KERN_INFO "%s :RING Buffer iounmap done", __FUNCTION__);
}

// @brief pop character out of ring buffer for iRMC Debug Console logging
char irmc_debug_uart_ring_buffer_get_char(void)
{
	int next_read_index = 0;
	char ret = 0;
	
	if(debug_uart_write_index == debug_uart_read_index){
		//there is no new data
	} else {
		ret = irmc_debug_uart_buffer[debug_uart_read_index];
		
		next_read_index = debug_uart_read_index + 1;
		if(next_read_index >= IRMC_DEBUG_UART_BUFFER_SIZE){
			next_read_index = 0;
		}
		debug_uart_read_index = next_read_index;
		irmc_sram_buffer->uart_read_index = debug_uart_read_index;
	}

	return ret;
}

/**
 * @brief get character count for iRMC Debug Console logging
 * @return	int - value equal to elements between read and write pointers
 */

int irmc_debug_uart_ring_buffer_count(void)
{
	if(debug_uart_write_index == debug_uart_read_index){
		return 0;
	} else if (debug_uart_write_index > debug_uart_read_index){
		return (debug_uart_write_index - debug_uart_read_index);
	} else {
		return ((IRMC_DEBUG_UART_BUFFER_SIZE - debug_uart_read_index) + debug_uart_write_index);
	}
}

