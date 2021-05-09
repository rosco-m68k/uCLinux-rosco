/* This is a module which is used for setting/remembering the mark field of
 * an connection, or optionally restore it to the skb
 */
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/checksum.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_CONNMARK.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>

static unsigned int
target(struct sk_buff **pskb,
       unsigned int hooknum,
       const struct net_device *in,
       const struct net_device *out,
       const void *targinfo,
       void *userinfo)
{
	const struct ipt_connmark_target_info *markinfo = targinfo;

	enum ip_conntrack_info ctinfo;
	struct ip_conntrack *ct = ip_conntrack_get((*pskb), &ctinfo);
	if (ct) {
	    switch(markinfo->mode) {
	    case IPT_CONNMARK_SET:
		ct->mark = markinfo->mark;
		break;
	    case IPT_CONNMARK_SAVE:
		ct->mark = (*pskb)->nfmark;
		break;
	    case IPT_CONNMARK_RESTORE:
		if (ct->mark != (*pskb)->nfmark) {
		    (*pskb)->nfmark = ct->mark;
		    (*pskb)->nfcache |= NFC_ALTERED;
		}
		break;
	    }
	}

	return IPT_CONTINUE;
}

static int
checkentry(const char *tablename,
	   const struct ipt_entry *e,
           void *targinfo,
           unsigned int targinfosize,
           unsigned int hook_mask)
{
	struct ipt_connmark_target_info *matchinfo = targinfo;
	if (targinfosize != IPT_ALIGN(sizeof(struct ipt_connmark_target_info))) {
		printk(KERN_WARNING "CONNMARK: targinfosize %u != %Zu\n",
		       targinfosize,
		       IPT_ALIGN(sizeof(struct ipt_connmark_target_info)));
		return 0;
	}

	if (matchinfo->mode == IPT_CONNMARK_RESTORE) {
	    if (strcmp(tablename, "mangle") != 0) {
		    printk(KERN_WARNING "CONNMARK: restore can only be called from \"mangle\" table, not \"%s\"\n", tablename);
		    return 0;
	    }
	}

	return 1;
}

static struct ipt_target ipt_connmark_reg
= { { NULL, NULL }, "CONNMARK", target, checkentry, NULL, THIS_MODULE };

static int __init init(void)
{
	if (ipt_register_target(&ipt_connmark_reg))
		return -EINVAL;

	return 0;
}

static void __exit fini(void)
{
	ipt_unregister_target(&ipt_connmark_reg);
}

module_init(init);
module_exit(fini);
