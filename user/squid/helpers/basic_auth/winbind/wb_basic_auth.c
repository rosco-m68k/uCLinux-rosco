/*
 * (C) 2000 Francesco Chemolli <kinkie@kame.usr.dsi.unimi.it>
 *
 * Distributed freely under the terms of the GNU General Public License,
 * version 2. See the file COPYING for licensing details
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */


#include "wbntlm.h"
#include "util.h"
/* stdio.h is included in wbntlm.h */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>           /* for gettimeofday */
#include <errno.h>              /* BUG: is this portable? */

#include "nsswitch/winbind_nss_config.h"
#include "nsswitch/winbindd_nss.h"

char debug_enabled=0;
char *myname;
pid_t mypid;
int err = 0;

NSS_STATUS winbindd_request(int req_type,
			    struct winbindd_request *request,
			    struct winbindd_response *response);
		 

void do_authenticate(char *user, char *pass)
{
    struct winbindd_request request;
    struct winbindd_response response;
    NSS_STATUS winbindd_result;
	
    memset(&request,0,sizeof(struct winbindd_request));
    memset(&response,0,sizeof(struct winbindd_response));

    strncpy(request.data.auth.user,user,sizeof(fstring)-1);
    strncpy(request.data.auth.pass,pass,sizeof(fstring)-1);

    winbindd_result = winbindd_request(WINBINDD_PAM_AUTH,
	&request, &response);
    debug("winbindd result: %d\n",winbindd_result);

    if (winbindd_result==NSS_STATUS_SUCCESS) {
	SEND("OK");
    } else {
	SEND("ERR");
    }

    return;		/* useless */
}

static void
usage(char *program)
{
    fprintf(stderr,"Usage: %s [-d] [-h]\n"
	    	" -d      enable debugging\n"
		" -h      this message\n",
		program);
}

void
process_options(int argc, char *argv[])
{
    int opt;

    opterr = 0;
    while (-1 != (opt = getopt(argc, argv, "dh"))) {
	switch (opt) {
	case 'd':
	    debug_enabled = 1;
	    break;
	case 'h':
	    usage(argv[0]);
	    exit(0);
	case '?':
	    opt = optopt;
	    /* fall thru to default */
	default:
	    warn("Unknown option: -%c\n\n", opt);
	    usage(argv[0]);
	    exit(1);
	    break;		/* not reached */
	}
    }
    return;
}

void manage_request(void)
{
    char buf[BUFFER_SIZE+1];
    int length;
    char *c, *user, *pass;
  
    if (fgets(buf, BUFFER_SIZE, stdin) == NULL) {
	warn("fgets() failed! dying..... errno=%d (%s)\n", errno,
	    strerror(errno));
	exit(1);    /* BIIG buffer */
    }
	
    c=memchr(buf,'\n',BUFFER_SIZE);
    if (c) {
	*c='\0';
	length=c-buf;
    } else {
	err = 1;
	return;
    }
    if (err) {
	warn("Oversized message\n");
	SEND("ERR");
	err = 0;
	return;
    }
  
    debug("Got '%s' from squid (length: %d).\n",buf,length);

    if (buf[0] == '\0') {
	warn("Invalid Request\n");
	SEND("ERR");
	return;
    }

    user=buf;

    pass=memchr(buf,' ',length);
    if (!pass) {
	warn("Password not found. Denying access\n");
	SEND("ERR");
	return;
    }
    *pass='\0';
    pass++;

    rfc1738_unescape(user);
    rfc1738_unescape(pass);

    do_authenticate(user,pass);
}


int main (int argc, char ** argv)
{
    if (argc > 0) {	/* should always be true */
	myname=strrchr(argv[0],'/');
	if (myname==NULL)
	    myname=argv[0];
    } else {
        myname="(unknown)";
    }
    mypid=getpid();
    process_options(argc, argv);

    debug("basic winbindd auth helper build " __DATE__ ", " __TIME__
    " starting up...\n");
    /* initialize FDescs */
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    while(1) {
	manage_request();
    }
    return 0;
}
