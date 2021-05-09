#include <net-snmp/net-snmp-config.h>

#include <sys/types.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif

#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/instance.h>
#include <net-snmp/agent/table.h>
#include <net-snmp/agent/table_data.h>
#include <net-snmp/agent/table_dataset.h>
#include "notification_log.h"

extern u_long   num_received;
u_long          num_deleted = 0;

u_long          max_logged = 1000;      /* goes against the mib default of infinite */
u_long          max_age = 1440; /* 24 hours, which is the mib default */

netsnmp_table_data_set *nlmLogTable;
netsnmp_table_data_set *nlmLogVarTable;

void
check_log_size(unsigned int clientreg, void *clientarg)
{
    netsnmp_table_row *row, *deleterow, *tmprow, *deletevarrow;
    netsnmp_table_data_set_storage *data;
    u_long          count = 0;
    struct timeval  now;
    long            tmpl;

    gettimeofday(&now, NULL);
    tmpl = netsnmp_timeval_uptime(&now);

    for (row = nlmLogTable->table->first_row; row; row = row->next) {
        /*
         * check max allowed count 
         */
        count++;
        if (max_logged && count == max_logged)
            break;

        /*
         * check max age 
         */

        data = (netsnmp_table_data_set_storage *) row->data;
        data = netsnmp_table_data_set_find_column(data, COLUMN_NLMLOGTIME);

        if (max_age && tmpl > (*(data->data.integer) + max_age * 100 * 60))
            break;
    }

    if (!row)
        return;

    /*
     * we've reached the limit, so keep looping but start deleting
     * from the beginning 
     */
    for (deleterow = nlmLogTable->table->first_row, row = row->next; row;
         row = row->next) {
        DEBUGMSGTL(("notification_log", "deleting a log entry\n"));

        /*
         * delete contained varbinds 
         */
        for (deletevarrow = nlmLogVarTable->table->first_row;
             deletevarrow; deletevarrow = tmprow) {

            tmprow = deletevarrow->next;

            if (deleterow->index_oid_len ==
                deletevarrow->index_oid_len - 1 &&
                snmp_oid_compare(deleterow->index_oid,
                                 deleterow->index_oid_len,
                                 deletevarrow->index_oid,
                                 deleterow->index_oid_len) == 0) {
                netsnmp_table_dataset_remove_and_delete_row(nlmLogVarTable,
                                                            deletevarrow);
            }
        }

        /*
         * delete the master row 
         */
        tmprow = deleterow->next;
        netsnmp_table_dataset_remove_and_delete_row(nlmLogTable,
                                                    deleterow);
        deleterow = tmprow;
        num_deleted++;
        /*
         * XXX: delete vars from it's table 
         */
    }
}


