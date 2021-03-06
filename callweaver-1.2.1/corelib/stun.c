/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * See http://www.callweaver.org for more information about
 * the CallWeaver project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/corelib/stun.c $", "$Revision: 4723 $")

#include "callweaver/udp.h"
#include "callweaver/rtp.h"
#include "callweaver/lock.h"
#include "callweaver/stun.h"
#include "callweaver/logger.h"
#include "callweaver/cli.h"
#include "callweaver/utils.h"
#include "callweaver/options.h"
#include "callweaver/udpfromto.h"

char stunserver_host[MAXHOSTNAMELEN] = "";
struct sockaddr_in stunserver_ip;
int stunserver_portno;
int stun_active = 0;
int stundebug = 0;

struct stun_request *stun_req_queue;

static const char *stun_msg2str(int msg)
{
    switch (msg)
    {
    case STUN_BINDREQ:
        return "Binding Request";
    case STUN_BINDRESP:
        return "Binding Response";
    case STUN_BINDERR:
        return "Binding Error Response";
    case STUN_SECREQ:
        return "Shared Secret Request";
    case STUN_SECRESP:
        return "Shared Secret Response";
    case STUN_SECERR:
        return "Shared Secret Error Response";
    }
    return "Non-RFC3489 Message";
}

static const char *stun_attr2str(int msg)
{
    switch (msg)
    {
    case STUN_MAPPED_ADDRESS:
        return "Mapped Address";
    case STUN_RESPONSE_ADDRESS:
        return "Response Address";
    case STUN_CHANGE_REQUEST:
        return "Change Request";
    case STUN_SOURCE_ADDRESS:
        return "Source Address";
    case STUN_CHANGED_ADDRESS:
        return "Changed Address";
    case STUN_USERNAME:
        return "Username";
    case STUN_PASSWORD:
        return "Password";
    case STUN_MESSAGE_INTEGRITY:
        return "Message Integrity";
    case STUN_ERROR_CODE:
        return "Error Code";
    case STUN_UNKNOWN_ATTRIBUTES:
        return "Unknown Attributes";
    case STUN_REFLECTED_FROM:
        return "Reflected From";
    }
    return "Non-RFC3489 Attribute";
}

int stun_addr2sockaddr(struct sockaddr_in *sin, struct stun_addr *addr)
{
    if (addr)
    {
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = addr->addr;
        sin->sin_port = addr->port;
        return 1;
    }
    return 0;
}

static int stun_process_attr(struct stun_state *state, struct stun_attr *attr)
{
    char iabuf[INET_ADDRSTRLEN];
    struct sockaddr_in sin;

    if (stundebug  &&  option_debug)
        cw_verbose("Found STUN Attribute %s (%04x), length %d\n",
            stun_attr2str(ntohs(attr->attr)), ntohs(attr->attr), ntohs(attr->len));
    switch (ntohs(attr->attr))
    {
    case STUN_USERNAME:
        state->username = (unsigned char *)(attr->value);
        break;
    case STUN_PASSWORD:
        state->password = (unsigned char *)(attr->value);
        break;
    case STUN_MAPPED_ADDRESS:
        state->mapped_addr = (struct stun_addr *)(attr->value);
        if (stundebug)
        {
            stun_addr2sockaddr(&sin, state->mapped_addr);
            cw_verbose("STUN: Mapped address is %s\n", cw_inet_ntoa(iabuf, sizeof(iabuf), sin.sin_addr));
            cw_verbose("STUN: Mapped port is %d\n", ntohs(state->mapped_addr->port));
        }
        break;
    case STUN_RESPONSE_ADDRESS:
        state->response_addr = (struct stun_addr *)(attr->value);
        break;
    case STUN_SOURCE_ADDRESS:
        state->source_addr = (struct stun_addr *)(attr->value);
        break;
    default:
        if (stundebug && option_debug)
        {
            cw_verbose("Ignoring STUN attribute %s (%04x), length %d\n", 
                         stun_attr2str(ntohs(attr->attr)),
                         ntohs(attr->attr),
                         ntohs(attr->len));
        }
        break;
    }
    return 0;
}

static void append_attr_string(struct stun_attr **attr, int attrval, const char *s, int *len, int *left)
{
    int size = sizeof(**attr) + strlen(s);

    if (*left > size)
    {
        (*attr)->attr = htons(attrval);
        (*attr)->len = htons(strlen(s));
        memcpy((*attr)->value, s, strlen(s));
        (*attr) = (struct stun_attr *)((*attr)->value + strlen(s));
        *len += size;
        *left -= size;
    }
}

