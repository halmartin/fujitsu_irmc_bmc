--- linux_orig/include/uapi/linux/i2c-dev.h	2017-02-21 12:20:54.988079550 +0530
+++ linux/include/uapi/linux/i2c-dev.h	2017-02-21 12:25:34.835641891 +0530
@@ -65,6 +65,12 @@
 #define I2C_BUS_TEST            0x0850  /* Perform the I2C Bus test*/
 #define I2C_MCTP_READ           0x0851  /* Perform MCTP Pkt Read*/
 
+/* Extended IOCTLS - Implemented in i2c-core and hardware for Pilot */ 
+#define ENABLE_SSIF		0x0852 
+#define I2C_SYS_RESET   0x0853
+#define I2C_SET_BB_TIMEOUT  0x0854  /* set bus busy timeout */ 
+#define I2C_DISABLE_SLAVE   0x0855  /* Disable I2C port Slave Interface */
+#define I2C_RECOV_CHECK_MS  0x0856  /* Set I2C bus recovery stuck signal check time (in milliseconds) */
 
 /* This is the structure as used in the I2C_SMBUS ioctl call */
 struct i2c_smbus_ioctl_data {
