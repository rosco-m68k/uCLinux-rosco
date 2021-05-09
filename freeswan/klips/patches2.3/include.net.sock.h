--- ./include/net/sock.h	2002/02/06 15:25:10	1.1
+++ ./include/net/sock.h	2002/05/22 12:14:56
@@ -245,6 +245,12 @@
 	__u32	end_seq;
 };
 
+#if 1
+struct udp_opt {
+	__u32 esp_in_udp;
+};
+#endif
+
 struct tcp_opt {
 	int	tcp_header_len;	/* Bytes of tcp header to send		*/
 
@@ -584,6 +590,9 @@
 #if defined(CONFIG_SPX) || defined (CONFIG_SPX_MODULE)
 		struct spx_opt		af_spx;
 #endif /* CONFIG_SPX */
+#if 1
+		struct udp_opt 		af_udp;
+#endif
 
 	} tp_pinfo;
 