static void append_attr_address(struct stun_attr **attr, int attrval, struct sockaddr_in *sin, int *len, int *left)
{
    int size = sizeof(**attr) + 8;
    struct stun_addr *addr;
    
    if (*left > size)
    {
        (*attr)->attr = htons(attrval);
        (*attr)->len = htons(8);
        addr = (struct stun_addr *)((*attr)->value);
        addr->unused = 0;
        addr->family = 0x01;
        addr->port = sin->sin_port;
        addr->addr = sin->sin_addr.s_addr;
        (*attr) = (struct stun_attr *)((*attr)->value + 8);
        *len += size;
        *left -= size;
    }
}

static int stun_send(int s, struct sockaddr_in *dst, struct stun_header *resp)
{
    return sendto(s,
                  resp,
                  ntohs(resp->msglen) + sizeof(*resp),
                  0,
                  (struct sockaddr *) dst,
                  sizeof(*dst));
/*
    // Alternative way to send STUN PACKETS using CallWeaver library functions.
    return cw_sendfromto(
	s,
	resp,ntohs(resp->msglen) + sizeof(*resp),0,
	NULL,0,
        (struct sockaddr *) dst,
        sizeof( struct sockaddr_in ) 
	);
*/
}


static void stun_req_id(struct stun_header *req)
{
    int x;
    
    for (x = 0;  x < 4;  x++)
        req->id.id[x] = cw_random();
}

/* ************************************************************************* */

struct stun_addr *cw_stun_find_request(stun_trans_id *st)
{
    struct stun_request *req_queue;
    struct stun_addr *a = NULL;

    req_queue=stun_req_queue;

    if (stundebug)
        cw_verbose("** Trying to lookup stun response for this sip packet %d\n", st->id[0]);
    while (req_queue != NULL)
    {
        //cw_verbose("** STUN FIND REQUEST compare trans_id %d\n",req_queue->req_head.id.id[0]);

        if (req_queue->got_response
            &&
            !memcmp((void *) &req_queue->req_head.id, st, sizeof(stun_trans_id))) 
        {
            if (stundebug)
                cw_verbose("** Found request in request queue for reqresp lookup\n");
            struct sockaddr_in sin;
            stun_addr2sockaddr(&sin, &req_queue->mapped_addr);
            //char iabuf[INET_ADDRSTRLEN];
            //cw_verbose("STUN: passing Mapped address is %s\n", cw_inet_ntoa(iabuf, sizeof(iabuf), sin.sin_addr));
            //cw_verbose("STUN: passing Mapped port is %d\n", ntohs(req_queue->mapped_addr.port));
            a = &req_queue->mapped_addr;        
            if (!stundebug)
                return a;
        }
        req_queue = req_queue->next;
    }
    return a;
}

/* ************************************************************************* */
int stun_remove_request(stun_trans_id *st)
{
    struct stun_request *req_queue;
    struct stun_request *req_queue_prev;
    struct stun_request *delqueue;
    time_t now;
    int found = 0;

    time(&now);
    req_queue = stun_req_queue;
    req_queue_prev = NULL;

    if (stundebug)
        cw_verbose("** Trying to lookup for removal stun queue %d\n", st->id[0]);
    while (req_queue != NULL)
    {
        if (req_queue->got_response
            &&
            !memcmp((void *) &req_queue->req_head.id, st, sizeof(stun_trans_id))) 
        {
            found = 1;
            delqueue = req_queue;
            if (stundebug)
                cw_verbose("** Found: request in removal stun queue %d\n", st->id[0]);
            if (req_queue_prev != NULL)
            {
                req_queue_prev->next = req_queue->next;
                req_queue = req_queue_prev;
            }
            else
            {
                stun_req_queue = req_queue->next;
                req_queue = stun_req_queue;
            }
            free(delqueue);
        }
        req_queue_prev = req_queue;
        if (req_queue != NULL)
            req_queue = req_queue->next;
    }
    if (!found)
        cw_verbose("** Not Found: request in removal stun queue %d\n", st->id[0]);

    /* Removing old requests, caused by "sip reload" whose requests are not linked to any transmission */
    req_queue = stun_req_queue;
    req_queue_prev = NULL;
    while (req_queue != NULL)
    {
        if (req_queue->whendone + 300 < now)
        {
            if (stundebug)
                cw_verbose("** DROP: request in removal stun queue %d (too old)\n",req_queue->req_head.id.id[0]);
            delqueue = req_queue;
            if (req_queue_prev != NULL)
            {
                req_queue_prev->next = req_queue->next;
                req_queue = req_queue_prev;
            }
            else
            {
                stun_req_queue = req_queue->next;
                req_queue = stun_req_queue;
            }
            free(delqueue);
        }
        req_queue_prev = req_queue;
        if (req_queue)
            req_queue = req_queue->next;
    }
    return 0;
}

