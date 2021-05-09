/*
 * Interface between the IPSEC code and the radix (radij) tree code
 * Copyright (C) 1996, 1997  John Ioannidis.
 * Copyright (C) 1998, 1999, 2000, 2001  Richard Guy Briggs.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * RCSID $Id: ipsec_radij.c,v 1.60 2002/02/19 23:59:45 rgb Exp $
 */

#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel.h> /* printk() */

#include "ipsec_param.h"

#ifdef MALLOC_SLAB
# include <linux/slab.h> /* kmalloc() */
#else /* MALLOC_SLAB */
# include <linux/malloc.h> /* kmalloc() */
#endif /* MALLOC_SLAB */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/interrupt.h> /* mark_bh */

#include <linux/netdevice.h>   /* struct device, struct net_device_stats and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/ip.h>          /* struct iphdr */
#include <linux/skbuff.h>
#include <freeswan.h>
#ifdef SPINLOCK
# ifdef SPINLOCK_23
#  include <linux/spinlock.h> /* *lock* */
# else /* 23_SPINLOCK */
#  include <asm/spinlock.h> /* *lock* */
# endif /* 23_SPINLOCK */
#endif /* SPINLOCK */
#ifdef NET_21
# include <asm/uaccess.h>
# include <linux/in6.h>
#endif
#include <asm/checksum.h>
#include <net/ip.h>

#include "ipsec_eroute.h"
#include "ipsec_sa.h"
 
#include "radij.h"
#include "ipsec_encap.h"
#include "radij.h"
#include "ipsec_encap.h"
#include "ipsec_radij.h"
#include "ipsec_netlink.h"
#include "ipsec_tunnel.h"	/* struct ipsecpriv */
#include "ipsec_xform.h"
 
#include <pfkeyv2.h>
#include <pfkey.h>

#include "ipsec_proto.h"

#ifdef CONFIG_IPSEC_DEBUG
int debug_radij = 0;
#endif /* CONFIG_IPSEC_DEBUG */

struct radij_node_head *rnh = NULL;
#ifdef SPINLOCK
spinlock_t eroute_lock = SPIN_LOCK_UNLOCKED;
#else /* SPINLOCK */
spinlock_t eroute_lock;
#endif /* SPINLOCK */

int
ipsec_radijinit(void)
{
	maj_keylen = sizeof (struct sockaddr_encap);

	rj_init();
	
	if (rj_inithead((void **)&rnh, /*16*/offsetof(struct sockaddr_encap, sen_type) * sizeof(__u8)) == 0) /* 16 is bit offset of sen_type */
		return -1;
	return 0;
}

int
ipsec_radijcleanup(void)
{
	int error;

	spin_lock_bh(&eroute_lock);

	error = radijcleanup();

	spin_unlock_bh(&eroute_lock);

	return error;
}

int
ipsec_cleareroutes(void)
{
	int error = 0;

	spin_lock_bh(&eroute_lock);

	error = radijcleartree();

	spin_unlock_bh(&eroute_lock);

	return error;
}

int
ipsec_breakroute(struct sockaddr_encap *eaddr,
		 struct sockaddr_encap *emask,
		 struct sk_buff **first,
		 struct sk_buff **last)
{
	struct eroute *ro;
	struct radij_node *rn;
	int error = 0;
#ifdef CONFIG_IPSEC_DEBUG
	char buf1[64], buf2[64];
	
	if (debug_eroute) {
		subnettoa(eaddr->sen_ip_src, emask->sen_ip_src, 0, buf1, sizeof(buf1));
		subnettoa(eaddr->sen_ip_dst, emask->sen_ip_dst, 0, buf2, sizeof(buf2));
		KLIPS_PRINT(debug_eroute, 
			    "klips_debug:ipsec_breakroute: "
			    "attempting to delete eroute for %s->%s\n",
			    buf1,
			    buf2);
	}
#endif /* CONFIG_IPSEC_DEBUG */

	spin_lock_bh(&eroute_lock);

	if ((error = rj_delete(eaddr, emask, rnh, &rn)) != 0) {
		spin_unlock_bh(&eroute_lock);
		KLIPS_PRINT(debug_eroute, 
			    "klips_debug:ipsec_breakroute: "
			    "node not found, eroute delete failed.\n");
		return error;
	}

	spin_unlock_bh(&eroute_lock);
	
	ro = (struct eroute *)rn;
	
