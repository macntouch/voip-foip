/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 2008, Eris Associates Limited, UK
 * Copyright (C) 1999 - 2005 Thorsten Lockert
 *
 * Written by Thorsten Lockert <tholo@trollphone.org>
 *
 * Funding provided by Troll Phone Networks AS
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

/*! \file
 *
 * \brief DNS Support for CallWeaver
 *
 * \author Thorsten Lockert <tholo at trollphone.org>
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <unistd.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/corelib/dns.c $", "$Revision: 4723 $")

#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/dns.h"
#define MAX_SIZE 4096

typedef struct {
	unsigned	id :16;		/* query identification number */
#if __BYTE_ORDER == __BIG_ENDIAN
			/* fields in third byte */
	unsigned	qr: 1;		/* response flag */
	unsigned	opcode: 4;	/* purpose of message */
	unsigned	aa: 1;		/* authoritive answer */
	unsigned	tc: 1;		/* truncated message */
	unsigned	rd: 1;		/* recursion desired */
			/* fields in fourth byte */
	unsigned	ra: 1;		/* recursion available */
	unsigned	unused :1;	/* unused bits (MBZ as of 4.9.3a3) */
	unsigned	ad: 1;		/* authentic data from named */
	unsigned	cd: 1;		/* checking disabled by resolver */
	unsigned	rcode :4;	/* response code */
#endif
#if __BYTE_ORDER == __LITTLE_ENDIAN || __BYTE_ORDER == __PDP_ENDIAN
			/* fields in third byte */
	unsigned	rd :1;		/* recursion desired */
	unsigned	tc :1;		/* truncated message */
	unsigned	aa :1;		/* authoritive answer */
	unsigned	opcode :4;	/* purpose of message */
	unsigned	qr :1;		/* response flag */
			/* fields in fourth byte */
	unsigned	rcode :4;	/* response code */
	unsigned	cd: 1;		/* checking disabled by resolver */
	unsigned	ad: 1;		/* authentic data from named */
	unsigned	unused :1;	/* unused bits (MBZ as of 4.9.3a3) */
	unsigned	ra :1;		/* recursion available */
#endif
			/* remaining bytes */
	unsigned	qdcount :16;	/* number of question entries */
	unsigned	ancount :16;	/* number of answer entries */
	unsigned	nscount :16;	/* number of authority entries */
	unsigned	arcount :16;	/* number of resource entries */
} dns_HEADER;

struct dn_answer {
	unsigned short rtype;
	unsigned short class;
	unsigned int ttl;
	unsigned short size;
} __attribute__ ((__packed__));

static int skip_name(char *s, int len)
{
	int x = 0;

	while (x < len) {
		if (*s == '\0') {
			s++;
			x++;
			break;
		}
		if ((*s & 0xc0) == 0xc0) {
			s += 2;
			x += 2;
			break;
		}
		x += *s + 1;
		s += *s + 1;
	}
	if (x >= len)
		return -1;
	return x;
}

/*--- dns_parse_answer: Parse DNS lookup result, call callback */
static int dns_parse_answer(void *context,
	int class, int type, char *answer, int len,
	int (*callback)(void *context, char *answer, int len, char *fullanswer))
{
	char *fullanswer = answer;
	struct dn_answer *ans;
	dns_HEADER *h;
	int res;
	int x;

	h = (dns_HEADER *)answer;
	answer += sizeof(dns_HEADER);
	len -= sizeof(dns_HEADER);

	for (x = 0; x < ntohs(h->qdcount); x++) {
		if ((res = skip_name(answer, len)) < 0) {
			cw_log(LOG_WARNING, "Couldn't skip over name\n");
			return -1;
		}
		answer += res + 4;	/* Skip name and QCODE / QCLASS */
		len -= res + 4;
		if (len < 0) {
			cw_log(LOG_WARNING, "Strange query size\n");
			return -1;
		}
	}

	for (x = 0; x < ntohs(h->ancount); x++) {
		if ((res = skip_name(answer, len)) < 0) {
			cw_log(LOG_WARNING, "Failed skipping name\n");
			return -1;
		}
		answer += res;
		len -= res;
		ans = (struct dn_answer *)answer;
		answer += sizeof(struct dn_answer);
		len -= sizeof(struct dn_answer);
		if (len < 0) {
			cw_log(LOG_WARNING, "Strange result size\n");
			return -1;
		}
		if (len < 0) {
			cw_log(LOG_WARNING, "Length exceeds frame\n");
			return -1;
		}

		if (ntohs(ans->class) == class && ntohs(ans->rtype) == type) {
			if (callback) {
				if ((res = callback(context, answer, ntohs(ans->size), fullanswer)) < 0) {
					cw_log(LOG_WARNING, "Failed to parse result\n");
					return -1;
				}
				if (res > 0)
					return 1;
			}
		}
		answer += ntohs(ans->size);
		len -= ntohs(ans->size);
	}
	return 0;
}


CW_MUTEX_DEFINE_STATIC(res_lock);

#if (defined(res_ninit) && !defined(__UCLIBC__))
#define HAS_RES_NINIT
struct state {
	struct __res_state rs;
	struct state *next;
};

static struct state *states;
#endif


/*--- cw_search_dns: Lookup record in DNS */
int cw_search_dns(void *context,
	   const char *dname, int class, int type,
	   int (*callback)(void *context, char *answer, int len, char *fullanswer))
{
	char answer[MAX_SIZE];
	int ret = -1;
#ifdef HAS_RES_NINIT
	struct state *s;

	cw_mutex_lock(&res_lock);
	if ((s = states))
		states = states->next;
	cw_mutex_unlock(&res_lock);

	if (!s && !(s = calloc(1, sizeof(*s))))
		return -1;

	if (!(ret = res_ninit(&s->rs))) {
		ret = res_nsearch(&s->rs, dname, class, type, (unsigned char *)answer, sizeof(answer));
		res_nclose(&s->rs);
	}

	cw_mutex_lock(&res_lock);
	s->next = states;
	states = s;
	cw_mutex_unlock(&res_lock);

	if (ret > 0 && (ret = dns_parse_answer(context, class, type, answer, ret, callback)) < 0)
		cw_log(LOG_WARNING, "DNS Parse error for %s\n", dname);
	if (ret == 0)
		cw_log(LOG_DEBUG, "No matches found in DNS for %s\n", dname);
#else
	cw_mutex_lock(&res_lock);
	if ((ret = res_init())) {
		ret = res_search(dname, class, type, answer, sizeof(answer));
#ifndef __APPLE__
		res_close();
#endif
	}
	cw_mutex_unlock(&res_lock);

	if (ret > 0 && (ret = dns_parse_answer(context, class, type, answer, ret, callback)) < 0)
		cw_log(LOG_WARNING, "DNS Parse error for %s\n", dname);
	if (ret == 0)
		cw_log(LOG_DEBUG, "No matches found in DNS for %s\n", dname);
#endif
	return ret;
}
