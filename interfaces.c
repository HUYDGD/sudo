/*
 *  CU sudo version 1.3.1
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Please send bugs, changes, problems to sudo-bugs@cs.colorado.edu
 *
 *******************************************************************
 *
 *  This module contains load_interfaces() a function that
 *  fills the interfaces global with a list of active ip
 *  addresses and their associated netmasks.
 *
 *  Todd C. Miller  Mon May  1 20:48:43 MDT 1995
 */

#ifndef lint
static char rcsid[] = "$Id$";
#endif /* lint */

#define MAIN

#include "config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif /* STDC_HEADERS */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */
#ifdef HAVE_MALLOC_H 
#include <malloc.h>   
#endif /* HAVE_MALLOC_H */ 
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#else
#include <sys/ioctl.h>
#endif /* HAVE_SYS_SOCKIO_H */
#ifdef _ISC
#include <sys/stream.h>
#include <sys/sioctl.h>
#include <sys/stropts.h>
#include <net/errno.h>
#define STRSET(cmd, param, len) {strioctl.ic_cmd=(cmd);\
                  strioctl.ic_dp=(param);\
                    strioctl.ic_len=(len);}
#endif /* _ISC */
#ifdef _MIPS
#include <net/soioctl.h>
#endif /* _MIPS */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/param.h>

#include "sudo.h"
#include "options.h"
#include "version.h"

#if !defined(STDC_HEADERS) && !defined(__GNUC__)
extern char *malloc	__P((size_t));
#endif /* !STDC_HEADERS && !__GNUC__ */

/*
 * Globals
 */
struct interface *interfaces;
int num_interfaces;
extern int Argc;
extern char **Argv;


/**********************************************************************
 *
 *  load_interfaces()
 *
 *  This function sets the interfaces global variable
 *  and sets the constituent ip addrs and netmasks.
 */

void load_interfaces()
{
    unsigned long localhost_mask;
    struct ifconf ifconf;
    struct ifreq ifreq;
    struct sockaddr_in *sin;
    char buf[BUFSIZ];
    int sock, i, j;
#ifdef _ISC
    struct strioctl strioctl;
#endif /* _ISC */

    /* so we can skip localhost and its ilk */
    localhost_mask = inet_addr("127.0.0.0");

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
	perror("socket");
	exit(1);
    }

    /*
     * get interface configuration or return (leaving interfaces NULL)
     */
    ifconf.ifc_len = sizeof(buf);
    ifconf.ifc_buf = buf;
#ifdef _ISC
    STRSET(SIOCGIFCONF, (caddr_t) &ifconf, sizeof(ifconf));
    if (ioctl(sock, I_STR, (caddr_t) &strioctl) < 0) {
	/* networking probably not installed in kernel */
	return;
    }
#else
    if (ioctl(sock, SIOCGIFCONF, (caddr_t) &ifconf) < 0) {
	/* networking probably not installed in kernel */
	return;
    }
#endif /* _ISC */

    /*
     * find out how many interfaces exist
     */
    num_interfaces = ifconf.ifc_len / sizeof(struct ifreq);

    /*
     * malloc() space for interfaces array
     */
    interfaces = (struct interface *) malloc(sizeof(struct interface) *
	num_interfaces);
    if (interfaces == NULL) {
	perror("malloc");
	(void) fprintf(stderr, "%s: cannot allocate memory!\n", Argv[0]);
	exit(1);
    }

    /*
     * for each interface, get the ip address and netmask
     */
    for (i = 0, j = 0; i < num_interfaces; i++) {
	(void) strncpy(ifreq.ifr_name, ifconf.ifc_req[i].ifr_name,
	    sizeof(ifreq.ifr_name));

	/* get the ip address */
#ifdef _ISC
	STRSET(SIOCGIFADDR, (caddr_t) &ifreq, sizeof(ifreq));
	if (ioctl(sock, I_STR, (caddr_t) &strioctl) < 0) {
#else
	if (ioctl(sock, SIOCGIFADDR, (caddr_t) &ifreq)) {
#endif /* _ISC */
	    /* non-fatal error if interface is down or not supported */
	    if (errno == EADDRNOTAVAIL || errno == ENXIO || errno == EAFNOSUPPORT)
		continue;

	    (void) fprintf(stderr, "%s: Error, ioctl: SIOCGIFADDR ", Argv[0]);
	    perror("");
	    exit(1);
	}
	sin = (struct sockaddr_in *) &ifreq.ifr_addr;

	/* make sure we don't have a dupe (usually consecutive) */
	if (j > 0 && memcmp(&interfaces[j-1].addr, &(sin->sin_addr),
	    sizeof(sin->sin_addr)) == 0)
	    continue;

	/* store the ip address */
	(void) memcpy(&interfaces[j].addr, &(sin->sin_addr),
	    sizeof(struct in_addr));

	/* get the netmask */
#ifdef SIOCGIFNETMASK
	(void) strncpy(ifreq.ifr_name, ifconf.ifc_req[i].ifr_name,
	    sizeof(ifreq.ifr_name));
#ifdef _ISC
	STRSET(SIOCGIFNETMASK, (caddr_t) &ifreq, sizeof(ifreq));
	if (ioctl(sock, I_STR, (caddr_t) &strioctl) == 0) {
#else
	if (ioctl(sock, SIOCGIFNETMASK, (caddr_t) &ifreq) == 0) {
#endif /* _ISC */
	    /* store the netmask */
	    (void) memcpy(&interfaces[j].netmask, &(sin->sin_addr),
		sizeof(struct in_addr));
	} else {
#else
	{
#endif /* SIOCGIFNETMASK */
	    if (IN_CLASSC(interfaces[j].addr.s_addr))
		interfaces[j].netmask.s_addr = htonl(IN_CLASSC_NET);
	    else if (IN_CLASSB(interfaces[j].addr.s_addr))
		interfaces[j].netmask.s_addr = htonl(IN_CLASSB_NET);
	    else
		interfaces[j].netmask.s_addr = htonl(IN_CLASSA_NET);
	}

	/* avoid localhost and friends */
	if ((interfaces[j].addr.s_addr & interfaces[j].netmask.s_addr) ==
	    localhost_mask)
	    continue;

	++j;
    }

    /* if there were bogus entries, realloc the array */
    if (i != j) {
	num_interfaces = j;
	interfaces = (struct interface *) realloc(interfaces,
	    sizeof(struct interface) * num_interfaces);
	if (interfaces == NULL) {
	    perror("realloc");
	    (void) fprintf(stderr, "%s: cannot allocate memory!\n", Argv[0]);
	    exit(1);
	}
    }
}