	KLIPS_PRINT(debug_eroute, 
		    "klips_debug:ipsec_breakroute: "
		    "deleted eroute=%p, ident=%p->%p, first=%p, last=%p\n",
		    ro,
		    ro->er_ident_s.data,
		    ro->er_ident_d.data,
		    ro->er_first,
		    ro->er_last);
	
	if (ro->er_ident_s.data != NULL) {
		kfree(ro->er_ident_s.data);
	}
	if (ro->er_ident_d.data != NULL) {
		kfree(ro->er_ident_d.data);
	}
	if (ro->er_first != NULL) {
#if 0
		struct net_device_stats *stats = (struct net_device_stats *) &(((struct ipsecpriv *)(ro->er_first->dev->priv))->mystats);
		stats->tx_dropped--;
#endif
		*first = ro->er_first;
	}
	if (ro->er_last != NULL) {
#if 0
		struct net_device_stats *stats = (struct net_device_stats *) &(((struct ipsecpriv *)(ro->er_last->dev->priv))->mystats);
		stats->tx_dropped--;
#endif
		*last = ro->er_last;
	}
	
	if (rn->rj_flags & (RJF_ACTIVE | RJF_ROOT))
		panic ("ipsec_breakroute RMT_DELEROUTE root or active node\n");
	
	memset((caddr_t)rn, 0, sizeof (struct eroute));
	kfree(rn);
	
	return 0;
}

int
ipsec_makeroute(struct sockaddr_encap *eaddr,
		struct sockaddr_encap *emask,
		struct sa_id said,
		uint32_t pid,
		struct sk_buff *skb,
		struct ident *ident_s,
		struct ident *ident_d)
{
	struct eroute *retrt;
	int error = 0;
	char sa[SATOA_BUF];
	size_t sa_len;
#ifdef CONFIG_IPSEC_DEBUG
	char buf1[64], buf2[64];
	
	if (debug_eroute) {
		subnettoa(eaddr->sen_ip_src, emask->sen_ip_src, 0, buf1, sizeof(buf1));
		subnettoa(eaddr->sen_ip_dst, emask->sen_ip_dst, 0, buf2, sizeof(buf2));
		sa_len = satoa(said, 0, sa, SATOA_BUF);
		KLIPS_PRINT(debug_eroute, 
			    "klips_debug:ipsec_makeroute: "
			    "attempting to insert eroute for %s->%s, SA: %s, PID:%d, skb=%p, ident:%s->%s\n",
			    buf1,
			    buf2,
			    sa_len ? sa : " (error)",
			    pid,
			    skb,
			    (ident_s ? (ident_s->data ? ident_s->data : "NULL") : "NULL"),
			    (ident_d ? (ident_d->data ? ident_d->data : "NULL") : "NULL"));
	}
#endif /* CONFIG_IPSEC_DEBUG */

	retrt = (struct eroute *)kmalloc(sizeof (struct eroute), GFP_ATOMIC);
	if (retrt == NULL) {
		printk("klips_error:ipsec_makeroute: "
		       "not able to allocate kernel memory");
		return -ENOMEM;
	}
	memset((caddr_t)retrt, 0, sizeof (struct eroute));

	retrt->er_eaddr = *eaddr;
	retrt->er_emask = *emask;
	retrt->er_said = said;
	retrt->er_pid = pid;
	retrt->er_count = 0;
	retrt->er_lasttime = jiffies/HZ;
	rd_key((&(retrt->er_rjt))) = &(retrt->er_eaddr);
	
	if (ident_s && ident_s->type != SADB_IDENTTYPE_RESERVED) {
		int data_len = ident_s->len * IPSEC_PFKEYv2_ALIGN - sizeof(struct sadb_ident);
		
		retrt->er_ident_s.type = ident_s->type;
		retrt->er_ident_s.id = ident_s->id;
		retrt->er_ident_s.len = ident_s->len;
		if(data_len) {
			if(!(retrt->er_ident_s.data = kmalloc(data_len, GFP_KERNEL))) {
				kfree(retrt);
				printk("klips_error:ipsec_makeroute: not able to allocate kernel memory (%d)\n", data_len);
				return ENOMEM;
			}
			memcpy(retrt->er_ident_s.data, ident_s->data, data_len);
		} else {
			retrt->er_ident_s.data = NULL;
		}
	}
	
