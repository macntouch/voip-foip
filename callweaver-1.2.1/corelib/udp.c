/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 2006, Steve Underwood
 *
 * Steve Underwood <steveu@coppice.org>
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

/*
 *
 * A simple abstration of UDP ports so they can be handed between streaming
 * protocols, such as when RTP switches to UDPTL on the same IP port.
 * 
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

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/corelib/udp.c $", "$Revision: 4723 $")

#include "callweaver/frame.h"
#include "callweaver/logger.h"
#include "callweaver/options.h"
#include "callweaver/channel.h"
#include "callweaver/acl.h"
#include "callweaver/channel.h"
#include "callweaver/config.h"
#include "callweaver/lock.h"
#include "callweaver/utils.h"
#include "callweaver/cli.h"
#include "callweaver/unaligned.h"
#include "callweaver/utils.h"
#include "callweaver/udp.h"
#include "callweaver/stun.h"

struct udp_socket_info_s
{
    int fd;
    struct sockaddr_in us;
    struct sockaddr_in them;
    int nochecksums;
    int nat;
    struct sockaddr_in stun_me;
    int stun_state;
    struct udp_socket_info_s *next;
    struct udp_socket_info_s *prev;
};

static uint16_t make_mask16(uint16_t x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    return x;
}

int udp_socket_destroy(udp_socket_info_t *info)
{
    if (info == NULL)
        return -1;
    if (info->fd >= 0)
        close(info->fd);
    free(info);
    return 0;
}

udp_socket_info_t *udp_socket_create(int nochecksums)
{
    int fd;
    long flags;
    udp_socket_info_t *info;
    
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        cw_log(LOG_ERROR, "Unable to allocate socket: %s\n", strerror(errno));
        return NULL;
    }
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#ifdef SO_NO_CHECK
    if (nochecksums)
        setsockopt(fd, SOL_SOCKET, SO_NO_CHECK, &nochecksums, sizeof(nochecksums));
#endif
    if ((info = malloc(sizeof(*info))) == NULL)
    {
        cw_log(LOG_ERROR, "Unable to allocate socket data: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }
    memset(info, 0, sizeof(*info));
    info->them.sin_family = AF_INET;
    info->us.sin_family = AF_INET;
    info->nochecksums = nochecksums;
    info->fd = fd;
    info->stun_state = STUN_STATE_IDLE;
    info->next = NULL;
    info->prev = NULL;
    return info;
}

int udp_socket_destroy_group(udp_socket_info_t *info)
{
    udp_socket_info_t *infox;
    udp_socket_info_t *infoy;

    /* Assume info could be in the middle of the group, and search both ways when
       destroying */
    infox = info->next;
    while (infox)
    {
        infoy = infox->next;
        udp_socket_destroy(infox);
        infox = infoy;
    }
    infox = info;
    while (infox)
    {
        infoy = infox->prev;
        udp_socket_destroy(infox);
        infox = infoy;
    }
    return 0;
}

