--- linux_old/net/ipv4/tcp_input.c	2016-09-07 15:58:44.629464085 +0530
+++ linux/net/ipv4/tcp_input.c	2016-09-07 16:57:59.560898929 +0530
@@ -87,7 +87,7 @@
 EXPORT_SYMBOL(sysctl_tcp_adv_win_scale);
 
 /* rfc5961 challenge ack rate limiting */
-int sysctl_tcp_challenge_ack_limit = 100;
+int sysctl_tcp_challenge_ack_limit = 1000;
 
 int sysctl_tcp_stdurg __read_mostly;
 int sysctl_tcp_rfc1337 __read_mostly;
@@ -121,6 +121,29 @@
 #define TCP_REMNANT (TCP_FLAG_FIN|TCP_FLAG_URG|TCP_FLAG_SYN|TCP_FLAG_PSH)
 #define TCP_HP_BITS (~(TCP_RESERVED_BITS|TCP_FLAG_PSH))
 
+typedef __u8  __attribute__((__may_alias__))  __u8_alias_t;
+typedef __u16 __attribute__((__may_alias__)) __u16_alias_t;
+typedef __u32 __attribute__((__may_alias__)) __u32_alias_t;
+typedef __u64 __attribute__((__may_alias__)) __u64_alias_t;
+
+static __always_inline void __read_once_size(const volatile void *p, void *res, int size)
+{
+        switch (size) {
+        case 1: *(__u8_alias_t  *) res = *(volatile __u8_alias_t  *) p; break;
+        case 2: *(__u16_alias_t *) res = *(volatile __u16_alias_t *) p; break;
+        case 4: *(__u32_alias_t *) res = *(volatile __u32_alias_t *) p; break;
+        case 8: *(__u64_alias_t *) res = *(volatile __u64_alias_t *) p; break;
+        default:
+                barrier();
+                __builtin_memcpy((void *)res, (const void *)p, size);
+                barrier();
+        }
+}
+
+#define WRITE_ONCE(x, val) x=(val)
+#define READ_ONCE(x) \
+        ({ typeof(x) __val; __read_once_size(&x, &__val, sizeof(__val)); __val; })
+
 /* Adapt the MSS value used to make delayed ack decision to the
  * real world.
  */
@@ -3296,12 +3319,16 @@
 	static u32 challenge_timestamp;
 	static unsigned int challenge_count;
 	u32 now = jiffies / HZ;
+	u32 count=0;
 
 	if (now != challenge_timestamp) {
+		u32 half = (sysctl_tcp_challenge_ack_limit + 1) >> 1;
 		challenge_timestamp = now;
-		challenge_count = 0;
+		WRITE_ONCE(challenge_count, half + prandom_u32_max(sysctl_tcp_challenge_ack_limit));
 	}
-	if (++challenge_count <= sysctl_tcp_challenge_ack_limit) {
+	count = READ_ONCE(challenge_count);
+	if (count > 0) {
+		WRITE_ONCE(challenge_count, count - 1);
 		NET_INC_STATS_BH(sock_net(sk), LINUX_MIB_TCPCHALLENGEACK);
 		tcp_send_ack(sk);
 	}