/* ************************************************************************* */

struct stun_request *cw_udp_stun_bindrequest(int fdus,
                                               struct sockaddr_in *suggestion, 
                                               const char *username,
                                               const char *password)
{
    //struct stun_request *req;
    struct stun_header *reqh;
    unsigned char reqdata[1024];
    int reqlen, reqleft;
    struct stun_attr *attr;
    struct stun_request *myreq=NULL;

    reqh = (struct stun_header *)reqdata;
    stun_req_id(reqh);
    reqlen = 0;
    reqleft = sizeof(reqdata) - sizeof(struct stun_header);
    reqh->msgtype = 0;
    reqh->msglen = 0;
    attr = (struct stun_attr *)reqh->ies;

    if (username)
        append_attr_string(&attr, STUN_USERNAME, username, &reqlen, &reqleft);
    if (password)
        append_attr_string(&attr, STUN_PASSWORD, password, &reqlen, &reqleft);

    reqh->msglen = htons(reqlen);
    reqh->msgtype = htons(STUN_BINDREQ);

    if ((myreq = malloc(sizeof(struct stun_request))) != NULL)
    {
        memset(myreq, 0, sizeof(struct stun_request));
        memcpy(&myreq->req_head, reqh, sizeof(struct stun_header));
        if (stun_send(fdus, suggestion, reqh) != -1)
        {
            if (stundebug) 
                cw_verbose("** STUN Packet SENT %d %d\n", reqh->id.id[0], myreq->req_head.id.id[0]);
            time(&myreq->whendone);
            myreq->next = stun_req_queue;
            stun_req_queue = myreq;
            return myreq;
        }
        else
        {
            free(myreq);
        }
    }

    return NULL;
}

int stun_handle_packet(int s, 
                       struct sockaddr_in *src, 
                       unsigned char *data, size_t len, 
                       struct stun_state *st)    
{
    struct stun_header *resp;
    struct stun_header *hdr = (struct stun_header *)data;
    struct stun_attr *attr;
    //struct stun_state st;
    int ret = STUN_IGNORE;    
    unsigned char respdata[1024];
    int resplen;
    int respleft;
    struct stun_request *req_queue;
    struct stun_request *req_queue_prev;

    memset(st, 0, sizeof(st));
    memcpy(&st->id, &hdr->id, sizeof(stun_trans_id));

    if (len < sizeof(struct stun_header))
    {
        if (option_debug)
            cw_log(LOG_DEBUG, "Runt STUN packet (only %zd, wanting at least %zd)\n", len, sizeof(struct stun_header));
        return -1;
    }
    if (stundebug)
        cw_verbose("STUN Packet, msg %s (%04x), length: %d\n", stun_msg2str(ntohs(hdr->msgtype)), ntohs(hdr->msgtype), ntohs(hdr->msglen));

    if (ntohs(hdr->msglen) > len - sizeof(struct stun_header))
    {
        if (option_debug)
            cw_log(LOG_DEBUG, "Scrambled STUN packet length (got %d, expecting %zd)\n", ntohs(hdr->msglen), len - sizeof(struct stun_header));
    }
    else
        len = ntohs(hdr->msglen);
    data += sizeof(struct stun_header);

    while (len)
    {
        if (len < sizeof(struct stun_attr))
        {
            if (option_debug)
                cw_log(LOG_DEBUG, "Runt Attribute (got %zd, expecting %zd)\n", len, sizeof(struct stun_attr));
            break;
        }
        attr = (struct stun_attr *) data;
        if ((ntohs(attr->len) + sizeof(struct stun_attr)) > len)
        {
            if (option_debug)
                cw_log(LOG_DEBUG, "Inconsistent Attribute (length %d exceeds remaining msg len %zd)\n", ntohs(attr->len), len);
            break;
        }

        if (stun_process_attr(st, attr))
        {
            if (option_debug)
                cw_log(LOG_DEBUG, "Failed to handle attribute %s (%04x)\n", stun_attr2str(ntohs(attr->attr)), ntohs(attr->attr));
            break;
        }
        /* Clear attribute in case previous entry was a string */
        attr->attr = 0;
        data += ntohs(attr->len) + sizeof(struct stun_attr);
        len -= ntohs(attr->len) + sizeof(struct stun_attr);
    }

