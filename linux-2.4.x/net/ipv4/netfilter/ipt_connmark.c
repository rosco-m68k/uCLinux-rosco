/* Kernel module to match connection mark values. */
#include <linux/module.h>
#include <linux/skbuff.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_connmark.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>

static int
match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
      const void *matchinfo,
      int offset,
      const void *hdr,
      u_int16_t datalen,
      int *hotdrop)
{
	const struct ipt_connmark_info *info = matchinfo;
	enum ip_conntrack_info ctinfo;
	struct ip_conntrack *ct = ip_conntrack_get((struct sk_buff *)skb, &ctinfo);
	if (!ct)
	    return 0;

	return ((ct->mark & info->mask) == info->mark) ^ info->invert;
}

static int
checkentry(const char *tablename,
           const struct ipt_ip *ip,
           void *matchinfo,
           unsigned int matchsize,
           unsigned int hook_mask)
{
	if (matchsize != IPT_ALIGN(sizeof(struct ipt_connmark_info)))
		return 0;

	return 1;
}

static struct ipt_match connmark_match
= { { NULL, NULL }, "connmark", &match, &checkentry, NULL, THIS_MODULE };

static int __init init(void)
{
	return ipt_register_match(&connmark_match);
}

static void __exit fini(void)
{
	ipt_unregister_match(&connmark_match);
}

module_init(init);
module_exit(fini);
