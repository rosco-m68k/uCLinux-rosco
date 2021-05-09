/*
 * manipulate eroutes
 * Copyright (C) 1996  John Ioannidis.
 * Copyright (C) 1997, 1998, 1999, 2000, 2001  Richard Guy Briggs.
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
 */

char eroute_c_version[] = "RCSID $Id: eroute.c,v 1.49 2002/03/08 21:44:04 rgb Exp $";


#include <sys/types.h>
#include <linux/types.h> /* new */
#include <string.h>
#include <errno.h>
#include <stdlib.h> /* system(), strtoul() */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>


#include <unistd.h>
#include <freeswan.h>
#include <signal.h>
#include <pfkeyv2.h>
#include <pfkey.h>
#include "radij.h"
#include "ipsec_encap.h"
#include "ipsec_netlink.h"


#include <stdio.h>
#include <getopt.h>

char *program_name;
char me[] = "ipsec eroute";
extern char *optarg;
extern int optind, opterr, optopt;
char *eroute_af_opt, *said_af_opt, *edst_opt, *spi_opt, *proto_opt, *said_opt, *dst_opt, *src_opt;
int action_type = 0;

extern unsigned int pfkey_lib_debug; /* used by libfreeswan/pfkey_v2_build */
int pfkey_sock;
fd_set pfkey_socks;
uint32_t pfkey_seq = 0;

static void
usage(char* arg)
{
	fprintf(stdout, "usage: %s --{add,addin,replace} --eraf <inet | inet6> --src <src>/<srcmaskbits>|<srcmask> --dst <dst>/<dstmaskbits>|<dstmask> <SA>\n", arg);
	fprintf(stdout, "            where <SA> is '--af <inet | inet6> --edst <edst> --spi <spi> --proto <proto>'\n");
	fprintf(stdout, "                       OR '--said <said>'\n");
	fprintf(stdout, "                       OR '--said <%%passthrough | %%passthrough4 | %%passthrough6 | %%drop | %%reject | %%trap | %%hold | %%pass>'.\n");
	fprintf(stdout, "       %s --del --eraf <inet | inet6>--src <src>/<srcmaskbits>|<srcmask> --dst <dst>/<dstmaskbits>|<dstmask>\n", arg);
	fprintf(stdout, "       %s --clear\n", arg);
	fprintf(stdout, "       %s --help\n", arg);
	fprintf(stdout, "       %s --version\n", arg);
	fprintf(stdout, "       %s\n", arg);
	fprintf(stdout, "        [ --debug ] is optional to any %s command.\n", arg);
	fprintf(stdout, "        [ --label <label> ] is optional to any %s command.\n", arg);
exit(1);
}

static struct option const longopts[] =
{
	{"dst", 1, 0, 'D'},
	{"src", 1, 0, 'S'},
	{"eraf", 1, 0, 'f'},
	{"add", 0, 0, 'a'},
	{"addin", 0, 0, 'A'},
	{"replace", 0, 0, 'r'},
	{"clear", 0, 0, 'c'},
	{"del", 0, 0, 'd'},
	{"af", 1, 0, 'i'},
	{"edst", 1, 0, 'e'},
	{"proto", 1, 0, 'p'},
	{"help", 0, 0, 'h'},
	{"spi", 1, 0, 's'},
	{"said", 1, 0, 'I'},
	{"version", 0, 0, 'v'},
	{"label", 1, 0, 'l'},
	{"optionsfrom", 1, 0, '+'},
	{"debug", 0, 0, 'g'},
	{0, 0, 0, 0}
};

