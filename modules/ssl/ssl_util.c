/*                      _             _
**  _ __ ___   ___   __| |    ___ ___| |  mod_ssl
** | '_ ` _ \ / _ \ / _` |   / __/ __| |  Apache Interface to OpenSSL
** | | | | | | (_) | (_| |   \__ \__ \ |  www.modssl.org
** |_| |_| |_|\___/ \__,_|___|___/___/_|  ftp.modssl.org
**                      |_____|
**  ssl_util.c
**  Utility Functions
*/

/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 */
                             /* ``Every day of my life
                                  I am forced to add another
                                  name to the list of people
                                  who piss me off!''
                                            -- Calvin          */
#include "mod_ssl.h"

#if 0 /* XXX */

/*  _________________________________________________________________
**
**  Utility Functions
**  _________________________________________________________________
*/

char *ssl_util_vhostid(pool *p, server_rec *s)
{
    char *id;
    SSLSrvConfigRec *sc;
    char *host;
    unsigned int port;

    host = s->server_hostname;
    if (s->port != 0)
        port = s->port;
    else {
        sc = mySrvConfig(s);
        if (sc->bEnabled)
            port = DEFAULT_HTTPS_PORT;
        else
            port = DEFAULT_HTTP_PORT;
    }
    id = ap_psprintf(p, "%s:%u", host, port);
    return id;
}

void ssl_util_strupper(char *s)
{
    for (; *s; ++s)
        *s = toupper(*s);
    return;
}

static const char ssl_util_uuencode_six2pr[64+1] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void ssl_util_uuencode(char *szTo, const char *szFrom, BOOL bPad)
{
    ssl_util_uuencode_binary((unsigned char *)szTo,
                             (const unsigned char *)szFrom,
                             strlen(szFrom), bPad);
}

void ssl_util_uuencode_binary(
    unsigned char *szTo, const unsigned char *szFrom, int nLength, BOOL bPad)
{
    const unsigned char *s;
    int nPad = 0;

    for (s = szFrom; nLength > 0; s += 3) {
        *szTo++ = ssl_util_uuencode_six2pr[s[0] >> 2];
        *szTo++ = ssl_util_uuencode_six2pr[(s[0] << 4 | s[1] >> 4) & 0x3f];
        if (--nLength == 0) {
            nPad = 2;
            break;
        }
        *szTo++ = ssl_util_uuencode_six2pr[(s[1] << 2 | s[2] >> 6) & 0x3f];
        if (--nLength == 0) {
            nPad = 1;
            break;
        }
        *szTo++ = ssl_util_uuencode_six2pr[s[2] & 0x3f];
        --nLength;
    }
    while(bPad && nPad--)
        *szTo++ = NUL;
    *szTo = NUL;
    return;
}

FILE *ssl_util_ppopen(server_rec *s, pool *p, char *cmd)
{
    FILE *fpout;
    int rc;

    fpout = NULL;
    rc = ap_spawn_child(p, ssl_util_ppopen_child,
                        (void *)cmd, kill_after_timeout,
                        NULL, &fpout, NULL);
    if (rc == 0 || fpout == NULL) {
        ap_log_error(APLOG_MARK, APLOG_ERR, s,
                     "ssl_util_ppopen: could not run: %s", cmd);
        return NULL;
    }
    return (fpout);
}

int ssl_util_ppopen_child(void *cmd, child_info *pinfo)
{
    int child_pid = 1;

    /*
     * Prepare for exec
     */
    ap_cleanup_for_exec();
#ifdef SIGHUP
    signal(SIGHUP, SIG_IGN);
#endif

    /*
     * Exec() the child program
     */
    execl(SHELL_PATH, SHELL_PATH, "-c", (char *)cmd, NULL);
    return (child_pid);
}

void ssl_util_ppclose(server_rec *s, pool *p, FILE *fp)
{
    ap_pfclose(p, fp);
    return;
}

/*
 * Run a filter program and read the first line of its stdout output
 */
char *ssl_util_readfilter(server_rec *s, pool *p, char *cmd)
{
    static char buf[MAX_STRING_LEN];
    FILE *fp;
    char c;
    int k;

    if ((fp = ssl_util_ppopen(s, p, cmd)) == NULL)
        return NULL;
    for (k = 0;    read(fileno(fp), &c, 1) == 1
                && (k < MAX_STRING_LEN-1)       ; ) {
        if (c == '\n' || c == '\r')
            break;
        buf[k++] = c;
    }
    buf[k] = NUL;
    ssl_util_ppclose(s, p, fp);

    return buf;
}