	if (ident_d && ident_d->type != SADB_IDENTTYPE_RESERVED) {
		int data_len = ident_d->len  * IPSEC_PFKEYv2_ALIGN - sizeof(struct sadb_ident);
		
		retrt->er_ident_d.type = ident_d->type;
		retrt->er_ident_d.id = ident_d->id;
		retrt->er_ident_d.len = ident_d->len;
		if(data_len) {
			if(!(retrt->er_ident_d.data = kmalloc(data_len, GFP_KERNEL))) {
				if (retrt->er_ident_s.data)
					kfree(retrt->er_ident_s.data);
				kfree(retrt);
				printk("klips_error:ipsec_makeroute: not able to allocate kernel memory (%d)\n", data_len);
				return ENOMEM;
			}
			memcpy(retrt->er_ident_d.data, ident_d->data, data_len);
		} else {
			retrt->er_ident_d.data = NULL;
		}
	}
	retrt->er_first = skb;
	retrt->er_last = NULL;
	
	spin_lock_bh(&eroute_lock);
	
	error = rj_addroute(&(retrt->er_eaddr), &(retrt->er_emask), 
			 rnh, retrt->er_rjt.rd_nodes);

	spin_unlock_bh(&eroute_lock);
	
	if(error) {
		sa_len = satoa(said, 0, sa, SATOA_BUF);
		KLIPS_PRINT(debug_eroute, 
			    "klips_debug:ipsec_makeroute: "
			    "rj_addroute not able to insert eroute for SA:%s\n",
			    sa_len ? sa : " (error)");
		if (retrt->er_ident_s.data)
			kfree(retrt->er_ident_s.data);
		if (retrt->er_ident_d.data)
			kfree(retrt->er_ident_d.data);
		
		kfree(retrt);
		
		return error;
	}

#ifdef CONFIG_IPSEC_DEBUG
	if (debug_eroute && 0) {
/*
		subnettoa(eaddr->sen_ip_src, emask->sen_ip_src, 0, buf1, sizeof(buf1));
		subnettoa(eaddr->sen_ip_dst, emask->sen_ip_dst, 0, buf2, sizeof(buf2));
*/
		subnettoa(rd_key((&(retrt->er_rjt)))->sen_ip_src, rd_mask((&(retrt->er_rjt)))->sen_ip_src, 0, buf1, sizeof(buf1));
		subnettoa(rd_key((&(retrt->er_rjt)))->sen_ip_dst, rd_mask((&(retrt->er_rjt)))->sen_ip_dst, 0, buf2, sizeof(buf2));
		sa_len = satoa(retrt->er_said, 0, sa, SATOA_BUF);
		
		KLIPS_PRINT(debug_eroute,
			    "klips_debug:ipsec_makeroute: "
			    "pid=%05d "
			    "count=%10d "
			    "lasttime=%6d "
			    "%-18s -> %-18s => %s\n",
			    retrt->er_pid,
			    retrt->er_count,
			    (int)(jiffies/HZ - retrt->er_lasttime),
			    buf1,
			    buf2,
			    sa_len ? sa : " (error)");
	}
#endif /* CONFIG_IPSEC_DEBUG */
	KLIPS_PRINT(debug_eroute,
		    "klips_debug:ipsec_makeroute: "
		    "succeeded, I think...\n");
	return 0;
}

struct eroute *
ipsec_findroute(struct sockaddr_encap *eaddr)
{
	struct radij_node *rn;
#ifdef CONFIG_IPSEC_DEBUG
	char buf1[ADDRTOA_BUF], buf2[ADDRTOA_BUF];
	
	if (debug_radij & DB_RJ_FINDROUTE) {
		addrtoa(eaddr->sen_ip_src, 0, buf1, sizeof(buf1));
		addrtoa(eaddr->sen_ip_dst, 0, buf2, sizeof(buf2));
		KLIPS_PRINT(debug_eroute,
			    "klips_debug:ipsec_findroute: "
			    "%s->%s\n",
			    buf1,
			    buf2);
	}
#endif /* CONFIG_IPSEC_DEBUG */
	rn = rj_match((caddr_t)eaddr, rnh);
	if(rn) {
		KLIPS_PRINT(debug_eroute,
			    "klips_debug:ipsec_findroute: "
			    "found, points to proto=%d, spi=%x, dst=%x.\n",
			    ((struct eroute*)rn)->er_said.proto,
			    ntohl(((struct eroute*)rn)->er_said.spi),
			    ntohl(((struct eroute*)rn)->er_said.dst.s_addr));
	}
	return (struct eroute *)rn;
}
		
