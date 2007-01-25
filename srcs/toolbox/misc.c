/* 
 * Copyright (c) 2005, 2006 by KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: misc.c,v 1.3 2007/01/24 17:12:02 tat Exp $";

#include <u/libu_conf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <toolbox/misc.h>
#include <toolbox/carpal.h>
#include <toolbox/memory.h>

/**
 *  \defgroup misc Miscellaneous
 *  \{
 */

/** \brief Returns \c 0 if \p c is neither a space or a tab, not-zero otherwise.
 */
inline int u_isblank(int c)
{
    return c == ' ' || c == '\t';
}

/** \brief Removes leading and trailing blanks (spaces and tabs) from \p s
 */
void u_trim(char *s)
{
    char *p;

    if(!s)
        return;

    /* trim trailing blanks */
    p = s + strlen(s) -1;
    while(s < p && u_isblank(*p))
        --p;
    p[1] = 0;

    /* trim leading blanks */
    p = s;
    while(*p && u_isblank(*p))
        ++p;

    if(p > s)
        memmove(s, p, 1 + strlen(p));
}

/** \brief Returns \c 1 if \p ln is a blank string i.e. a string formed by 
           ONLY spaces and/or tabs characters.
 */
inline int u_isblank_str(const char *ln)
{
    for(; *ln; ++ln)
        if(!u_isblank(*ln))
            return 0;
    return 1;
}

/** \brief Returns \c 0 if \p c is neither a CR (\\r) or a LF (\\n), 
     not-zero otherwise.
 */
inline int u_isnl(int c)
{
    return c == '\n' || c == '\r';
}

/** \brief Dups the first \p len chars of \p s.
     Returns the dupped zero terminated string or \c NULL on error.
 */
char *u_strndup(const char *s, size_t len)
{
    char *cp;

    if((cp = (char*) u_malloc(len + 1)) == NULL)
        return NULL;
    memcpy(cp, s, len);
    cp[len] = 0;
    return cp;
}

/** \brief Dups the supplied string \p s */
char *u_strdup(const char *s)
{
    return u_strndup(s, strlen(s));
}

/** \brief Save the PID of the calling process to a file named \p pf 
     (that should be a fully qualified path).
     Returns \c 0 on success, not-zero on error.
 */
int u_savepid (const char *pf)
{
    FILE *pfp;

    dbg_return_if ((pfp = fopen(pf, "w")) == NULL, ~0);

    fprintf(pfp, "%ld\n", (long) getpid());
    fclose(pfp);

    return 0;
}

/** \brief  Safe string copy, see also the U_SSTRNCPY define 
  Safe string copy which null-terminates the destination string \a dst before
  copying the source string \a src for no more than \a size bytes.
  Returns a pointer to the destination string \a dst.
*/
char *u_sstrncpy (char *dst, const char *src, size_t size)
{
    dst[size] = '\0';
    return strncpy(dst, src, size);
}

/** \brief Dups the memory block \c src of size \c size.
     Returns the pointer of the dup'd block on success, \c NULL on error.
 */
void* u_memdup(const void *src, size_t size)
{
    void *p;

    p = u_malloc(size);
    if(p)
        memcpy(p, src, size);
    return p;
}

/**
 * \brief   tokenize the supplied \p wlist string
 *
 * Tokenize the \p delim separated string \p wlist and place its
 * pieces (at most \p tokv_sz - 1) into \p tokv.
 *
 * \param wlist     list of strings possibily separated by chars in \p delim 
 * \param delim     set of token separators
 * \param tokv      pre-allocated string array
 * \param tokv_sz   number of cells in \p tokv array
 *
 */
int u_tokenize (char *wlist, const char *delim, char **tokv, size_t tokv_sz)
{
    char **ap;

    dbg_return_if (wlist == NULL, ~0);
    dbg_return_if (delim == NULL, ~0);
    dbg_return_if (tokv == NULL, ~0);
    dbg_return_if (tokv_sz == 0, ~0);

    ap = tokv;

    for ( ; (*ap = strsep(&wlist, delim)) != NULL; )
    {
        /* skip empty field */
        if (**ap == '\0')
            continue;

        /* check bounds */
        if (++ap >= &tokv[tokv_sz - 1])
            break;
    }

    /* put an explicit stopper to tokv */
    *ap = NULL;

    return 0;
}