/** Initialize the nlmLogVariableTable table by defining it's contents and how it's structured */
void
initialize_table_nlmLogVariableTable(void)
{
    static oid      nlmLogVariableTable_oid[] =
        { 1, 3, 6, 1, 2, 1, 92, 1, 3, 2 };
    size_t          nlmLogVariableTable_oid_len =
        OID_LENGTH(nlmLogVariableTable_oid);
    netsnmp_table_data_set *table_set;

    /*
     * create the table structure itself 
     */
    table_set = netsnmp_create_table_data_set("nlmLogVariableTable");
    nlmLogVarTable = table_set;
    nlmLogVarTable->table->store_indexes = 1;

    /***************************************************
     * Adding indexes
     */
    /*
     * declaring the nlmLogName index
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding index nlmLogName of type ASN_OCTET_STR to table nlmLogVariableTable\n"));
    netsnmp_table_dataset_add_index(table_set, ASN_OCTET_STR);
    /*
     * declaring the nlmLogIndex index
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding index nlmLogIndex of type ASN_UNSIGNED to table nlmLogVariableTable\n"));
    netsnmp_table_dataset_add_index(table_set, ASN_UNSIGNED);
    /*
     * declaring the nlmLogVariableIndex index
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding index nlmLogVariableIndex of type ASN_UNSIGNED to table nlmLogVariableTable\n"));
    netsnmp_table_dataset_add_index(table_set, ASN_UNSIGNED);

    /*
     * adding column nlmLogVariableIndex of type ASN_UNSIGNED and access
     * of NoAccess 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableIndex (#1) of type ASN_UNSIGNED to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLEINDEX,
                                      ASN_UNSIGNED, 0, NULL, 0);
    /*
     * adding column nlmLogVariableID of type ASN_OBJECT_ID and access of
     * ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableID (#2) of type ASN_OBJECT_ID to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set, COLUMN_NLMLOGVARIABLEID,
                                      ASN_OBJECT_ID, 0, NULL, 0);
    /*
     * adding column nlmLogVariableValueType of type ASN_INTEGER and
     * access of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableValueType (#3) of type ASN_INTEGER to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLEVALUETYPE,
                                      ASN_INTEGER, 0, NULL, 0);
    /*
     * adding column nlmLogVariableCounter32Val of type ASN_COUNTER and
     * access of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableCounter32Val (#4) of type ASN_COUNTER to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLECOUNTER32VAL,
                                      ASN_COUNTER, 0, NULL, 0);
    /*
     * adding column nlmLogVariableUnsigned32Val of type ASN_UNSIGNED and
     * access of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableUnsigned32Val (#5) of type ASN_UNSIGNED to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLEUNSIGNED32VAL,
                                      ASN_UNSIGNED, 0, NULL, 0);
    /*
     * adding column nlmLogVariableTimeTicksVal of type ASN_TIMETICKS and
     * access of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableTimeTicksVal (#6) of type ASN_TIMETICKS to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLETIMETICKSVAL,
                                      ASN_TIMETICKS, 0, NULL, 0);
    /*
     * adding column nlmLogVariableInteger32Val of type ASN_INTEGER and
     * access of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableInteger32Val (#7) of type ASN_INTEGER to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLEINTEGER32VAL,
                                      ASN_INTEGER, 0, NULL, 0);
    /*
     * adding column nlmLogVariableOctetStringVal of type ASN_OCTET_STR
     * and access of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableOctetStringVal (#8) of type ASN_OCTET_STR to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLEOCTETSTRINGVAL,
                                      ASN_OCTET_STR, 0, NULL, 0);
    /*
     * adding column nlmLogVariableIpAddressVal of type ASN_IPADDRESS and
     * access of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableIpAddressVal (#9) of type ASN_IPADDRESS to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLEIPADDRESSVAL,
                                      ASN_IPADDRESS, 0, NULL, 0);
    /*
     * adding column nlmLogVariableOidVal of type ASN_OBJECT_ID and access 
     * of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableOidVal (#10) of type ASN_OBJECT_ID to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLEOIDVAL,
                                      ASN_OBJECT_ID, 0, NULL, 0);
    /*
     * adding column nlmLogVariableCounter64Val of type ASN_COUNTER64 and
     * access of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableCounter64Val (#11) of type ASN_COUNTER64 to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLECOUNTER64VAL,
                                      ASN_COUNTER64, 0, NULL, 0);
    /*
     * adding column nlmLogVariableOpaqueVal of type ASN_OPAQUE and access 
     * of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogVariableTable",
                "adding column nlmLogVariableOpaqueVal (#12) of type ASN_OPAQUE to table nlmLogVariableTable\n"));
    netsnmp_table_set_add_default_row(table_set,
                                      COLUMN_NLMLOGVARIABLEOPAQUEVAL,
                                      ASN_OPAQUE, 0, NULL, 0);

    /*
     * registering the table with the master agent 
     */
    /*
     * note: if you don't need a subhandler to deal with any aspects of
     * the request, change nlmLogVariableTable_handler to "NULL" 
     */
    netsnmp_register_table_data_set(netsnmp_create_handler_registration
                                    ("nlmLogVariableTable",
                                     nlmLogVariableTable_handler,
                                     nlmLogVariableTable_oid,
                                     nlmLogVariableTable_oid_len,
                                     HANDLER_CAN_RWRITE), table_set, NULL);
}

