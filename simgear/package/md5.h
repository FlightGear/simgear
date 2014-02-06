     /*	$OpenBSD: md5.h,v 1.15 2004/05/03 17:30:14 millert Exp $	*/

/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 */

#ifndef _MD5_H_
#define _MD5_H_

#ifdef __cplusplus
 extern "C" {
#endif
     
#if defined(_MSC_VER)
typedef unsigned char    u_int8_t;
typedef unsigned int     u_int32_t;
typedef unsigned __int64 u_int64_t;
#endif
     
#define	MD5_BLOCK_LENGTH		64
#define	MD5_DIGEST_LENGTH		16
#define	MD5_DIGEST_STRING_LENGTH	(MD5_DIGEST_LENGTH * 2 + 1)

typedef struct MD5Context {
	u_int32_t state[4];			/* state */
	u_int64_t count;			/* number of bits, mod 2^64 */
	u_int8_t buffer[MD5_BLOCK_LENGTH];	/* input buffer */
} SG_MD5_CTX;

void	 SG_MD5Init(SG_MD5_CTX *);
void	 SG_MD5Update(SG_MD5_CTX *, const u_int8_t *, size_t);
void	 SG_MD5Pad(SG_MD5_CTX *);
void	 SG_MD5Final(u_int8_t [MD5_DIGEST_LENGTH], SG_MD5_CTX *);
void	 SG_MD5Transform(u_int32_t [4], const u_int8_t [MD5_BLOCK_LENGTH]);

#ifdef __cplusplus
} // of extern C
#endif

#endif /* _MD5_H_ */

     

 