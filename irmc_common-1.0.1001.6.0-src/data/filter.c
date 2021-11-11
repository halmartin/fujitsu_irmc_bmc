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
 @file filter.c
 @brief Escape sequences filter
 @date 2012-02-03
 @author Maxim Gorbachyov
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

#include "filter.h"

/** Exclude pattern structure */
typedef struct {
	const char *pattern;
	unsigned patternLen;
	unsigned excludeRepeated;
} exclude_list_item_t;

typedef struct filter_tree_node {
	unsigned count;
	char *next;
	struct filter_tree_node **links;
	exclude_list_item_t *item;
} filter_tree_node_t;

/// 2 seconds timeout
#define ESC_SEQUENCE_TIMEOUT	(2*(HZ))

//@{ 
/// @name Escape Sequences Codes definitions

#define ASCII_ESC	0x1B

static const char clClearScreen[] = { ASCII_ESC, '[', '2', 'J' };
static const char clAltClearScreen[] = { ASCII_ESC, '[', 'H', ASCII_ESC, '[', 'J' };
static const char clExEscLeftparen[] = { ASCII_ESC, '(' };
static const char clExEscStop[] = { ASCII_ESC, 's', 't', 'o', 'p' };
static const char clExEscQ[] = { ASCII_ESC, 'Q' };
static const char clExEscREscrEscR[] = { ASCII_ESC, 'R', ASCII_ESC, 'r', ASCII_ESC, 'R' };
static const char clExEscCaret[] = { ASCII_ESC, '^' };
static const char clExEscBracketM[] = { ASCII_ESC, '[', 'm' };
static const char clExEscBracket1_1H[] = { ASCII_ESC, '[', '1', ';', '1', 'H' };
static const char clExEscBracket1_80H[] = { ASCII_ESC, '[', '1', ';', '8', '0', 'H' };
//@}

/** Max length of exclude pattern */
#define MAX_PATTERN_LEN	7

/** Exclude pattern list of items */
static exclude_list_item_t clExcludePatternList[] = {
	{ clExEscLeftparen, sizeof(clExEscLeftparen), 0 },
	{ clExEscStop, sizeof(clExEscStop), 0 },
	{ clExEscQ, sizeof(clExEscQ), 0 },
	{ clExEscREscrEscR, sizeof(clExEscREscrEscR), 0 },
	{ clExEscCaret, sizeof(clExEscCaret), 0 },
	{ clExEscBracketM, sizeof(clExEscBracketM), 1 },
	{ clExEscBracket1_1H, sizeof(clExEscBracket1_1H), 1 },
	{ clExEscBracket1_80H, sizeof(clExEscBracket1_80H), 1 },
	{ clClearScreen, sizeof(clClearScreen), 1 },
	{ clAltClearScreen, sizeof(clAltClearScreen), 1 }
};

/** Exclude pattern items in list */
#define EXCLUDE_PATTERN_ITEMS	10

/** Exclude item data matching structure */
static struct {
	char string[MAX_PATTERN_LEN];
	unsigned length;
	unsigned long timestamp;
} clExcludePattern;

static filter_tree_node_t *filter_root;
static filter_put_char_func_t put_char_callback;
static filter_tree_node_t *parent;
static exclude_list_item_t *last_found_item;

/// Linux kernel module parameter "filter_all"
unsigned consolelog_filter_all;

static filter_tree_node_t * alloc_filter_node(void)
{
	filter_tree_node_t *node;

	node = kmalloc(sizeof(filter_tree_node_t), GFP_KERNEL);
	if (node) {
		memset(node, 0, sizeof(*node));
	} else {
		printk(KERN_ERR "%s() failed to allocate memory\n", __FUNCTION__);
	}
	return node;
}

