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
 @file filter.h
 @brief Escape sequences filter API
 @date 2012-02-03
 @author Maxim Gorbachyov
 */

#ifndef _CL_FILTER_H_
#define _CL_FILTER_H_

extern unsigned consolelog_filter_all;

typedef void (*filter_put_char_func_t)(char);

void filter_init(filter_put_char_func_t);
void filter_free(void);
int detect_exclude_pattern(char ch);

#endif /* _CL_FILTER_H_ */