int
main(int argc, char **argv)
{
/*	int fd; */
	char *endptr;
/*	int ret; */
	int c, previous = -1;
	const char* error_s;
	int debug = 0;

	int error = 0;

	char ipaddr_txt[ADDRTOT_BUF];                
	struct sadb_ext *extensions[SADB_EXT_MAX + 1];
	struct sadb_msg *pfkey_msg;
	ip_address pfkey_address_s_ska;
	/*struct sockaddr_in pfkey_address_d_ska;*/
	ip_address pfkey_address_sflow_ska;
	ip_address pfkey_address_dflow_ska;
	ip_address pfkey_address_smask_ska;
	ip_address pfkey_address_dmask_ska;

	ip_said said;
	ip_subnet s_subnet, d_subnet;
 	int eroute_af = 0;
 	int said_af = 0;

	int argcount = argc;

	program_name = argv[0];
	eroute_af_opt = said_af_opt = edst_opt = spi_opt = proto_opt = said_opt = dst_opt = src_opt = NULL;

	while((c = getopt_long(argc, argv, ""/*"acdD:e:i:hprs:S:f:vl:+:g"*/, longopts, 0)) != EOF) {
		switch(c) {
		case 'g':
			debug = 1;
			pfkey_lib_debug = 1;
			argcount--;
			break;
		case 'a':
			if(action_type) {
				fprintf(stderr, "%s: Only one of '--add', '--addin', '--replace', '--clear', or '--del' options permitted.\n",
					program_name);
				exit(1);
			}
			action_type = EMT_SETEROUTE;
			break;
		case 'A':
			if(action_type) {
				fprintf(stderr, "%s: Only one of '--add', '--addin', '--replace', '--clear', or '--del' options permitted.\n",
					program_name);
				exit(1);
			}
			action_type = EMT_INEROUTE;
			break;
		case 'r':
			if(action_type) {
				fprintf(stderr, "%s: Only one of '--add', '--addin', '--replace', '--clear', or '--del' options permitted.\n",
					program_name);
				exit(1);
			}
			action_type = EMT_REPLACEROUTE;
			break;
		case 'c':
			if(action_type) {
				fprintf(stderr, "%s: Only one of '--add', '--addin', '--replace', '--clear', or '--del' options permitted.\n",
					program_name);
				exit(1);
			}
			action_type = EMT_CLREROUTE;
			break;
		case 'd':
			if(action_type) {
				fprintf(stderr, "%s: Only one of '--add', '--addin', '--replace', '--clear', or '--del' options permitted.\n",
					program_name);
				exit(1);
			}
			action_type = EMT_DELEROUTE;
			break;
		case 'e':
			if(said_opt) {
				fprintf(stderr, "%s: Error, EDST parameter redefined:%s, already defined in SA:%s\n",
					program_name, optarg, said_opt);
				exit (1);
			}				
			if(edst_opt) {
				fprintf(stderr, "%s: Error, EDST parameter redefined:%s, already defined as:%s\n",
					program_name, optarg, edst_opt);
				exit (1);
			}				
			error_s = ttoaddr(optarg, 0, said_af, &said.dst);
			if(error_s != NULL) {
				fprintf(stderr, "%s: Error, %s converting --edst argument:%s\n",
					program_name, error_s, optarg);
				exit (1);
			}
			edst_opt = optarg;
			break;
		case 'h':
		case '?':
			usage(program_name);
			exit(1);
		case 's':
			if(said_opt) {
				fprintf(stderr, "%s: Error, SPI parameter redefined:%s, already defined in SA:%s\n",
					program_name, optarg, said_opt);
				exit (1);
			}				
			if(spi_opt) {
				fprintf(stderr, "%s: Error, SPI parameter redefined:%s, already defined as:%s\n",
					program_name, optarg, spi_opt);
				exit (1);
			}				
			said.spi = htonl(strtoul(optarg, &endptr, 0));
			if(!(endptr == optarg + strlen(optarg))) {
				fprintf(stderr, "%s: Invalid character in SPI parameter: %s\n",
					program_name, optarg);
				exit (1);
			}
			if(ntohl(said.spi) < 0x100) {
				fprintf(stderr, "%s: Illegal reserved spi: %s => 0x%lx Must be larger than 0x100.\n",
					program_name, optarg, ntohl(said.spi));
				exit(1);
			}
			spi_opt = optarg;
			break;
		case 'p':
			if(said_opt) {
				fprintf(stderr, "%s: Error, PROTO parameter redefined:%s, already defined in SA:%s\n",
					program_name, optarg, said_opt);
				exit (1);
			}				
			if(proto_opt) {
				fprintf(stderr, "%s: Error, PROTO parameter redefined:%s, already defined as:%s\n",
					program_name, optarg, proto_opt);
				exit (1);
			}
#if 0
			if(said.proto) {
				fprintf(stderr, "%s: Warning, PROTO parameter redefined:%s\n",
					program_name, optarg);
				exit (1);
			}
#endif
			if(!strcmp(optarg, "ah"))
				said.proto = SA_AH;
			if(!strcmp(optarg, "esp"))
				said.proto = SA_ESP;
			if(!strcmp(optarg, "tun"))
				said.proto = SA_IPIP;
			if(!strcmp(optarg, "comp"))
				said.proto = SA_COMP;
			if(said.proto == 0) {
				fprintf(stderr, "%s: Invalid PROTO parameter: %s\n",
					program_name, optarg);
				exit (1);
			}
			proto_opt = optarg;
			break;
		case 'I':
			if(said_opt) {
				fprintf(stderr, "%s: Error, SAID parameter redefined:%s, already defined in SA:%s\n",
					program_name, optarg, said_opt);
				exit (1);
			}				
			if(proto_opt) {
				fprintf(stderr, "%s: Error, PROTO parameter redefined in SA:%s, already defined as:%s\n",
					program_name, optarg, proto_opt);
				exit (1);
			}
			if(edst_opt) {
				fprintf(stderr, "%s: Error, EDST parameter redefined in SA:%s, already defined as:%s\n",
					program_name, optarg, edst_opt);
				exit (1);
			}
			if(spi_opt) {
				fprintf(stderr, "%s: Error, SPI parameter redefined in SA:%s, already defined as:%s\n",
					program_name, optarg, spi_opt);
				exit (1);
			}
			if(said_af_opt) {
				fprintf(stderr, "%s: Error, address family parameter redefined in SA:%s, already defined as:%s\n",
					program_name, optarg, said_af_opt);
				exit (1);
			}
			error_s = ttosa(optarg, 0, &said);
			if(error_s != NULL) {
				fprintf(stderr, "%s: Error, %s converting --sa argument:%s\n",
					program_name, error_s, optarg);
				exit (1);
			} else if(ntohl(said.spi) < 0x100){
				fprintf(stderr, "%s: Illegal reserved spi: %s => 0x%x Must be larger than or equal to 0x100.\n",
					program_name, optarg, said.spi);
				exit(1);
			}
			said_af = addrtypeof(&said.dst);
			said_opt = optarg;
			break;
		case 'v':
			fprintf(stdout, "%s %s\n", me, ipsec_version_code());
			fprintf(stdout, "See `ipsec --copyright' for copyright information.\n");
			exit(1);
		case 'D':
			if(dst_opt) {
				fprintf(stderr, "%s: Error, --dst parameter redefined:%s, already defined as:%s\n",
					program_name, optarg, dst_opt);
				exit (1);
			}				
			error_s = ttosubnet(optarg, 0, eroute_af, &d_subnet);
			if (error_s != NULL) {
				fprintf(stderr, "%s: Error, %s converting --dst argument: %s\n",
					program_name, error_s, optarg);
				exit (1);
			}
			dst_opt = optarg;
			break;
		case 'S':
			if(src_opt) {
				fprintf(stderr, "%s: Error, --src parameter redefined:%s, already defined as:%s\n",
					program_name, optarg, src_opt);
				exit (1);
			}				
			error_s = ttosubnet(optarg, 0, eroute_af, &s_subnet);
			if (error_s != NULL) {
				fprintf(stderr, "%s: Error, %s converting --src argument: %s\n",
					program_name, error_s, optarg);
				exit (1);
			}
			src_opt = optarg;
			break;
		case 'l':
			program_name = malloc(strlen(argv[0])
					      + 10 /* update this when changing the sprintf() */
					      + strlen(optarg));
			sprintf(program_name, "%s --label %s",
				argv[0],
				optarg);
			argcount -= 2;
			break;
		case 'i': /* specifies the address family of the SAID, stored in said_af */
			if(said_af_opt) {
				fprintf(stderr, "%s: Error, address family of SAID redefined:%s, already defined as:%s\n",
					program_name, optarg, said_af_opt);
				exit (1);
			}				
			if(!strcmp(optarg, "inet"))
				said_af = AF_INET;
			if(!strcmp(optarg, "inet6"))
				said_af = AF_INET6;
			if(said_af == 0) {
				fprintf(stderr, "%s: Invalid address family parameter for SAID: %s\n",
					program_name, optarg);
				exit (1);
			}
			said_af_opt = optarg;
			break;
		case 'f': /* specifies the address family of the eroute, stored in eroute_af */
			if(eroute_af_opt) {
				fprintf(stderr, "%s: Error, address family of eroute redefined:%s, already defined as:%s\n",
					program_name, optarg, eroute_af_opt);
				exit (1);
			}				
			if(!strcmp(optarg, "inet"))
				eroute_af = AF_INET;
			if(!strcmp(optarg, "inet6"))
				eroute_af = AF_INET6;
			if(eroute_af == 0) {
				fprintf(stderr, "%s: Invalid address family parameter for eroute: %s\n",
					program_name, optarg);
				exit (1);
			}
			eroute_af_opt = optarg;
			break;
		case '+': /* optionsfrom */
			optionsfrom(optarg, &argc, &argv, optind, stderr);
			/* no return on error */
			break;
		default:
		}
		previous = c;
	}

	if(debug) {
		fprintf(stdout, "%s: DEBUG: argc=%d\n", program_name, argc);
	}
	
	if(argcount == 1) {
		system("cat /proc/net/ipsec_eroute");
		exit(0);
	}

	/* Sanity checks */

	if(debug) {
		fprintf(stdout, "%s: DEBUG: action_type=%d\n", program_name, action_type);
	}

	switch(action_type) {
	case EMT_SETEROUTE:
	case EMT_REPLACEROUTE:
	case EMT_INEROUTE:
		if(!(said_af_opt && edst_opt && spi_opt && proto_opt) && !(said_opt)) {
			fprintf(stderr, "%s: add and addin options must have SA specified.\n",
				program_name);
			exit(1);
		}
	case EMT_DELEROUTE:
		if(!src_opt) {
			fprintf(stderr, "%s: Error -- %s option '--src' is required.\n",
				program_name, (action_type == EMT_SETEROUTE) ? "add" : "del");
			exit(1);
		}
		if(!dst_opt) {
			fprintf(stderr, "%s: Error -- %s option '--dst' is required.\n",
				program_name, (action_type == EMT_SETEROUTE) ? "add" : "del");
			exit(1);
		}
	case EMT_CLREROUTE:
		break;
	default:
		fprintf(stderr, "%s: exactly one of '--add', '--addin', '--replace', '--del' or '--clear' options must be specified.\n"
			"Try %s --help' for usage information.\n",
			program_name, program_name);
		exit(1);
	}

	if((pfkey_sock = socket(PF_KEY, SOCK_RAW, PF_KEY_V2) ) < 0) {
		fprintf(stderr, "%s: Trouble openning PF_KEY family socket with error: ",
			program_name);
		switch(errno) {
		case ENOENT:
			fprintf(stderr, "device does not exist.  See FreeS/WAN installation procedure.\n");
			break;
		case EACCES:
			fprintf(stderr, "access denied.  ");
			if(getuid() == 0) {
				fprintf(stderr, "Check permissions.  Should be 600.\n");
			} else {
				fprintf(stderr, "You must be root to open this file.\n");
			}
			break;
		case EUNATCH:
			fprintf(stderr, "KLIPS not loaded.\n");
			break;
		case ENODEV:
			fprintf(stderr, "KLIPS not loaded or enabled.\n");
			break;
		case EBUSY:
			fprintf(stderr, "KLIPS is busy.  Most likely a serious internal error occured in a previous command.  Please report as much detail as possible to development team.\n");
			break;
		case EINVAL:
			fprintf(stderr, "Invalid argument, KLIPS not loaded or check kernel log messages for specifics.\n");
			break;
		case ENOBUFS:
		case ENOMEM:
		case ENFILE:
			fprintf(stderr, "No kernel memory to allocate socket.\n");
			break;
		case EMFILE:
			fprintf(stderr, "Process file table overflow.\n");
			break;
		case ESOCKTNOSUPPORT:
			fprintf(stderr, "Socket type not supported.\n");
			break;
		case EPROTONOSUPPORT:
			fprintf(stderr, "Protocol version not supported.\n");
			break;
		case EAFNOSUPPORT:
			fprintf(stderr, "KLIPS not loaded or enabled.\n");
			break;
		default:
			fprintf(stderr, "Unknown file open error %d.  Please report as much detail as possible to development team.\n", errno);
		}
		exit(1);
	}

	if(debug) {
		fprintf(stdout, "%s: DEBUG: PFKEYv2 socket successfully openned=%d.\n", program_name, pfkey_sock);
	}

	/* Build an SADB_X_ADDFLOW or SADB_X_DELFLOW message to send down. */
	/* It needs <base, SA, address(SD), flow(SD), mask(SD)> minimum. */
	pfkey_extensions_init(extensions);
	if((error = pfkey_msg_hdr_build(&extensions[0],
					(action_type == EMT_SETEROUTE
					 || action_type == EMT_REPLACEROUTE
					 || action_type == EMT_INEROUTE)
					? SADB_X_ADDFLOW : SADB_X_DELFLOW,
					proto2satype(said.proto),
					0,
					++pfkey_seq,
					getpid()))) {
		fprintf(stderr, "%s: Trouble building message header, error=%d.\n",
			program_name, error);
		pfkey_extensions_free(extensions);
		exit(1);
	}

	if(debug) {
		fprintf(stdout, "%s: DEBUG: pfkey_msg_hdr_build successfull.\n", program_name);
	}

	switch(action_type) {
	case EMT_SETEROUTE:
	case EMT_REPLACEROUTE:
	case EMT_INEROUTE:
	case EMT_CLREROUTE:
		if((error = pfkey_sa_build(&extensions[SADB_EXT_SA],
					   SADB_EXT_SA,
					   said.spi, /* in network order */
					   0,
					   0,
					   0,
					   0,
					   (action_type == EMT_CLREROUTE) ? SADB_X_SAFLAGS_CLEARFLOW : 0))) {
			fprintf(stderr, "%s: Trouble building sa extension, error=%d.\n",
				program_name, error);
			pfkey_extensions_free(extensions);
			exit(1);
		}
		if(debug) {
			fprintf(stdout, "%s: DEBUG: pfkey_sa_build successful.\n", program_name);
		}

	default:
	}

	switch(action_type) {
	case EMT_SETEROUTE:
	case EMT_REPLACEROUTE:
	case EMT_INEROUTE:
		anyaddr(said_af, &pfkey_address_s_ska);
		if((error = pfkey_address_build(&extensions[SADB_EXT_ADDRESS_SRC],
						SADB_EXT_ADDRESS_SRC,
						0,
						0,
						sockaddrof(&pfkey_address_s_ska)))) {
			addrtot(&pfkey_address_s_ska, 0, ipaddr_txt, sizeof(ipaddr_txt));
			fprintf(stderr, "%s: Trouble building address_s extension (%s), error=%d.\n",
				program_name, ipaddr_txt, error);
			pfkey_extensions_free(extensions);
			exit(1);
		}
		if(debug) {
			fprintf(stdout, "%s: DEBUG: pfkey_address_build successful for src.\n", program_name);
		}

		if((error = pfkey_address_build(&extensions[SADB_EXT_ADDRESS_DST],
						SADB_EXT_ADDRESS_DST,
						0,
						0,
						sockaddrof(&said.dst)))) {
			addrtot(&said.dst, 0, ipaddr_txt, sizeof(ipaddr_txt));
			fprintf(stderr, "%s: Trouble building address_d extension (%s), error=%d.\n",
				program_name, ipaddr_txt, error);
			pfkey_extensions_free(extensions);
			exit(1);
		}
		if(debug) {
			fprintf(stdout, "%s: DEBUG: pfkey_address_build successful for dst.\n", program_name);
		}
	default:
	}
	
	switch(action_type) {
	case EMT_SETEROUTE:
	case EMT_REPLACEROUTE:
	case EMT_INEROUTE:
	case EMT_DELEROUTE:
		networkof(&s_subnet, &pfkey_address_sflow_ska); /* src flow */
		if((error = pfkey_address_build(&extensions[SADB_X_EXT_ADDRESS_SRC_FLOW],
						SADB_X_EXT_ADDRESS_SRC_FLOW,
						0,
						0,
						sockaddrof(&pfkey_address_sflow_ska)))) {
			addrtot(&pfkey_address_sflow_ska, 0, ipaddr_txt, sizeof(ipaddr_txt));
			fprintf(stderr, "%s: Trouble building address_sflow extension (%s), error=%d.\n",
				program_name, ipaddr_txt, error);
			pfkey_extensions_free(extensions);
			exit(1);
		}
		if(debug) {
			fprintf(stdout, "%s: DEBUG: pfkey_address_build successful for src flow.\n", program_name);
		}
	
		networkof(&d_subnet, &pfkey_address_dflow_ska); /* dst flow */
		if((error = pfkey_address_build(&extensions[SADB_X_EXT_ADDRESS_DST_FLOW],
						SADB_X_EXT_ADDRESS_DST_FLOW,
						0,
						0,
						sockaddrof(&pfkey_address_dflow_ska)))) {
			addrtot(&pfkey_address_dflow_ska, 0, ipaddr_txt, sizeof(ipaddr_txt));
			fprintf(stderr, "%s: Trouble building address_dflow extension (%s), error=%d.\n",
				program_name, ipaddr_txt, error);
			pfkey_extensions_free(extensions);
			exit(1);
		}
		if(debug) {
			fprintf(stdout, "%s: DEBUG: pfkey_address_build successful for dst flow.\n", program_name);
		}
		
		maskof(&s_subnet, &pfkey_address_smask_ska); /* src mask */
		if((error = pfkey_address_build(&extensions[SADB_X_EXT_ADDRESS_SRC_MASK],
						SADB_X_EXT_ADDRESS_SRC_MASK,
						0,
						0,
						sockaddrof(&pfkey_address_smask_ska)))) {
			addrtot(&pfkey_address_smask_ska, 0, ipaddr_txt, sizeof(ipaddr_txt));
			fprintf(stderr, "%s: Trouble building address_smask extension (%s), error=%d.\n",
				program_name, ipaddr_txt, error);
			pfkey_extensions_free(extensions);
			exit(1);
		}
		if(debug) {
			fprintf(stdout, "%s: DEBUG: pfkey_address_build successful for src mask.\n", program_name);
		}
		
		maskof(&d_subnet, &pfkey_address_dmask_ska); /* dst mask */
		if((error = pfkey_address_build(&extensions[SADB_X_EXT_ADDRESS_DST_MASK],
						SADB_X_EXT_ADDRESS_DST_MASK,
						0,
						0,
						sockaddrof(&pfkey_address_dmask_ska)))) {
			addrtot(&pfkey_address_dmask_ska, 0, ipaddr_txt, sizeof(ipaddr_txt));
			fprintf(stderr, "%s: Trouble building address_dmask extension (%s), error=%d.\n",
				program_name, ipaddr_txt, error);
			pfkey_extensions_free(extensions);
			exit(1);
		}
		if(debug) {
			fprintf(stdout, "%s: DEBUG: pfkey_address_build successful for dst mask.\n", program_name);
		}
	}
	
	if((error = pfkey_msg_build(&pfkey_msg, extensions, EXT_BITS_IN))) {
		fprintf(stderr, "%s: Trouble building pfkey message, error=%d.\n",
			program_name, error);
		pfkey_extensions_free(extensions);
		pfkey_msg_free(&pfkey_msg);
		exit(1);
	}
	if(debug) {
		fprintf(stdout, "%s: DEBUG: pfkey_msg_build successful.\n", program_name);
	}

	if((error = write(pfkey_sock,
			  pfkey_msg,
			  pfkey_msg->sadb_msg_len * IPSEC_PFKEYv2_ALIGN)) !=
	   pfkey_msg->sadb_msg_len * IPSEC_PFKEYv2_ALIGN) {
		fprintf(stderr, "%s: pfkey write failed, returning %d with errno=%d.\n",
			program_name, error, errno);
		pfkey_extensions_free(extensions);
		pfkey_msg_free(&pfkey_msg);
		switch(errno) {
		case EINVAL:
			fprintf(stderr, "Invalid argument, check kernel log messages for specifics.\n");
			break;
		case ENXIO:
			if((action_type == EMT_SETEROUTE) ||
			   (action_type == EMT_REPLACEROUTE)) {
				fprintf(stderr, "Invalid mask.\n");
				break;
			}
			if(action_type == EMT_DELEROUTE) {
				fprintf(stderr, "Mask not found.\n");
				break;
			}
		case EFAULT:
			if((action_type == EMT_SETEROUTE) ||
			   (action_type == EMT_REPLACEROUTE)) {
				fprintf(stderr, "Invalid address.\n");
				break;
			}
			if(action_type == EMT_DELEROUTE) {
				fprintf(stderr, "Address not found.\n");
				break;
			}
		case EACCES:
			fprintf(stderr, "access denied.  ");
			if(getuid() == 0) {
				fprintf(stderr, "Check permissions.  Should be 600.\n");
			} else {
				fprintf(stderr, "You must be root to open this file.\n");
			}
			break;
		case EUNATCH:
			fprintf(stderr, "KLIPS not loaded.\n");
			break;
		case EBUSY:
			fprintf(stderr, "KLIPS is busy.  Most likely a serious internal error occured in a previous command.  Please report as much detail as possible to development team.\n");
			break;
		case ENODEV:
			fprintf(stderr, "KLIPS not loaded or enabled.\n");
			fprintf(stderr, "No device?!?\n");
			break;
		case ENOBUFS:
			fprintf(stderr, "No kernel memory to allocate SA.\n");
			break;
		case ESOCKTNOSUPPORT:
			fprintf(stderr, "Algorithm support not available in the kernel.  Please compile in support.\n");
			break;
		case EEXIST:
			fprintf(stderr, "eroute already in use.  Delete old one first.\n");
			break;
		case ENOENT:
			if((action_type == EMT_INEROUTE)) {
				fprintf(stderr, "non-existant IPIP SA.\n");
				break;
			}
			fprintf(stderr, "eroute doesn't exist.  Can't delete.\n");
			break;
		default:
			fprintf(stderr, "Unknown socket write error %d.  Please report as much detail as possible to development team.\n", errno);
		}
/*		fprintf(stderr, "%s: socket write returned errno %d\n",
		program_name, errno);*/
		exit(1);
	}
	if(debug) {
		fprintf(stdout, "%s: DEBUG: pfkey write successful.\n", program_name);
	}

	if(pfkey_msg) {
		pfkey_extensions_free(extensions);
		pfkey_msg_free(&pfkey_msg);
	}

	(void) close(pfkey_sock);  /* close the socket */

	if(debug) {
		fprintf(stdout, "%s: DEBUG: write ok\n", program_name);
	}

	exit(0);
}
/*
 * $Log: eroute.c,v $
 * Revision 1.49  2002/03/08 21:44:04  rgb
 * Update for all GNU-compliant --version strings.
 *
 * Revision 1.48  2002/02/15 19:54:11  rgb
 * Purged dead code.
 *
 * Revision 1.47  2001/11/09 01:42:36  rgb
 * Re-formatted usage text for clarity.
 *
 * Revision 1.46  2001/10/02 17:03:45  rgb
 * Check error return for all "tto*" calls and report errors.  This, in
 * conjuction with the fix to "tto*" will detect AF not set.
 *
 * Revision 1.45  2001/09/07 22:12:27  rgb
 * Added EAFNOSUPPORT error return explanation for KLIPS not loaded.
 *
 * Revision 1.44  2001/07/06 19:49:33  rgb
 * Renamed EMT_RPLACEROUTE to EMT_REPLACEROUTE for clarity and logical text
 * searching.
 * Added EMT_INEROUTE for supporting incoming policy checks.
 * Added inbound policy checking code for IPIP SAs.
 *
 * Revision 1.43  2001/06/15 05:02:05  rgb
 * Fixed error return messages and codes.
 *
 * Revision 1.42  2001/06/14 19:35:14  rgb
 * Update copyright date.
 *
 * Revision 1.41  2001/05/21 02:02:54  rgb
 * Eliminate 1-letter options.
 *
 * Revision 1.40  2001/05/16 04:39:57  rgb
 * Fix --label option to add to command name rather than replace it.
 * Fix 'print table' option to ignore --label and --debug options.
 *
 * Revision 1.39  2001/02/26 19:59:03  rgb
 * Added a number of missing ntohl() conversions for debug output.
 * Implement magic SAs %drop, %reject, %trap, %hold, %pass as part
 * of the new SPD and to support opportunistic.
 * Enforced spi > 0x100 requirement, now that pass uses a magic SA.
 *
 * Revision 1.38  2000/09/17 18:56:48  rgb
 * Added IPCOMP support.
 *
 * Revision 1.37  2000/09/12 22:36:08  rgb
 * Gerhard's IPv6 support.
 * Restructured to remove unused extensions from CLEARFLOW messages.
 * Added debugging.
 *
 * Revision 1.36  2000/09/08 19:17:31  rgb
 * Removed all references to CONFIG_IPSEC_PFKEYv2.
 *
 * Revision 1.35  2000/08/27 01:46:52  rgb
 * Update copyright dates and remove no longer used resolve_ip().
 *
 * Revision 1.34  2000/07/26 03:41:45  rgb
 * Changed all printf's to fprintf's.  Fixed tncfg's usage to stderr.
 *
 * Revision 1.33  2000/07/13 21:54:49  rgb
 * Remove old cruft from a time when libfreeswan didn't exist and I checked
 * name lookup errors with the default address.
 *
 * Revision 1.32  2000/06/21 16:51:27  rgb
 * Added no additional argument option to usage text.
 *
 * Revision 1.31  2000/03/16 06:40:49  rgb
 * Hardcode PF_KEYv2 support.
 *
 * Revision 1.30  2000/01/22 23:22:46  rgb
 * Use new function proto2satype().
 *
 * Revision 1.29  2000/01/21 09:42:32  rgb
 * Replace resolve_ip() with atoaddr() from freeswanlib.
 *
 * Revision 1.28  2000/01/21 06:22:28  rgb
 * Changed to AF_ENCAP macro.
 * Added --debug switch to command line.
 * Added pfkeyv2 support to completely avoid netlink.
 *
 * Revision 1.27  1999/12/07 18:27:10  rgb
 * Added headers to silence fussy compilers.
 * Converted local functions to static to limit scope.
 *
 * Revision 1.26  1999/11/25 09:07:44  rgb
 * Fixed printf % escape bug.
 * Clarified assignment in conditional with parens.
 *
 * Revision 1.25  1999/11/23 23:06:26  rgb
 * Sort out pfkey and freeswan headers, putting them in a library path.
 *
 * Revision 1.24  1999/06/10 15:55:14  rgb
 * Add error return code.
 *
 * Revision 1.23  1999/04/15 15:37:27  rgb
 * Forward check changes from POST1_00 branch.
 *
 * Revision 1.19.2.2  1999/04/13 20:58:10  rgb
 * Add argc==1 --> /proc/net/ipsec_*.
 *
 * Revision 1.19.2.1  1999/03/30 17:01:36  rgb
 * Make main() return type explicit.
 *
 * Revision 1.22  1999/04/11 00:12:08  henry
 * GPL boilerplate
 *
 * Revision 1.21  1999/04/06 04:54:37  rgb
 * Fix/Add RCSID Id: and Log: bits to make PHMDs happy.  This includes
 * patch shell fixes.
 *
 * Revision 1.20  1999/03/17 15:40:54  rgb
 * Make explicit main() return type of int.
 *
 * Revision 1.19  1999/01/26 05:51:01  rgb
 * Updated to use %passthrough instead of bypass.
 *
 * Revision 1.18  1999/01/22 06:34:52  rgb
 * Update to include SAID command line parameter.
 * Add IPSEC 'bypass' switch.
 * Add error-checking.
 * Cruft clean-out.
 *
 * Revision 1.17  1998/11/29 00:52:26  rgb
 * Add explanation to warning about default source or destination.
 *
 * Revision 1.16  1998/11/12 21:08:03  rgb
 * Add --label option to identify caller from scripts.
 *
 * Revision 1.15  1998/10/27 00:33:27  rgb
 * Make output error text more fatal-sounding.
 *
 * Revision 1.14  1998/10/26 01:28:38  henry
 * use SA_* protocol names, not IPPROTO_*, to avoid compile problems
 *
 * Revision 1.13  1998/10/25 02:44:56  rgb
 * Institute more precise error return codes from eroute commands.
 *
 * Revision 1.12  1998/10/19 18:58:55  rgb
 * Added inclusion of freeswan.h.
 * a_id structure implemented and used: now includes protocol.
 *
 * Revision 1.11  1998/10/09 18:47:29  rgb
 * Add 'optionfrom' to get more options from a named file.
 *
 * Revision 1.10  1998/10/09 04:34:58  rgb
 * Changed help output from stderr to stdout.
 * Changed error messages from stdout to stderr.
 * Added '--replace' option.
 * Deleted old commented out cruft.
 *
 * Revision 1.9  1998/08/18 17:18:13  rgb
 * Delete old commented out cruft.
 * Reduce destination and source default subnet to warning, not fatal.
 *
 * Revision 1.8  1998/08/05 22:24:45  rgb
 * Change includes to accomodate RH5.x
 *
 * Revision 1.7  1998/07/29 20:49:08  rgb
 * Change to use 0x-prefixed hexadecimal for spi's.
 *
 * Revision 1.6  1998/07/28 00:14:24  rgb
 * Convert from positional parameters to long options.
 * Add --clean option.
 * Add hostname lookup support.
 *
 * Revision 1.5  1998/07/14 18:13:28  rgb
 * Restructured for better argument checking.
 * Added command to clear the eroute table.
 *
 * Revision 1.4  1998/07/09 18:14:10  rgb
 * Added error checking to IP's and keys.
 * Made most error messages more specific rather than spamming usage text.
 * Added more descriptive kernel error return codes and messages.
 * Converted all spi translations to unsigned.
 * Removed all invocations of perror.
 *
 * Revision 1.3  1998/05/27 18:48:19  rgb
 * Adding --help and --version directives.
 *
 * Revision 1.2  1998/04/13 03:15:29  rgb
 * Commands are now distinguishable from arguments when invoking usage.
 *
 * Revision 1.1.1.1  1998/04/08 05:35:10  henry
 * RGB's ipsec-0.8pre2.tar.gz ipsec-0.8
 *
 * Revision 0.3  1996/11/20 14:51:32  ji
 * Fixed problems with #include paths.
 * Changed (incorrect) references to ipsp into ipsec.
 *
 * Revision 0.2  1996/11/08 15:45:24  ji
 * First limited release.
 *
 *
 */
