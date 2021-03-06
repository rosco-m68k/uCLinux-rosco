#
# This script was written by Renaud Deraison <deraison@cvs.nessus.org>
#
# See the Nessus Scripts License for details
#

if(description)
{
  script_id(11424);
  script_version ("$Revision: 1.3 $");
 
  
  name["english"] = "WebDAV enabled";
 

  script_name(english:name["english"], francais:name["francais"]);
  desc["english"] = "
The remote server is running with WebDAV enabled. 

WebDAV is an industry standard extension to the HTTP specification.
It adds a capability for authorized users to remotely add and manage
the content of a web server.

If you do not use this extension, you should disable it.

Solution : If you use IIS, refer to Microsoft KB article Q241520
Risk factor : Medium";


 script_description(english:desc["english"]);

 summary["english"] = "Checks the presence of WebDAV";
 summary["francais"] = "V?rifie la pr?sence de WebDAV";
 script_summary(english:summary["english"], francais:summary["francais"]);
 script_category(ACT_GATHER_INFO);

 script_copyright(english:"This script is Copyright (C) 2003 Renaud Deraison",
     	 	  francais:"Ce script est Copyright (C) 2003 Renaud Deraison");

 family["english"] = "General";

 script_family(english:family["english"]);

 script_dependencie("find_service.nes");
 script_require_ports("Services/www", 80);
 exit(0);
}


include("http_func.inc");

port = get_kb_item("Services/www");
if(!port) port = 80;

if(get_port_state(port))
{
  soc = http_open_socket(port);
  if(soc)
  {
  req = string("OPTIONS * HTTP/1.0\r\n\r\n") ;
  send(socket:soc, data:req);
  r = http_recv_headers(soc);
  close(soc);
  if(egrep(pattern:"^DAV: ", string:r))
   {
    security_warning(port);
   }
  }
}