/**
 * \brief   snprintf-like function that returns 0 on success and ~0 on error
 *
 * snprintf-like function that returns 0 on success and ~0 on error
 *
 * \param str       destination buffer
 * \param size      size of \p str
 * \param fmt       snprintf format string
 *
 *   Returns \c 0 on success, not-zero on error.
 */
int u_snprintf(char *str, size_t size, const char *fmt, ...)
{
    va_list ap;
    int wr;

    va_start(ap, fmt);

    wr = vsnprintf(str, size, fmt, ap);

    va_end(ap);

    dbg_err_if(wr < 0 || wr >= (int)size);

    return 0;
err:
    return ~0;
}

/**
 * \brief   snprintf-like function that handle path separator issues
 *
 * Calls snprintf with the provided arguments and remove consecutive
 * path separators from the resulting string.
 *
 * \param buf       destination buffer
 * \param sz        size of \p str
 * \param sep       path separator to use ('/' or '\')
 * \param fmt       snprintf format string
 *
 *   Returns \c 0 on success, not-zero on error.
 */
int u_path_snprintf(char *buf, size_t sz, char sep, const char *fmt, ...)
{
    va_list ap;
    int wr, i, len;

    va_start(ap, fmt);

    wr = vsnprintf(buf, sz, fmt, ap);

    va_end(ap);

    dbg_err_if(wr < 0 || wr >= (int)sz);

    /* remove multiple consecutive '/' */
    for(len = i = strlen(buf); i > 0; --i)
        if(buf[i] == sep && buf[i-1] == sep)
            memmove(buf + i, buf + i + 1, len--);

    return 0;
err:
    return ~0;
}

inline void u_use_unused_args(char *dummy, ...)
{
    dummy = 0;
    return;
}

/** \brief  Return \c 1 if the supplied buffer \p data has non-ascii bytes */
int u_data_is_bin (char *data, size_t sz)
{
    size_t i;

    for (i = 0; i < sz; i++)
    {
        if (!isascii(data[i]))
            return 1;
    }

    return 0;
}

int u_data_dump (char *data, size_t sz, const char *file)
{
    FILE *fp = NULL;

    dbg_err_if ((fp = fopen(file, "w")) == NULL); 
    dbg_err_if (fwrite(data, sz, 1, fp) < 1);
    fclose(fp);

    return 0;
err:
    U_FCLOSE(fp); 
    return ~0;
}

/**
 *  \brief  Load file into memory from the supplied FS entry 
 *
 *  \param  path    the path name of the file 
 *  \param  sz_max  if >0 impose this size limit (otherwise unlimited)
 *  \param  pbuf    the pointer to the memory location holding the file's 
 *                  contents as a value-result argument
 *  \param  psz     size of the loaded file as a v-r argument
 *
 *  \return \c 0 on success, \c ~0 on error
 */ 
int u_load_file (const char *path, size_t sz_max, char **pbuf, size_t *psz)
{   
    FILE *fp = NULL;
    char *buf = NULL;
    size_t sz; 
    struct stat sb;
    
    dbg_return_if (path == NULL, ~0);
    dbg_return_if (pbuf == NULL, ~0);
    dbg_return_if (psz == NULL, ~0);
    
    warn_err_sifm ((fp = fopen(path, "r")) == NULL, "%s", path);
    warn_err_sifm (fstat(fileno(fp), &sb) == -1, "%s", path); 
    sz = sb.st_size;
    warn_err_ifm (sz_max > 0 && sz > sz_max, 
            "file too big (%zu vs %zu bytes)", sz, sz_max);
    warn_err_sif ((buf = u_zalloc(sz)) == NULL);
    warn_err_sifm (fread(buf, sz, 1, fp) != 1, "%s", path);
    
    U_FCLOSE(fp);
    
    *pbuf = buf;
    *psz = sz;
    
    return 0;
err:
    U_FCLOSE(fp);
    U_FREE(buf);
    return ~0;
}

