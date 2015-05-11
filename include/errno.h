/*
 * errno.h - definicion de los numeros y mensajes de error
 *          
 */
 
#ifndef __ERRNO_H__
#define __ERRNO_H__

extern unsigned int errno;

#define EBADF       9
#define EBNULL      10  
#define ESIZE       11
#define EWCON       12
#define EACCES      13
#define ENOPCB      14
#define ENOMEM	    15
#define ENOLOG      16
#define ENOEXIST    17
#define ERCON       18
#define ENOSYS      38
#define ENOSEM	    40
#define ENOWNER	    41




#endif  /* __ERRNO_H__ */
