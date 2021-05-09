#include <net-snmp/net-snmp-config.h>

#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <net-snmp/agent/read_only.h>

#if HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

/** @defgroup read_only read_only: make your handler read_only automatically 
 *  The only purpose of this handler is to return an
 *  appropriate error for any requests passed to it in a SET mode.
 *  Inserting it into your handler chain will ensure you're never
 *  asked to perform a SET request so you can ignore those error
 *  conditions.
 *  @ingroup handler
 *  @{
 */

/** returns a read_only handler that can be injected into a given
 *  handler chain.
 */
netsnmp_mib_handler *
netsnmp_get_read_only_handler(void)
{
    return netsnmp_create_handler("read_only", netsnmp_read_only_helper);
}

/** @internal Implements the read_only handler */
int
netsnmp_read_only_helper(netsnmp_mib_handler *handler,
                         netsnmp_handler_registration *reginfo,
                         netsnmp_agent_request_info *reqinfo,
                         netsnmp_request_info *requests)
{

    DEBUGMSGTL(("helper:read_only", "Got request\n"));

    switch (reqinfo->mode) {

    case MODE_SET_RESERVE1:
    case MODE_SET_RESERVE2:
    case MODE_SET_ACTION:
    case MODE_SET_COMMIT:
    case MODE_SET_FREE:
    case MODE_SET_UNDO:
        netsnmp_set_all_requests_error(reqinfo, requests,
                                       SNMP_ERR_NOTWRITABLE);
        return SNMP_ERR_NOERROR;

    default:
        return netsnmp_call_next_handler(handler, reginfo, reqinfo,
                                         requests);
    }
    return SNMP_ERR_GENERR;     /* should never get here */
}

/** initializes the read_only helper which then registers a read_only
 *  handler as a run-time injectable handler for configuration file
 *  use.
 */
void
netsnmp_init_read_only_helper(void)
{
    netsnmp_register_handler_by_name("read_only",
                                     netsnmp_get_read_only_handler());
}
