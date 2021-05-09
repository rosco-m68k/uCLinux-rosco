RCSID $Id: net.ipv4.af_inet.c,v 1.7 1999/12/31 15:15:21 rgb Exp $
--- ./net/ipv4/af_inet.c.preipsec	Wed Jun  3 18:17:50 1998
+++ ./net/ipv4/af_inet.c	Fri Sep 17 10:14:12 1999
@@ -1146,6 +1146,17 @@
 	ip_alias_init();
 #endif
 
+#if defined(CONFIG_IPSEC)
+	{
+               extern /* void */ int ipsec_init(void);
+		/*
+		 *  Initialise AF_INET ESP and AH protocol support including 
+		 *  e-routing and SA tables
+		 */
+		ipsec_init();
+	}
+#endif /* CONFIG_IPSEC */
+
 #ifdef CONFIG_INET_RARP
 	rarp_ioctl_hook = rarp_ioctl;
 #endif