static void filter_tree_add_item(exclude_list_item_t *item)
{
	char ch, *next;
	unsigned i, j, found, count;
	filter_tree_node_t *node, *down;
	void *links_buf, *next_buf;

	if (!filter_root) {
		filter_root = alloc_filter_node();
		if (!filter_root)
			return;
	}

	node = filter_root;

	for (i = 1; i < item->patternLen; ++i) {
		ch = item->pattern[i];
		found = 0;
		next = node->next;
		count = node->count;
		for (j = 0; j < count; ++j) {
			if (next[j] == ch) {
				node = node->links[j];
				found = 1;
				break;
			}
		}

		if (found)
			continue;

		// no such sequence. create new downlink.
		links_buf = kmalloc((count + 1) * sizeof(filter_tree_node_t*), GFP_KERNEL);
		if (!links_buf) {
			printk(KERN_ERR "%s() failed to allocate memory\n", __FUNCTION__);
			return;
		}
		memcpy(links_buf, node->links, count * sizeof(filter_tree_node_t*));

		next_buf = kmalloc(count + 1, GFP_KERNEL);
		if (!next_buf) {
			kfree(links_buf);
			printk(KERN_ERR "%s() failed to allocate memory\n", __FUNCTION__);
			return;
		}
		memcpy(next_buf, node->next, count);

		down = alloc_filter_node();
		if (!down) {
			kfree(links_buf);
			kfree(next_buf);
			return;
		}
		down->item = item;

		kfree(node->links);
		node->links = links_buf;
		node->links[count] = down;

		kfree(node->next);
		node->next = next_buf;
		node->next[count] = ch;

		node->count++;

		node = down;
	}
}

void filter_init(filter_put_char_func_t func)
{
	unsigned i;

	memset(&clExcludePattern, 0, sizeof(clExcludePattern));
	put_char_callback = func;

	for (i = 0; i < EXCLUDE_PATTERN_ITEMS; i++)
		filter_tree_add_item(&clExcludePatternList[i]);
}

static void filter_tree_remove_node(filter_tree_node_t *node)
{
	filter_tree_node_t *down;

	if (!node)
		return;

	if (node->count) {
		unsigned i;

		for (i = 0; i < node->count; ++i) {
			down = node->links[i];
			filter_tree_remove_node(down);
		}

		kfree(node->links);
		kfree(node->next);
	}

	kfree(node);
}

void filter_free(void)
{
	filter_tree_remove_node(filter_root);
	filter_root = NULL;
}

/**
 @brief Incremental detect escape-sequences
 @param ch symbol to check
 @return false or true

 Warning: this function is statefull and modify the log buffer in some cases!
 */
int detect_exclude_pattern(char ch)
{
	int found = 0;
	int found_first_time = 0;

	if ((ch == ASCII_ESC) && (clExcludePattern.length == 0)) {
		clExcludePattern.string[clExcludePattern.length++] = ch;
		clExcludePattern.timestamp = jiffies;
		found = 1;
		parent = filter_root;
	} else if (consolelog_filter_all && (clExcludePattern.length > 0)) {
		// this branch was in ThreadX filtering, probably it's not needed now.

		clExcludePattern.string[clExcludePattern.length] = ch;

		// valid pattern only if interval between chars is less than 2 secs
		if (time_before(jiffies, clExcludePattern.timestamp + ESC_SEQUENCE_TIMEOUT)) {
			found = 1;
			clExcludePattern.timestamp = jiffies;
			if ((('a' <= ch) && (ch <= 'z')) || (('A' <= ch) && (ch <= 'Z'))) {
				clExcludePattern.length = 0;
			}
		}
	} else if (clExcludePattern.length > 0) {
		unsigned i;

		clExcludePattern.string[clExcludePattern.length++] = ch;

		// valid pattern only if interval between chars is less than 2 secs
		if (time_before(jiffies, clExcludePattern.timestamp + ESC_SEQUENCE_TIMEOUT)) {
			unsigned count;
			char *next;
			filter_tree_node_t *link;

			clExcludePattern.timestamp = jiffies;

			count = parent->count;
			next = parent->next;
			for (i = 0; i < count; ++i) {
				if (next[i] == ch) {
					found = 1;
					link = parent->links[i];
					if (link->count == 0) {
						// complete pattern is found
						if (link->item->excludeRepeated
						    && (link->item != last_found_item)) {
							found_first_time = 1;
						} else {
							// discard it
							clExcludePattern.length = 0;
						}
						last_found_item = link->item;
					}
					parent = link;
					break;
				}
			}
		}

		if (!found || found_first_time) {
			for (i = 0; i < clExcludePattern.length; ++i)
				put_char_callback(clExcludePattern.string[i]);

			clExcludePattern.length = 0;

			if (!found_first_time)
				last_found_item = NULL;

			// calling function should not handle current char
			found = 1;
		}
	}
	return found;
}

