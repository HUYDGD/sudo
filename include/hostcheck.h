/*
 * Copyright (c) 2020 Laszlo Orban <laszlo.orban@oneidentity.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef SUDO_HOSTCHECK_H
#define SUDO_HOSTCHECK_H

#if defined(HAVE_OPENSSL)

# include <openssl/x509v3.h>

/*
 * Compatibility defines for OpenSSL 1.0.2 (not needed for 1.1.x)
 */
# if !defined(HAVE_ASN1_STRING_GET0_DATA) && !defined(HAVE_WOLFSSL)
#  define ASN1_STRING_get0_data(x)	ASN1_STRING_data(x)
# endif /* !HAVE_ASN1_STRING_GET0_DATA && !HAVE_WOLFSSL */

typedef enum {
    MatchFound,
    MatchNotFound,
    NoSANPresent,
    MalformedCertificate,
    Error
} HostnameValidationResult;

HostnameValidationResult validate_hostname(const X509 *cert,
	const char *hostname, const char *ipaddr, int resolve);

#endif /* HAVE_OPENSSL */

#endif /* SUDO_HOSTCHECK_H */
