--- uboot/board/HORNET/config.mk	1969-12-31 19:00:00.000000000 -0500
+++ uboot.ok/board/HORNET/config.mk	2010-05-21 14:23:42.137302125 -0400
@@ -0,0 +1,26 @@
+#
+# (C) Copyright 2000
+# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
+#
+# See file CREDITS for list of people who contributed to this
+# project.
+#
+# This program is free software; you can redistribute it and/or
+# modify it under the terms of the GNU General Public License as
+# published by the Free Software Foundation; either version 2 of
+# the License, or (at your option) any later version.
+#
+# This program is distributed in the hope that it will be useful,
+# but WITHOUT ANY WARRANTY; without even the implied warranty of
+# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+# GNU General Public License for more details.
+#
+# You should have received a copy of the GNU General Public License
+# along with this program; if not, write to the Free Software
+# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
+# MA 02111-1307 USA
+#
+
+PLATFORM_RELFLAGS += -gdwarf-2
+#TEXT_BASE  = 0x01F00000
+TEXT_BASE = 0x87F00000