#ifdef CONFIG_PROC_FS
int
ipsec_rj_walker_procprint(struct radij_node *rn, void *w0)
{
	struct eroute *ro = (struct eroute *)rn;
	struct rjtentry *rd = (struct rjtentry *)rn;
	struct wsbuf *w = (struct wsbuf *)w0;
	char buf1[64], buf2[64];
	char sa[SATOA_BUF];
	size_t sa_len;
	struct sockaddr_encap *key, *mask;
	
	KLIPS_PRINT(debug_radij,
		    "klips_debug:ipsec_rj_walker_procprint: "
		    "rn=%p, w0=%p\n",
		    rn,
		    w0);
	if (rn == NULL)	{
		return 120;
	}
	
	if (rn->rj_b >= 0) {
		return 0;
	}
	
	key = rd_key(rd);
	mask = rd_mask(rd);
	
	if ((key == 0) || (mask == 0)) {
		return 0;
	}

	subnettoa(key->sen_ip_src, mask->sen_ip_src, 0, buf1, sizeof(buf1));
	subnettoa(key->sen_ip_dst, mask->sen_ip_dst, 0, buf2, sizeof(buf2));
	sa_len = satoa(ro->er_said, 0, sa, SATOA_BUF);
#define IPSEC_EROUTE_IDENT_
#ifndef IPSEC_EROUTE_IDENT
	w->len += sprintf(w->buffer + w->len,
/*
			  "%05d "
*/
			  "%-10d "
/*
			  "%6d "
*/
			  "%-18s -> %-18s => %s\n",
/*
			  ro->er_pid,
*/
			  ro->er_count,
/*
			  jiffies / HZ - ro->er_lasttime,
*/
			  buf1,
			  buf2,
			  sa_len ? sa : " (error)");
#else /* IPSEC_EROUTE_IDENT */
	w->len += sprintf(w->buffer + w->len,
/*
			  "%05d "
*/
			  "%-10d "
/*
			  "%6d "
*/
			  "%-18s -> %-18s => %s (%s) (%s)\n",
/*
			  ro->er_pid,
*/
			  ro->er_count,
/*
			  jiffies / HZ - ro->er_lasttime,
*/
			  buf1,
			  buf2,
			  sa_len ? sa : " (error)",
			  (ro->er_ident_s.data ? ro->er_ident_s.data : ""),
			  (ro->er_ident_d.data ? ro->er_ident_d.data : ""));
#endif /* IPSEC_EROUTE_IDENT */
	
	w->pos = w->begin + w->len;
	if(w->pos < w->offset) {
		w->len = 0;
		w->begin = w->pos;
	}
	if (w->pos > w->offset + w->length) {
		return -ENOBUFS;
	}
	return 0;
}
#endif          /* CONFIG_PROC_FS */

int
ipsec_rj_walker_delete(struct radij_node *rn, void *w0)
{
	struct eroute *ro;
	struct rjtentry *rd = (struct rjtentry *)rn;
	struct radij_node *rn2;
	int error = 0;
	struct sockaddr_encap *key, *mask;
#ifdef CONFIG_IPSEC_DEBUG
	char buf1[64] = { 0 }, buf2[64] = { 0 };
#endif /* CONFIG_IPSEC_DEBUG */

	if (rn == NULL)	{
		return 120;
	}
	
	key = rd_key(rd);
	mask = rd_mask(rd);
	
	if(!key || !mask) {
		return -ENODATA;
	}
#ifdef CONFIG_IPSEC_DEBUG
	if(debug_radij)	{
		subnettoa(key->sen_ip_src, mask->sen_ip_src, 0, buf1, sizeof(buf1));
		subnettoa(key->sen_ip_dst, mask->sen_ip_dst, 0, buf2, sizeof(buf2));
		KLIPS_PRINT(debug_radij, 
			    "klips_debug:ipsec_rj_walker_delete: "
			    "deleting: %s -> %s\n",
			    buf1,
			    buf2);
	}
#endif /* CONFIG_IPSEC_DEBUG */

	if((error = rj_delete(key, mask, rnh, &rn2))) {
		KLIPS_PRINT(debug_radij,
			    "klips_debug:ipsec_rj_walker_delete: "
			    "rj_delete failed with error=%d.\n", error);
		return error;
	}

	if(rn2 != rn) {
		printk("klips_debug:ipsec_rj_walker_delete: "
		       "tried to delete a different node?!?  This should never happen!\n");
	}
 
	ro = (struct eroute *)rn;
	
	if (ro->er_ident_s.data)
		kfree(ro->er_ident_s.data);
	if (ro->er_ident_d.data)
		kfree(ro->er_ident_d.data);
	
	memset((caddr_t)rn, 0, sizeof (struct eroute));
	kfree(rn);
	
	return 0;
}

