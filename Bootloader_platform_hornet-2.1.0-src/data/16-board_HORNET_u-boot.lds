--- u-boot-1.1.6/board/HORNET/u-boot.lds	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/board/HORNET/u-boot.lds	2010-04-05 16:47:24.000000000 -0400
@@ -0,0 +1,75 @@
+/*
+ * (C) Copyright 2000
+ * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
+ *
+ * See file CREDITS for list of people who contributed to this
+ * project.
+ *
+ * This program is free software; you can redistribute it and/or
+ * modify it under the terms of the GNU General Public License as
+ * published by the Free Software Foundation; either version 2 of
+ * the License, or (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program; if not, write to the Free Software
+ * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
+ * MA 02111-1307 USA
+ */
+
+OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
+OUTPUT_ARCH(arm)
+ENTRY(_start)
+SECTIONS
+{
+	. = 0x00000000;
+
+	. = ALIGN(4);
+	.text      :
+	{
+	  cpu/arm926ejs/start.o	(.text)
+ 	  cpu/arm926ejs/libarm926ejs.a(.text)
+          cpu/arm926ejs/PILOTIII/libPILOTIII.a(.text)
+	  board/HORNET/libHORNET.a (.text)
+	  lib_arm/libarm.a (.text)
+	  lib_generic/libgeneric.a (.text)
+	  net/libnet.a (.text)
+	}
+
+	. = 0x87F0FF80;
+
+	.fmh 	:
+	{
+		board/HORNET/fmh.o (.bss)
+	} 
+
+	. = 0x87F10000;
+
+	.text2 		:
+	{
+	  *(.text)
+	}
+
+	. = ALIGN(4);
+	.rodata : { *(.rodata) }
+
+	. = ALIGN(4);
+	.data : { *(.data) }
+
+	. = ALIGN(4);
+	.got : { *(.got) }
+
+	__u_boot_cmd_start = .;
+	.u_boot_cmd : { *(.u_boot_cmd) }
+	__u_boot_cmd_end = .;
+
+
+	. = ALIGN(4);
+	__bss_start = .;
+	.bss : { *(.bss) }
+	_end = .;
+}