/** Initialize the nlmLogTable table by defining it's contents and how it's structured */
void
initialize_table_nlmLogTable(void)
{
    static oid      nlmLogTable_oid[] = { 1, 3, 6, 1, 2, 1, 92, 1, 3, 1 };
    size_t          nlmLogTable_oid_len = OID_LENGTH(nlmLogTable_oid);

    /*
     * create the table structure itself 
     */
    nlmLogTable = netsnmp_create_table_data_set("nlmLogTable");

    /***************************************************
     * Adding indexes
     */
    /*
     * declaring the nlmLogIndex index
     */
    DEBUGMSGTL(("initialize_table_nlmLogTable",
                "adding index nlmLogName of type ASN_OCTET_STR to table nlmLogTable\n"));
    netsnmp_table_dataset_add_index(nlmLogTable, ASN_OCTET_STR);

    DEBUGMSGTL(("initialize_table_nlmLogTable",
                "adding index nlmLogIndex of type ASN_UNSIGNED to table nlmLogTable\n"));
    netsnmp_table_dataset_add_index(nlmLogTable, ASN_UNSIGNED);

    /*
     * adding column nlmLogTime of type ASN_TIMETICKS and access of
     * ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogTable",
                "adding column nlmLogTime (#2) of type ASN_TIMETICKS to table nlmLogTable\n"));
    netsnmp_table_set_add_default_row(nlmLogTable, COLUMN_NLMLOGTIME,
                                      ASN_TIMETICKS, 0, NULL, 0);
    /*
     * adding column nlmLogDateAndTime of type ASN_OCTET_STR and access of 
     * ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogTable",
                "adding column nlmLogDateAndTime (#3) of type ASN_OCTET_STR to table nlmLogTable\n"));
    netsnmp_table_set_add_default_row(nlmLogTable,
                                      COLUMN_NLMLOGDATEANDTIME,
                                      ASN_OCTET_STR, 0, NULL, 0);
    /*
     * adding column nlmLogEngineID of type ASN_OCTET_STR and access of
     * ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogTable",
                "adding column nlmLogEngineID (#4) of type ASN_OCTET_STR to table nlmLogTable\n"));
    netsnmp_table_set_add_default_row(nlmLogTable, COLUMN_NLMLOGENGINEID,
                                      ASN_OCTET_STR, 0, NULL, 0);
    /*
     * adding column nlmLogEngineTAddress of type ASN_OCTET_STR and access 
     * of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogTable",
                "adding column nlmLogEngineTAddress (#5) of type ASN_OCTET_STR to table nlmLogTable\n"));
    netsnmp_table_set_add_default_row(nlmLogTable,
                                      COLUMN_NLMLOGENGINETADDRESS,
                                      ASN_OCTET_STR, 0, NULL, 0);
    /*
     * adding column nlmLogEngineTDomain of type ASN_OBJECT_ID and access
     * of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogTable",
                "adding column nlmLogEngineTDomain (#6) of type ASN_OBJECT_ID to table nlmLogTable\n"));
    netsnmp_table_set_add_default_row(nlmLogTable,
                                      COLUMN_NLMLOGENGINETDOMAIN,
                                      ASN_OBJECT_ID, 0, NULL, 0);
    /*
     * adding column nlmLogContextEngineID of type ASN_OCTET_STR and
     * access of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogTable",
                "adding column nlmLogContextEngineID (#7) of type ASN_OCTET_STR to table nlmLogTable\n"));
    netsnmp_table_set_add_default_row(nlmLogTable,
                                      COLUMN_NLMLOGCONTEXTENGINEID,
                                      ASN_OCTET_STR, 0, NULL, 0);
    /*
     * adding column nlmLogContextName of type ASN_OCTET_STR and access of 
     * ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogTable",
                "adding column nlmLogContextName (#8) of type ASN_OCTET_STR to table nlmLogTable\n"));
    netsnmp_table_set_add_default_row(nlmLogTable,
                                      COLUMN_NLMLOGCONTEXTNAME,
                                      ASN_OCTET_STR, 0, NULL, 0);
    /*
     * adding column nlmLogNotificationID of type ASN_OBJECT_ID and access 
     * of ReadOnly 
     */
    DEBUGMSGTL(("initialize_table_nlmLogTable",
                "adding column nlmLogNotificationID (#9) of type ASN_OBJECT_ID to table nlmLogTable\n"));
    netsnmp_table_set_add_default_row(nlmLogTable,
                                      COLUMN_NLMLOGNOTIFICATIONID,
                                      ASN_OBJECT_ID, 0, NULL, 0);

    /*
     * registering the table with the master agent 
     */
    /*
     * note: if you don't need a subhandler to deal with any aspects of
     * the request, change nlmLogTable_handler to "NULL" 
     */
    netsnmp_register_table_data_set(netsnmp_create_handler_registration
                                    ("nlmLogTable", nlmLogTable_handler,
                                     nlmLogTable_oid, nlmLogTable_oid_len,
                                     HANDLER_CAN_RWRITE), nlmLogTable,
                                    NULL);

    /*
     * hmm...  5 minutes seems like a reasonable time to check for out
     * dated notification logs right? 
     */
    snmp_alarm_register(300, SA_REPEAT, check_log_size, NULL);
}

