--- u-boot-1.1.6/net/net.c	2014-04-08 10:09:28.708507918 +0800
+++ u-boot-1.1.6-ami/net/net.c	2014-04-08 10:09:44.440507945 +0800
@@ -174,6 +174,10 @@
 extern void NCSI_Start(void);
 #endif
 
+#ifdef CONFIG_SIDEBAND_SUPPORT
+extern void SIDEBAND_Start(void);
+#endif
+
 // Added for AMI Extension - R2C
 extern void R2C_Initiate(char * Ether);
 extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
@@ -469,6 +473,12 @@
 			NetState = NETLOOP_SUCCESS;		
 			break;
 #endif
+#ifdef CONFIG_SIDEBAND_SUPPORT
+		case SIDEBAND:
+			SIDEBAND_Start();
+			NetState = NETLOOP_SUCCESS;		
+			break;
+#endif
 		default:
 			break;
 		}
@@ -578,6 +588,7 @@
 			return NetBootFileXferSize;
 
 		case NETLOOP_FAIL:
+			eth_halt();		/* AMI : Bug Fix */
 			return (-1);
 		}
 	}