udp_socket_info_t *udp_socket_create_group_with_bindaddr(int nochecksums, int group, struct in_addr *addr, int startport, int endport)
{
    udp_socket_info_t *info;
    udp_socket_info_t *info_extra;
    struct sockaddr_in sockaddr;
    int i;
    int x;
    int xx;
    int port_mask;
    int startplace;

    if ((info = udp_socket_create(nochecksums)) == NULL)
        return NULL;
    if (group > 1)
    {
        info_extra = info;
        for (i = 1;  i < group;  i++)
        {
            if ((info_extra->next = udp_socket_create(nochecksums)) == NULL)
            {
                /* Unravel what we did so far, and give up */
                udp_socket_destroy_group(info);
                return NULL;
            }
            info_extra->next->prev = info_extra;
            info_extra = info_extra->next;
        }
    }

    /* Find a port or group of ports we can bind to, within a specified numeric range */
    port_mask = make_mask16(group);
    x = ((rand()%(endport - startport)) + startport) & ~port_mask;
    startplace = x;
    for (;;)
    {
        xx = x;
        memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin_addr = *addr;
        sockaddr.sin_port = htons(xx);
        info_extra = info;
        while (info_extra)
        {
            if (udp_socket_set_us(info_extra, &sockaddr))
                break;
            sockaddr.sin_port = htons(++xx);
            info_extra = info_extra->next;
        }
        if (info_extra == NULL)
        {
            /* We must have bound them all OK. */
            return info;
        }
        if (errno != EADDRINUSE)
        {
            cw_log(LOG_ERROR, "Unexpected bind error: %s\n", strerror(errno));
            udp_socket_destroy_group(info);
            return NULL;
        }
        x += (port_mask + 1);
        if (x > endport)
            x = (startport + port_mask) & ~port_mask;
        if (x == startplace)
            break;
    }
    cw_log(LOG_ERROR, "No ports available within the range %d to %d. Can't setup media stream.\n", startport, endport);
    /* Unravel what we did so far, and give up */
    udp_socket_destroy_group(info);
    return NULL;
}

udp_socket_info_t *udp_socket_find_group_element(udp_socket_info_t *info, int element)
{
    int i;

    /* Find the start of the group */
    while (info->prev)
        info = info->prev;
    /* Now count to the element we want */
    for (i = 0;  i < element  &&  info;  i++)
        info = info->next;
    return info;
}

int udp_socket_set_us(udp_socket_info_t *info, const struct sockaddr_in *us)
{
    int res;
    long flags;

    if (info == NULL  ||  info->fd < 0)
        return -1;

    if (info->us.sin_addr.s_addr  ||  info->us.sin_port)
    {
        /* We are already bound, so we need to re-open the socket to unbind it */
        close(info->fd);
        if ((info->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            cw_log(LOG_ERROR, "Unable to re-allocate socket: %s\n", strerror(errno));
            return -1;
        }
        flags = fcntl(info->fd, F_GETFL);
        fcntl(info->fd, F_SETFL, flags | O_NONBLOCK);
#ifdef SO_NO_CHECK
        if (info->nochecksums)
            setsockopt(info->fd, SOL_SOCKET, SO_NO_CHECK, &info->nochecksums, sizeof(info->nochecksums));
#endif
    }
    info->us.sin_port = us->sin_port;
    info->us.sin_addr.s_addr = us->sin_addr.s_addr;
       if ((res = bind(info->fd, (struct sockaddr *) &info->us, sizeof(info->us))) < 0)
    {
        info->us.sin_port = 0;
        info->us.sin_addr.s_addr = 0;
    }
    return res;
}

void udp_socket_set_them(udp_socket_info_t *info, const struct sockaddr_in *them)
{
    info->them.sin_port = them->sin_port;
    info->them.sin_addr.s_addr = them->sin_addr.s_addr;
}

int udp_socket_set_tos(udp_socket_info_t *info, int tos)
{
    int res;

    if (info == NULL)
        return -1;
    if ((res = setsockopt(info->fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)))) 
        cw_log(LOG_WARNING, "Unable to set TOS to %d\n", tos);
    return res;
}

void udp_socket_set_nat(udp_socket_info_t *info, int nat_mode)
{
    if (info == NULL)
        return;
    info->nat = nat_mode;
    if (nat_mode  &&  info->stun_state == STUN_STATE_IDLE  &&  stun_active)
    {
        if (stundebug)
            cw_log(LOG_DEBUG, "Sending stun request on this UDP channel (port %d) cause NAT is on\n",ntohs(info->us.sin_port) );
        cw_udp_stun_bindrequest(info->fd, &stunserver_ip, NULL, NULL);
        info->stun_state = STUN_STATE_REQUEST_PENDING;
    }
}

int udp_socket_restart(udp_socket_info_t *info)
{
    if (info == NULL)
        return -1;
    memset(&info->them.sin_addr.s_addr, 0, sizeof(info->them.sin_addr.s_addr));
    info->them.sin_port = 0;
    return 0;
}

