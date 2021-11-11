/*
 *
 * pca954x.h - I2C multiplexer/switch support
 *
 * Copyright (c) 2008-2009 Rodolfo Giometti <giometti@linux.it>
 * Copyright (c) 2008-2009 Eurotech S.p.A. <info@eurotech.it>
 * Michael Lawnick <michael.lawnick.ext@nsn.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifndef _LINUX_I2C_PCA954X_H
#define _LINUX_I2C_PCA954X_H


#define PCA954X_MAX_NCHANS  8
#define PCA954X_MAX_COUNT	5
#define PCA9548_MAX_CHAN	8
#define PCA9546_MAX_CHAN	4
#define PCA9543_MAX_CHAN	2
#define PCA9544_MAX_CHAN	4

typedef struct {
	int status;
	u8 enabled;
	u8 bind;
	u8 bus;
    u8 rstGpio;
    u8 maxChan;
    u8 instance;
    u8 chanMask;
    u8 type;
    u8 vBusNum[PCA954X_MAX_NCHANS];
    struct i2c_client *client;
}tPca954xChip;

typedef enum {
	pca_9543 = 0,
	pca_9546,
	pca_9548,
	pca_9544,
	pca_unsuported
}ePcaType;

typedef struct {
	u8 type;
	u8 maxChn;
	u8 sizeOfName;
	u8 dummy;
	char name[12];
	int (*select_chan)   (struct i2c_adapter *, void *mux_dev, u32 chan_id);
	int (*deselect_chan) (struct i2c_adapter *, void *mux_dev, u32 chan_id);
}tMuxProp;

struct i2c_client *pca954x_addChip(u8 bus, struct i2c_board_info *board_info);
ePcaType pca954x_getType(char *name);
int pca954x_getMaxChan(ePcaType type);
void pca954x_removeChip(struct i2c_client *regI2Client);
void pca954x_reset(u8 pinNum);

#endif /* _LINUX_I2C_PCA954X_H */