BOOL ssl_util_path_check(ssl_pathcheck_t pcm, char *path)
{
    struct stat sb;

    if (path == NULL)
        return FALSE;
    if (pcm & SSL_PCM_EXISTS && stat(path, &sb) != 0)
        return FALSE;
    if (pcm & SSL_PCM_ISREG && !S_ISREG(sb.st_mode))
        return FALSE;
    if (pcm & SSL_PCM_ISDIR && !S_ISDIR(sb.st_mode))
        return FALSE;
    if (pcm & SSL_PCM_ISNONZERO && sb.st_mode <= 0)
        return FALSE;
    return TRUE;
}

ssl_algo_t ssl_util_algotypeof(X509 *pCert, EVP_PKEY *pKey) 
{
    ssl_algo_t t;
            
    t = SSL_ALGO_UNKNOWN;
    if (pCert != NULL)
        pKey = X509_get_pubkey(pCert);
    if (pKey != NULL) {
        switch (EVP_PKEY_type(pKey->type)) {
            case EVP_PKEY_RSA: 
                t = SSL_ALGO_RSA;
                break;
            case EVP_PKEY_DSA: 
                t = SSL_ALGO_DSA;
                break;
            default:
                break;
        }
    }
    return t;
}

char *ssl_util_algotypestr(ssl_algo_t t) 
{
    char *cp;

    cp = "UNKNOWN";
    switch (t) {
        case SSL_ALGO_RSA: 
            cp = "RSA";
            break;
        case SSL_ALGO_DSA: 
            cp = "DSA";
            break;
        default:
            break;
    }
    return cp;
}

char *ssl_util_ptxtsub(
    pool *p, const char *cpLine, const char *cpMatch, char *cpSubst)
{
#define MAX_PTXTSUB 100
    char *cppMatch[MAX_PTXTSUB];
    char *cpResult;
    int nResult;
    int nLine;
    int nSubst;
    int nMatch;
    char *cpI;
    char *cpO;
    char *cp;
    int i;

    /*
     * Pass 1: find substitution locations and calculate sizes
     */
    nLine  = strlen(cpLine);
    nMatch = strlen(cpMatch);
    nSubst = strlen(cpSubst);
    for (cpI = (char *)cpLine, i = 0, nResult = 0;
         cpI < cpLine+nLine && i < MAX_PTXTSUB;    ) {
        if ((cp = strstr(cpI, cpMatch)) != NULL) {
            cppMatch[i++] = cp;
            nResult += ((cp-cpI)+nSubst);
            cpI = (cp+nMatch);
        }
        else {
            nResult += strlen(cpI);
            break;
        }
    }
    cppMatch[i] = NULL;
    if (i == 0)
        return NULL;

    /*
     * Pass 2: allocate memory and assemble result
     */
    cpResult = ap_pcalloc(p, nResult+1);
    for (cpI = (char *)cpLine, cpO = cpResult, i = 0; cppMatch[i] != NULL; i++) {
        ap_cpystrn(cpO, cpI, cppMatch[i]-cpI+1);
        cpO += (cppMatch[i]-cpI);
        ap_cpystrn(cpO, cpSubst, nSubst+1);
        cpO += nSubst;
        cpI = (cppMatch[i]+nMatch);
    }
    ap_cpystrn(cpO, cpI, cpResult+nResult-cpO+1);

    return cpResult;
}

/*  _________________________________________________________________
**
**  Special Functions for Win32/OpenSSL
**  _________________________________________________________________
*/

#ifdef WIN32
static HANDLE lock_cs[CRYPTO_NUM_LOCKS];

static void win32_locking_callback(int mode, int type, char* file, int line)
{
    if (mode & CRYPTO_LOCK)
        WaitForSingleObject(lock_cs[type], INFINITE);
    else
        ReleaseMutex(lock_cs[type]);
    return;
}
#endif /* WIN32 */

void ssl_util_thread_setup(void)
{
#ifdef WIN32
    int i;

    for (i = 0; i < CRYPTO_NUM_LOCKS; i++)
        lock_cs[i] = CreateMutex(NULL, FALSE, NULL);
    CRYPTO_set_locking_callback((void(*)(int, int, const char *, int))
                                win32_locking_callback);
#endif /* WIN32 */
    return;
}

#endif /* XXX */