int udp_socket_fd(udp_socket_info_t *info)
{
    if (info == NULL)
        return -1;
    return info->fd;
}

int udp_socket_get_stunstate(udp_socket_info_t *info)
{
    if (info)
        return info->stun_state;
    return 0;
}

void udp_socket_set_stunstate(udp_socket_info_t *info, int state)
{
    info->stun_state = state;
}

const struct sockaddr_in *udp_socket_get_stun(udp_socket_info_t *info)
{
    static const struct sockaddr_in dummy = {0};

    if (info)
        return &info->stun_me;
    return &dummy;
}

void udp_socket_set_stun(udp_socket_info_t *info, const struct sockaddr_in *stun)
{
    memcpy(&info->stun_me, stun, sizeof(struct sockaddr_in));
}

const struct sockaddr_in *udp_socket_get_us(udp_socket_info_t *info)
{
    static const struct sockaddr_in dummy = {0};

    if (info)
        return &info->us;
    return &dummy;
}

const struct sockaddr_in *udp_socket_get_apparent_us(udp_socket_info_t *info)
{
    static const struct sockaddr_in dummy = {0};

    if (info)
    {
        if (info->stun_state == STUN_STATE_RESPONSE_RECEIVED)
            return &info->stun_me;
        return &info->us;
    }
    return &dummy;
}

const struct sockaddr_in *udp_socket_get_them(udp_socket_info_t *info)
{
    static const struct sockaddr_in dummy = {0};

    if (info)
        return &info->them;
    return &dummy;
}

int udp_socket_recvfrom(udp_socket_info_t *info,
                        void *buf,
                        size_t size,
                        int flags,
                        struct sockaddr *sa,
                        socklen_t *salen,
                        int *action)
{
    struct sockaddr_in stun_sin;
    struct stun_state stun_me;
    int res;

    *action = 0;
    if (info == NULL  ||  info->fd < 0)
        return 0;
    if ((res = recvfrom(info->fd, buf, size, flags, sa, salen)) >= 0)
    {
        if ((info->nat  &&  !stun_active)
            ||
            (info->nat  &&  stun_active  &&  info->stun_state == STUN_STATE_IDLE))
        {
            /* Send to whoever sent to us */
            if (info->them.sin_addr.s_addr != ((struct sockaddr_in *) sa)->sin_addr.s_addr
                || 
                   info->them.sin_port != ((struct sockaddr_in *) sa)->sin_port)
            {
                memcpy(&info->them, sa, sizeof(info->them));
                *action |= 1;
            }
        }
        if (info->stun_state == STUN_STATE_REQUEST_PENDING)
        {
            if (stundebug)
                cw_log(LOG_DEBUG, "Checking if payload it is a stun RESPONSE\n");
            memset(&stun_me, 0, sizeof(struct stun_state));
            stun_handle_packet(info->stun_state, (struct sockaddr_in *) sa, buf, res, &stun_me);
            if (stun_me.msgtype == STUN_BINDRESP)
            {
                if (stundebug)
                    cw_log(LOG_DEBUG, "Got STUN bind response\n");
                info->stun_state = STUN_STATE_RESPONSE_RECEIVED;
                if (stun_addr2sockaddr(&stun_sin, stun_me.mapped_addr))
                {
                    memcpy(&info->stun_me, &stun_sin, sizeof(struct sockaddr_in));
                }
                else
                {
                    if (stundebug)
                        cw_log(LOG_DEBUG, "Stun response did not contain mapped address\n");
                }
                stun_remove_request(&stun_me.id);
                return -1;
            }
        }
    }
    return res;
}

int udp_socket_sendto(udp_socket_info_t *info, void *buf, size_t size, int flags)
{
    if (info == NULL  ||  info->fd < 0)
        return 0;
    if (info->them.sin_port == 0)
        return 0;
    return sendto(info->fd, buf, size, flags, (struct sockaddr *) &info->them, sizeof(info->them));
}