int
notification_log_config_handler(netsnmp_mib_handler *handler,
                                netsnmp_handler_registration *reginfo,
                                netsnmp_agent_request_info *reqinfo,
                                netsnmp_request_info *requests)
{
    /*
     *this handler exists only to act as a trigger when the
     * configuration variables get set to a value and thus
     * notifications must be possibly deleted from our archives.
     */
    if (reqinfo->mode == MODE_SET_COMMIT)
        check_log_size(0, NULL);
    return SNMP_ERR_NOERROR;
}

void
init_notification_log(void)
{
    static oid      my_nlmStatsGlobalNotificationsLogged_oid[] =
        { 1, 3, 6, 1, 2, 1, 92, 1, 2, 1, 0 };
    static oid      my_nlmStatsGlobalNotificationsBumped_oid[] =
        { 1, 3, 6, 1, 2, 1, 92, 1, 2, 2, 0 };
    static oid      my_nlmConfigGlobalEntryLimit_oid[] =
        { 1, 3, 6, 1, 2, 1, 92, 1, 1, 1, 0 };
    static oid      my_nlmConfigGlobalAgeOut_oid[] =
        { 1, 3, 6, 1, 2, 1, 92, 1, 1, 2, 0 };

    /*
     * static variables 
     */
    netsnmp_register_read_only_counter32_instance
        ("nlmStatsGlobalNotificationsLogged",
         my_nlmStatsGlobalNotificationsLogged_oid,
         OID_LENGTH(my_nlmStatsGlobalNotificationsLogged_oid),
         &num_received, NULL);

    netsnmp_register_read_only_counter32_instance
        ("nlmStatsGlobalNotificationsBumped",
         my_nlmStatsGlobalNotificationsBumped_oid,
         OID_LENGTH(my_nlmStatsGlobalNotificationsBumped_oid),
         &num_deleted, NULL);

    netsnmp_register_ulong_instance("nlmConfigGlobalEntryLimit",
                                    my_nlmConfigGlobalEntryLimit_oid,
                                    OID_LENGTH
                                    (my_nlmConfigGlobalEntryLimit_oid),
                                    &max_logged,
                                    notification_log_config_handler);

    netsnmp_register_ulong_instance("nlmConfigGlobalAgeOut",
                                    my_nlmConfigGlobalAgeOut_oid,
                                    OID_LENGTH
                                    (my_nlmConfigGlobalAgeOut_oid),
                                    &max_age,
                                    notification_log_config_handler);

    /*
     * tables 
     */
    initialize_table_nlmLogTable();
    initialize_table_nlmLogVariableTable();

    /*
     * disable flag 
     */
    netsnmp_ds_register_config(ASN_BOOLEAN, "snmptrapd", "dontRetainLogs",
			   NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_APP_DONT_LOG);
}