/*
 * $Log: ipsec_radij.c,v $
 * Revision 1.60  2002/02/19 23:59:45  rgb
 * Removed redundant compiler directives.
 *
 * Revision 1.59  2002/02/06 04:13:47  mcr
 * 	missing #ifdef CONFIG_IPSEC_DEBUG.
 *
 * Revision 1.58  2002/01/29 17:17:56  mcr
 * 	moved include of ipsec_param.h to after include of linux/kernel.h
 * 	otherwise, it seems that some option that is set in ipsec_param.h
 * 	screws up something subtle in the include path to kernel.h, and
 * 	it complains on the snprintf() prototype.
 *
 * Revision 1.57  2002/01/29 04:00:52  mcr
 * 	more excise of kversions.h header.
 *
 * Revision 1.56  2002/01/29 02:13:17  mcr
 * 	introduction of ipsec_kversion.h means that include of
 * 	ipsec_param.h must preceed any decisions about what files to
 * 	include to deal with differences in kernel source.
 *
 * Revision 1.55  2001/11/26 09:23:48  rgb
 * Merge MCR's ipsec_sa, eroute, proc and struct lifetime changes.
 *
 * Revision 1.53.2.1  2001/09/25 02:26:32  mcr
 * 	headers adjusted for new usage.
 *
 * Revision 1.54  2001/10/18 04:45:20  rgb
 * 2.4.9 kernel deprecates linux/malloc.h in favour of linux/slab.h,
 * lib/freeswan.h version macros moved to lib/kversions.h.
 * Other compiler directive cleanups.
 *
 * Revision 1.53  2001/09/19 17:19:40  rgb
 * Debug output bugfix for NetCelo's PF_KEY ident patch.
 *
 * Revision 1.52  2001/09/19 16:33:37  rgb
 * Temporarily disable ident fields to /proc/net/ipsec_eroute.
 *
 * Revision 1.51  2001/09/15 16:24:04  rgb
 * Re-inject first and last HOLD packet when an eroute REPLACE is done.
 *
 * Revision 1.50  2001/09/14 16:58:36  rgb
 * Added support for storing the first and last packets through a HOLD.
 *
 * Revision 1.49  2001/09/08 21:13:32  rgb
 * Added pfkey ident extension support for ISAKMPd. (NetCelo)
 *
 * Revision 1.48  2001/06/15 04:12:56  rgb
 * Fixed kernel memory allocation error return code polarity bug.
 *
 * Revision 1.47  2001/06/14 19:35:09  rgb
 * Update copyright date.
 *
 * Revision 1.46  2001/06/08 08:47:18  rgb
 * Fixed for debug disabled.
 *
 * Revision 1.45  2001/05/27 06:12:11  rgb
 * Added structures for pid, packet count and last access time to eroute.
 * Added packet count to beginning of /proc/net/ipsec_eroute.
 *
 * Revision 1.44  2001/05/03 19:41:01  rgb
 * Initialise error return variable.
 * Use more appropriate return value for ipsec_rj_walker_delete().
 *
 * Revision 1.43  2001/02/27 22:24:54  rgb
 * Re-formatting debug output (line-splitting, joining, 1arg/line).
 * Check for satoa() return codes.
 *
 * Revision 1.42  2001/02/27 06:21:57  rgb
 * Added findroute success instrumentation.
 *
 * Revision 1.41  2000/11/06 04:32:08  rgb
 * Ditched spin_lock_irqsave in favour of spin_lock_bh.
 *
 * Revision 1.40  2000/09/08 19:12:56  rgb
 * Change references from DEBUG_IPSEC to CONFIG_IPSEC_DEBUG.
 *
 * Revision 1.39  2000/08/30 05:25:20  rgb
 * Correct debug text in ipsec_breakroute() from incorrect
 * "ipsec_callback".
 *
 * Revision 1.38  2000/07/28 14:58:31  rgb
 * Changed kfree_s to kfree, eliminating extra arg to fix 2.4.0-test5.
 *
 * Revision 1.37  2000/03/16 14:02:50  rgb
 * Fixed debug scope to enable compilation with debug off.
 *
 * Revision 1.36  2000/01/21 06:14:46  rgb
 * Added debugging text to ipsec_rj_walker_delete().
 * Set return code to negative for consistency.
 *
 * Revision 1.35  1999/11/23 23:05:24  rgb
 * Use provided macro ADDRTOA_BUF instead of hardcoded value.
 *
 * Revision 1.34  1999/11/18 04:13:56  rgb
 * Replaced all kernel version macros to shorter, readable form.
 * Added CONFIG_PROC_FS compiler directives in case it is shut off.
 *
 * Revision 1.33  1999/11/17 15:53:39  rgb
 * Changed all occurrences of #include "../../../lib/freeswan.h"
 * to #include <freeswan.h> which works due to -Ilibfreeswan in the
 * klips/net/ipsec/Makefile.
 *
 * Revision 1.32  1999/10/26 13:58:33  rgb
 * Put spinlock flags variable declaration outside the debug compiler
 * directive to enable compilation with debug shut off.
 *
 * Revision 1.31  1999/10/15 22:13:29  rgb
 * Clean out cruft.
 * Align /proc/net/ipsec_eroute output for easier readability.
 * Fix double linefeed in radij debug output.
 * Fix double locking bug that locks up 2.0.36 but not 2.0.38.
 *
 * Revision 1.30  1999/10/08 18:37:33  rgb
 * Fix end-of-line spacing to sate whining PHMs.
 *
 * Revision 1.29  1999/10/03 18:52:45  rgb
 * Spinlock support for 2.0.xx.
 * Dumb return code spin_unlock fix.
 *
 * Revision 1.28  1999/10/01 16:22:24  rgb
 * Switch from assignment init. to functional init. of spinlocks.
 *
 * Revision 1.27  1999/10/01 15:44:53  rgb
 * Move spinlock header include to 2.1> scope.
 *
 * Revision 1.26  1999/10/01 00:01:23  rgb
 * Added eroute structure locking.
 *
 * Revision 1.25  1999/06/10 16:07:30  rgb
 * Silence delete eroute on no debug.
 *
 * Revision 1.24  1999/05/09 03:25:36  rgb
 * Fix bug introduced by 2.2 quick-and-dirty patch.
 *
 * Revision 1.23  1999/05/05 22:02:31  rgb
 * Add a quick and dirty port to 2.2 kernels by Marc Boucher <marc@mbsi.ca>.
 *
 * Revision 1.22  1999/04/29 15:17:23  rgb
 * Add return values to init and cleanup functions.
 * Add sanity checking for null pointer arguments.
 *
 * Revision 1.21  1999/04/11 00:28:58  henry
 * GPL boilerplate
 *
 * Revision 1.20  1999/04/06 04:54:26  rgb
 * Fix/Add RCSID Id: and Log: bits to make PHMDs happy.  This includes
 * patch shell fixes.
 *
 * Revision 1.19  1999/02/17 16:50:35  rgb
 * Clean out unused cruft.
 * Consolidate for space and speed efficiency.
 * Convert DEBUG_IPSEC to KLIPS_PRINT
 *
 * Revision 1.18  1999/01/22 06:22:06  rgb
 * Cruft clean-out.
 * 64-bit clean-up.
 *
 * Revision 1.17  1998/12/02 03:09:39  rgb
 * Clean up debug printing conditionals to compile with debugging off.
 *
 * Revision 1.16  1998/12/01 13:49:39  rgb
 * Wrap version info printing in debug switches.
 *
 * Revision 1.15  1998/11/30 13:22:54  rgb
 * Rationalised all the klips kernel file headers.  They are much shorter
 * now and won't conflict under RH5.2.
 *
 * Revision 1.14  1998/10/31 06:48:17  rgb
 * Fixed up comments in #endif directives.
 *
 * Revision 1.13  1998/10/27 13:48:09  rgb
 * Cleaned up /proc/net/ipsec_* filesystem for easy parsing by scripts.
 * Fixed less(1) truncated output bug.
 * Code clean-up.
 *
 * Revision 1.12  1998/10/25 02:41:36  rgb
 * Change return type on ipsec_breakroute and ipsec_makeroute and add an
 * argument to be able to transmit more infomation about errors.
 * Fix cut-and-paste debug statement identifier.
 *
 * Revision 1.11  1998/10/22 06:45:39  rgb
 * Cleaned up cruft.
 * Convert to use satoa for printk.
 *
 * Revision 1.10  1998/10/19 14:44:28  rgb
 * Added inclusion of freeswan.h.
 * sa_id structure implemented and used: now includes protocol.
 *
 * Revision 1.9  1998/10/09 04:30:52  rgb
 * Added 'klips_debug' prefix to all klips printk debug statements.
 * Deleted old commented out cruft.
 *
 * Revision 1.8  1998/08/06 17:24:23  rgb
 * Fix addrtoa return code bug from stale manpage advice preventing packets
 * from being erouted.
 *
 * Revision 1.7  1998/08/06 07:44:59  rgb
 * Fixed /proc/net/ipsec_eroute subnettoa and addrtoa return value bug that
 * ended up in nothing being printed.
 *
 * Revision 1.6  1998/08/05 22:16:41  rgb
 * Cleanup to prevent cosmetic errors (ie. debug output) from being fatal.
 *
 * Revision 1.5  1998/07/29 20:38:44  rgb
 * Debug and fix subnettoa and addrtoa output.
 *
 * Revision 1.4  1998/07/28 00:02:39  rgb
 * Converting to exclusive use of addrtoa.
 * Fix eroute delete.
 *
 * Revision 1.3  1998/07/14 18:21:26  rgb
 * Add function to clear the eroute table.
 *
 * Revision 1.2  1998/06/23 02:59:14  rgb
 * Added debugging output to eroute add/delete routines.
 *
 * Revision 1.9  1998/06/18 21:29:06  henry
 * move sources from klips/src to klips/net/ipsec, to keep stupid kernel
 * build scripts happier in presence of symbolic links
 *
 * Revision 1.8  1998/06/05 02:32:26  rgb
 * Fix spi ntoh kernel debug output.
 *
 * Revision 1.7  1998/05/25 20:30:37  rgb
 * Remove temporary ipsec_walk, rj_deltree and rj_delnodes functions.
 *
 * Rename ipsec_rj_walker (ipsec_walk) to ipsec_rj_walker_procprint and
 * add ipsec_rj_walker_delete.
 *
 * Revision 1.6  1998/05/21 13:08:57  rgb
 * Rewrote procinfo subroutines to avoid *bad things* when more that 3k of
 * information is available for printout.
 *
 * Revision 1.5  1998/05/18 21:35:55  rgb
 * Clean up output for numerical consistency and readability.  Zero freed
 * eroute memory.
 *
 * Revision 1.4  1998/04/21 21:28:58  rgb
 * Rearrange debug switches to change on the fly debug output from user
 * space.  Only kernel changes checked in at this time.  radij.c was also
 * changed to temporarily remove buggy debugging code in rj_delete causing
 * an OOPS and hence, netlink device open errors.
 *
 * Revision 1.3  1998/04/14 17:30:39  rgb
 * Fix up compiling errors for radij tree memory reclamation.
 *
 * Revision 1.2  1998/04/12 22:03:23  rgb
 * Updated ESP-3DES-HMAC-MD5-96,
 * 	ESP-DES-HMAC-MD5-96,
 * 	AH-HMAC-MD5-96,
 * 	AH-HMAC-SHA1-96 since Henry started freeswan cvs repository
 * from old standards (RFC182[5-9] to new (as of March 1998) drafts.
 *
 * Fixed eroute references in /proc/net/ipsec*.
 *
 * Started to patch module unloading memory leaks in ipsec_netlink and
 * radij tree unloading.
 *
 * Revision 1.1  1998/04/09 03:06:10  henry
 * sources moved up from linux/net/ipsec
 *
 * Revision 1.1.1.1  1998/04/08 05:35:03  henry
 * RGB's ipsec-0.8pre2.tar.gz ipsec-0.8
 *
 * Revision 0.4  1997/01/15 01:28:15  ji
 * No changes.
 *
 * Revision 0.3  1996/11/20 14:39:04  ji
 * Minor cleanups.
 * Rationalized debugging code.
 *
 * Revision 0.2  1996/11/02 00:18:33  ji
 * First limited release.
 *
 *
 */