/** \brief  Top level I/O routine 
 *
 * Try to read/write - atomically - a chunk of \p l bytes from/to the object 
 * referenced by the descriptor \p sd.  The data chunk is written to/read from
 * the buffer starting at \p buf.  The I/O driver function \p f is used to 
 * carry out the job, its interface and behaviour must conform to those of 
 * \c POSIX.1 \c read() or \c write().  If \p n is not \c NULL, it will store
 * the number of bytes actually read/written: this information is significant
 * only when u_net_io has failed.  If \p eof is not \c NULL, it will be set
 * to \c 1 on an end-of-file condition.
 *
 * \param f         the I/O function, i.e. \c read(2) or \c write(2)
 * \param sd        the file descriptor on which the I/O operation is performed
 * \param buf       the data chunk to be read or written
 * \param l         the length in bytes of \p buf
 * \param n         the number of bytes read/written as a value-result arg
 * \param eof       true if end-of-file condition
 * 
 * \return  A \c ~0 is returned if an error other than \c EINTR or \c EAGAIN 
 *          has occurred, or if the requested amount of data could 
 *          not be entirely read/written.  A \c 0 is returned on success.
 */
int u_io (iof_t f, int sd, void *buf, size_t l, ssize_t *n, int *eof)
{
#define SET_PPTR(pptr, val) do {if ((pptr)) *(pptr) = (val);} while (0);
    ssize_t nret;
    size_t nleft = l;
    char *p = buf;

    SET_PPTR(n, 0);
    SET_PPTR(eof, 0);

    while (nleft > 0) 
    {
        if ((nret = (f) (sd, p, nleft)) == -1)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            else
            {
                warn_strerror(errno);
                goto end;
            }
        }

        /* test EOF */
        if (nret == 0)
        {
            SET_PPTR(eof, 1);
            goto end;
        }
        
        nleft -= nret;
        p += nret;
    }

end:
    SET_PPTR(n, l - nleft);
    return nleft ? ~0 : 0;
#undef SET_PPTR
}

/**
 *  \brief  sleep(3) wrapper that handles EINTR and EAGAIN
 *
 *  \param  secs        sleep for 'secs' seconds
 *
 *  \return on success returns the socket descriptor; on failure returns -1
 */ 
int u_sleep(unsigned int secs)
{
    int sleep_for, c;

    for(sleep_for = secs; sleep_for > 0; sleep_for = c)
    {
        if((c = sleep(sleep_for)) == 0)
            break;
        else if(errno != EINTR && errno != EAGAIN)
            return -1; /* should never happen */
    }

    return 0;
}

/**
 *  \brief  accept(2) wrapper that handles EINTR and EAGAIN
 *
 *  \param  ld          file descriptor
 *  \param  addr        see accept(2)   
 *  \param  addrlen     size of addr struct
 *
 *  \return on success returns the socket descriptor; on failure returns -1
 */ 
int u_accept(int ld, struct sockaddr *addr, int *addrlen)
{
    int ad = -1;

again:
    ad = accept(ld, addr, addrlen);
    if(ad == -1 && (errno == EINTR || errno == EAGAIN))
        goto again; /* interrupted */

    return ad;
}

/**
 *  \brief  read(2) wrapper that handles EINTR and EAGAIN
 *
 *  \param  fd      file descriptor
 *  \param  buf     buffer to read into
 *  \param  size    size of the buffer
 *
 *  \return \c on success returns the number of bytes read (that will be always 'size' except on eof); on failure returns -1
 */ 
ssize_t u_read(int fd, void *buf, size_t size)
{
    ssize_t nw = 0;
    int eof = 0;

    if(u_io((iof_t) read, fd, buf, size, &nw, &eof))
    {
        if(eof)
            return nw; /* may be zero */
        else
            return -1;
    }

    return nw;
}

/**
 *  \brief  write(2) wrapper that handle EINTR and EAGAIN
 *
 *  \param  fd      file descriptor
 *  \param  buf     buffer to write
 *  \param  size    size of the buffer
 *
 *  \return \c size on success, -1 on error
 */ 
ssize_t u_write(int fd, void *buf, size_t size)
{
    ssize_t nw;

    if(u_io((iof_t) write, fd, buf, size, NULL, NULL))
        return -1;

    return size;
}

/**
 *      \}
 */