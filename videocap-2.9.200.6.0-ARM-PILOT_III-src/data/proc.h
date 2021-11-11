/*****************************************************************
 **                                                             **
 **     (C) Copyright 2009-2015, American Megatrends Inc.       **
 **                                                             **
 **             All Rights Reserved.                            **
 **                                                             **
 **         5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                             **
 **         Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                             **
 ****************************************************************/

#ifndef AMI_PROC_H
#define AMI_PROC_H
#include<linux/version.h>

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int DumpPalette(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int INCTSEColumnSize(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int INCTSERowSize(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int INCTSEMaxRectangles(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int DECTSEColumnSize(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int DECTSERowSize(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int DECTSEMaxRectangles(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int ReadDriverStatus(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int ReadDriverInfo(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int ReadDriverStatistics(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int SetAutoCalibrateMode(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int OddRawBufDump(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int EvenRawBufDump(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int MoveLeft(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int MoveRight(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int MoveUp(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int MoveDown(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int DumpRegisters(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int DumpVideo(struct file *file,  char __user *buf, size_t count, loff_t *offset);
int DumpTfeBuff(struct file *file,  char __user *buf, size_t count, loff_t *offset);
#else
int DumpPalette(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int INCTSEColumnSize(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int INCTSERowSize(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int INCTSEMaxRectangles(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int DECTSEColumnSize(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int DECTSERowSize(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int DECTSEMaxRectangles(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int ReadDriverStatus(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int ReadDriverInfo(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int ReadDriverStatistics(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int SetAutoCalibrateMode(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int OddRawBufDump(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int EvenRawBufDump(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int MoveLeft(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int MoveRight(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int MoveUp(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int MoveDown(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int DumpRegisters(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int DumpVideo(char *buf, char **start, off_t offset, int count, int *eof, void *data);
int DumpTfeBuff(char *buf, char **start, off_t offset, int count, int *eof, void *data);
#endif

#endif // AMI_PROC_H