    /* Null terminate any string */
    *data = '\0';
    resp = (struct stun_header *)respdata;
    resplen = 0;
    respleft = sizeof(respdata) - sizeof(struct stun_header);
    resp->id = hdr->id;
    resp->msgtype = 0;
    resp->msglen = 0;
    attr = (struct stun_attr *)resp->ies;
    if (!len)
    {
        st->msgtype=ntohs(hdr->msgtype);
        switch (ntohs(hdr->msgtype))
        {
        case STUN_BINDREQ:
            if (stundebug)
            {
                cw_verbose("STUN Bind Request, username: %s\n", 
                             st->username  ?  (const char *) st->username  :  "<none>");
            }
            if (st->username)
                append_attr_string(&attr, STUN_USERNAME, (const char *)st->username, &resplen, &respleft);
            append_attr_address(&attr, STUN_MAPPED_ADDRESS, src, &resplen, &respleft);
            resp->msglen = htons(resplen);
            resp->msgtype = htons(STUN_BINDRESP);
            stun_send(s, src, resp);
            ret = STUN_ACCEPT;
            break;
        case STUN_BINDRESP:
            if (stundebug)
                cw_verbose("** STUN Bind Response\n");
            req_queue = stun_req_queue;
            req_queue_prev = NULL;
            while (req_queue != NULL)
            {
                if (!req_queue->got_response
                    && 
                    memcmp((void *) &req_queue->req_head.id, (void *) &st->id, sizeof(stun_trans_id)) == 0)
                {
                    if (stundebug)
                        cw_verbose("** Found response in request queue. ID: %d done at: %ld gotresponse: %d\n",req_queue->req_head.id.id[0],(long int)req_queue->whendone,req_queue->got_response);
                    req_queue->got_response = 1;
                    memcpy(&req_queue->mapped_addr, st->mapped_addr, sizeof(struct stun_addr));
                }
                else
                {
                    if (stundebug)
                        cw_verbose("** STUN request not matching. ID: %d done at: %ld gotresponse %d:\n",req_queue->req_head.id.id[0],(long int)req_queue->whendone,req_queue->got_response);
                }

                req_queue_prev = req_queue;
                req_queue = req_queue->next;
            }
            ret = STUN_ACCEPT;
            break;
        default:
            if (stundebug)
                cw_verbose("Dunno what to do with STUN message %04x (%s)\n", ntohs(hdr->msgtype), stun_msg2str(ntohs(hdr->msgtype)));
        }
    }
    return ret;
}

/* ************************************************************************* */

int stun_do_debug(int fd, int argc, char *argv[])
{
    if (argc != 2)
        return RESULT_SHOWUSAGE;
    stundebug = 1;
    cw_cli(fd, "STUN Debugging Enabled\n");
    return RESULT_SUCCESS;
}
   
int stun_no_debug(int fd, int argc, char *argv[])
{
    if (argc != 3)
        return RESULT_SHOWUSAGE;
    stundebug = 0;
    cw_cli(fd, "STUN Debugging Disabled\n");
    return RESULT_SUCCESS;
}

/* ************************************************************************* */

static char stun_debug_usage[] =
  "Usage: stun debug\n"
  "       Enable STUN (Simple Traversal of UDP through NATs) debugging\n";

static char stun_no_debug_usage[] =
  "Usage: stun no debug\n"
  "       Disable STUN debugging\n";

static struct cw_cli_entry  cli_stun_debug =
{{ "stun", "debug", NULL } , stun_do_debug, "Enable STUN debugging", stun_debug_usage };

static struct cw_cli_entry  cli_stun_no_debug =
{{ "stun", "no", "debug", NULL } , stun_no_debug, "Disable STUN debugging", stun_no_debug_usage };

void cw_stun_init(void)
{
    stundebug = 0;
    stun_active = 0;
    stun_req_queue = NULL;
    cw_cli_register(&cli_stun_debug);
    cw_cli_register(&cli_stun_no_debug);
}