u_long          default_num = 0;

void
log_notification(struct hostent *host, netsnmp_pdu *pdu,
                 netsnmp_transport *transport)
{
    long            tmpl;
    struct timeval  now;
    netsnmp_table_row *row;

    static oid      snmptrapoid[] = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0 };
    size_t          snmptrapoid_len = OID_LENGTH(snmptrapoid);
    netsnmp_variable_list *vptr;
    u_char         *logdate;
    size_t          logdate_size;
    time_t          timetnow;

    u_long          vbcount = 0;
    u_long          tmpul;
    int             col;

    if (netsnmp_ds_get_boolean(NETSNMP_DS_APPLICATION_ID, 
			       NETSNMP_DS_APP_DONT_LOG)) {
        return;
    }

    DEBUGMSGTL(("log_notification", "logging something\n"));
    row = netsnmp_create_table_data_row();

    default_num++;

    /*
     * indexes to the table 
     */
    netsnmp_table_row_add_index(row, ASN_OCTET_STR, "default",
                                strlen("default"));
    netsnmp_table_row_add_index(row, ASN_UNSIGNED, &default_num,
                                sizeof(default_num));

    /*
     * add the data 
     */
    gettimeofday(&now, NULL);
    tmpl = netsnmp_timeval_uptime(&now);
    netsnmp_set_row_column(row, COLUMN_NLMLOGTIME, ASN_TIMETICKS,
                           (u_char *) & tmpl, sizeof(tmpl));
    time(&timetnow);
    logdate = date_n_time(&timetnow, &logdate_size);
    netsnmp_set_row_column(row, COLUMN_NLMLOGDATEANDTIME, ASN_OCTET_STR,
                           logdate, logdate_size);
    netsnmp_set_row_column(row, COLUMN_NLMLOGENGINEID, ASN_OCTET_STR,
                           pdu->securityEngineID,
                           pdu->securityEngineIDLen);
    if (transport && transport->domain == netsnmpUDPDomain) {
        /*
         * lame way to check for the udp domain 
         */
        /*
         * no, it is the correct way to do it -- jbpn 
         */
        struct sockaddr_in *addr =
            (struct sockaddr_in *) pdu->transport_data;
        if (addr) {
            char            buf[sizeof(in_addr_t) +
                                sizeof(addr->sin_port)];
            in_addr_t       locaddr = htonl(addr->sin_addr.s_addr);
            u_short         portnum = htons(addr->sin_port);
            memcpy(buf, &locaddr, sizeof(in_addr_t));
            memcpy(buf + sizeof(in_addr_t), &portnum,
                   sizeof(addr->sin_port));
            netsnmp_set_row_column(row, COLUMN_NLMLOGENGINETADDRESS,
                                   ASN_OCTET_STR, buf,
                                   sizeof(in_addr_t) +
                                   sizeof(addr->sin_port));
        }
    }
    netsnmp_set_row_column(row, COLUMN_NLMLOGENGINETDOMAIN, ASN_OBJECT_ID,
                           (const u_char *) transport->domain,
                           sizeof(oid) * transport->domain_length);
    netsnmp_set_row_column(row, COLUMN_NLMLOGCONTEXTENGINEID,
                           ASN_OCTET_STR, pdu->contextEngineID,
                           pdu->contextEngineIDLen);
    netsnmp_set_row_column(row, COLUMN_NLMLOGCONTEXTNAME, ASN_OCTET_STR,
                           pdu->contextName, pdu->contextNameLen);
    for (vptr = pdu->variables; vptr; vptr = vptr->next_variable) {
        if (snmp_oid_compare(snmptrapoid, snmptrapoid_len,
                             vptr->name, vptr->name_length) == 0) {
            netsnmp_set_row_column(row, COLUMN_NLMLOGNOTIFICATIONID,
                                   ASN_OBJECT_ID, vptr->val.string,
                                   vptr->val_len);

        } else {
            netsnmp_table_row *myrow;
            myrow = netsnmp_create_table_data_row();

            /*
             * indexes to the table 
             */
            netsnmp_table_row_add_index(myrow, ASN_OCTET_STR, "default",
                                        strlen("default"));
            netsnmp_table_row_add_index(myrow, ASN_UNSIGNED, &default_num,
                                        sizeof(default_num));
            vbcount++;
            netsnmp_table_row_add_index(myrow, ASN_UNSIGNED, &vbcount,
                                        sizeof(vbcount));

            /*
             * OID 
             */
            netsnmp_set_row_column(myrow, COLUMN_NLMLOGVARIABLEID,
                                   ASN_OBJECT_ID, (u_char *) vptr->name,
                                   vptr->name_length * sizeof(oid));

            /*
             * value 
             */
            switch (vptr->type) {
            case ASN_OBJECT_ID:
                tmpul = 7;
                col = COLUMN_NLMLOGVARIABLEOIDVAL;
                break;

            case ASN_INTEGER:
                tmpul = 4;
                col = COLUMN_NLMLOGVARIABLEINTEGER32VAL;
                break;

            case ASN_UNSIGNED:
                tmpul = 2;
                col = COLUMN_NLMLOGVARIABLEUNSIGNED32VAL;
                break;

            case ASN_COUNTER:
                tmpul = 1;
                col = COLUMN_NLMLOGVARIABLECOUNTER32VAL;
                break;

            case ASN_TIMETICKS:
                tmpul = 3;
                col = COLUMN_NLMLOGVARIABLETIMETICKSVAL;
                break;

            case ASN_OCTET_STR:
                tmpul = 6;
                col = COLUMN_NLMLOGVARIABLEOCTETSTRINGVAL;
                break;

            default:
                /*
                 * unsupported 
                 */
                DEBUGMSGTL(("log_notification",
                            "skipping type %d\n", vptr->type));
                continue;
            }
            netsnmp_set_row_column(myrow, COLUMN_NLMLOGVARIABLEVALUETYPE,
                                   ASN_INTEGER, (u_char *) & tmpul,
                                   sizeof(tmpul));
            netsnmp_set_row_column(myrow, col, vptr->type,
                                   vptr->val.string, vptr->val_len);
            DEBUGMSGTL(("log_notification",
                        "adding a row to the variables table\n"));
            netsnmp_table_dataset_add_row(nlmLogVarTable, myrow);
        }
    }

    /*
     * store the row 
     */
    netsnmp_table_dataset_add_row(nlmLogTable, row);

    check_log_size(0, NULL);
    DEBUGMSGTL(("log_notification", "done logging something\n"));
}

/** handles requests for the nlmLogTable table, if anything else needs to be done */
int
nlmLogTable_handler(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info *reqinfo,
                    netsnmp_request_info *requests)
{
    /*
     * perform anything here that you need to do.  The requests have
     * already been processed by the master table_dataset handler, but
     * this gives you chance to act on the request in some other way if
     * need be. 
     */
    return SNMP_ERR_NOERROR;
}

/** handles requests for the nlmLogVariableTable table, if anything else needs to be done */
int
nlmLogVariableTable_handler(netsnmp_mib_handler *handler,
                            netsnmp_handler_registration *reginfo,
                            netsnmp_agent_request_info *reqinfo,
                            netsnmp_request_info *requests)
{
    /*
     * perform anything here that you need to do.  The requests have
     * already been processed by the master table_dataset handler, but
     * this gives you chance to act on the request in some other way if
     * need be. 
     */
    return SNMP_ERR_NOERROR;
}
