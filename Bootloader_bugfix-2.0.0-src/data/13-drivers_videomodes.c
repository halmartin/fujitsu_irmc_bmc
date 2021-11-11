--- uboot/drivers/videomodes.c	2006-11-02 09:15:01.000000000 -0500
+++ uboot/drivers/videomodes.c	2012-03-23 10:29:02.604684236 -0400
@@ -156,7 +156,7 @@
 int video_get_params (struct ctfb_res_modes *pPar, char *penv)
 {
 	char *p, *s, *val_s;
-	int i = 0, t;
+	int i = 0/*, t*/;
 	int bpp;
 	int mode;
 	/* first search for the environment containing the real param string */
@@ -174,7 +174,7 @@
 	}
 	/* search for mode as a default value */
 	p = s;
-	t = 0;
+//	t = 0;
 	mode = 0;		/* default */
 	while ((i = video_get_param_len (p, ',')) != 0) {
 		GET_OPTION ("mode:", mode)
