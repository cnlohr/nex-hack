/* 2007-07-26: File added by Sony Corporation */
/* With non GPL files, use following license */
/*
 * Copyright 2006,2007 Sony Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Otherwise with GPL files, use following license */
/*
 *  Copyright 2006,2007 Sony Corporation.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  version 2 of the  License.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

//#include <linux/kernel.h>
//#include <linux/module.h>
//#include <linux/init.h>
//#include <linux/vmalloc.h>
//#include <linux/lz77.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define u8 uint8_t
#include "lz77.h"

static inline void lz77_put_union(unsigned char **d, struct lz77_union *src)
{
	unsigned char *dst = *d;
	int i;

	*dst++ = src->type;
	for(i=0; i<src->index; i++){
		if(src->l_cw[i].type==0)//literal
			*dst++ = src->l_cw[i].lit[0];
		else{
			*dst++ = src->l_cw[i].lit[0];
			*dst++ = src->l_cw[i].lit[1];
		}
	}

	*d = dst;
	memset(src, 0, sizeof(*src));
}

static inline void lz77_flush(struct lz77_union *u_buff, unsigned char **dst)
{
	unsigned char index = u_buff->index;

	if(index<8){
		u_buff->type |= (1<<index);
		u_buff->l_cw[index].type = 0x01;
		u_buff->l_cw[index].lit[0] = 0x00;
		u_buff->l_cw[index].lit[1] = 0x00;
		u_buff->index++;
	}

	lz77_put_union(dst, u_buff);

	if(index==8){
		unsigned char *d = *dst;
		*d++ = 0xFF;
		*d++ = 0x00;
		*d++ = 0x00;
		*dst = d;
	}

}

static inline void lz77_put_lit(struct lz77_union *u_buff, unsigned char lit, unsigned char **dst)
{
	int i=u_buff->index++;

	u_buff->type &= ~(1<<i);
	u_buff->l_cw[i].type = 0;
	u_buff->l_cw[i].lit[0] = lit;
}

static inline int lz77_put_codeword(struct lz77_union *u_buff, int addr, int size, unsigned char **dst)
{
	int i=u_buff->index++;

	size = size_to_index(size);
	u_buff->type |= (1<<i);

	u_buff->l_cw[i].type = 1;
	u_buff->l_cw[i].lit[0] = ((size&0x0F)<<4) | ((addr&0xF00)>>8);
	u_buff->l_cw[i].lit[1] = addr&0xFF;

	return index_to_size(size);
}

static inline int lz77_search_match(unsigned char *s1, unsigned char *s2, unsigned char *s_e)
{
	int len=0;
	while(s1<s_e){
		if(*s1++ == *s2++)
			len++;
		else
			break;
	}
	return len;
}

static inline void lz77_init_index(uint8_t *src, int len, void *workspace)
{
	short *hash = WORKSPACE_HASH(workspace);
	short *index = WORKSPACE_INDEX(workspace);
	int i;

	memset(hash, 0xFF, HASH_SIZE);
	for(i=0;i<=(len-3);i++){
		short *h = hash+HASH_VALUE(src[i],src[i+1], src[i+2]);

		index[i] = *h;
		*h = i;
	}
}

static inline void lz77_exit_index(void *workspace)
{
	return ;
}

static inline int lz77_search_index(uint8_t *ss, uint8_t *st, uint8_t *se, int *ml, void *workspace)
{
	int max_len = MIN_LEN-1;//at least 3 byte matching for not to lose
	int match_s = -1;
	short *index = WORKSPACE_INDEX(workspace);
	int off = st-ss;
	short *h = index+off;

	while(*h>=0 && *h>(off-WIN_LEN)){
		int m_l;
		m_l = lz77_search_match(st, ss+*h, se);
		if(m_l > max_len){
			max_len = m_l;
			match_s = *h;
			if(max_len>=MAX_LEN){
				break;
			}
		}

		h = index+*h;
	}

	if(match_s<0)
		return -1;

	*ml = max_len;
	return (off-match_s);
}

/*
  type_bit * 8
  lit/codeword *8
      lit: literal ==> 1 byte
      codeword: size_bit*4 + addr_bit*12 ==> 2 byte
 ...
 */
int lz77_deflate(unsigned char *src, int len, unsigned char *dst, int dst_len, void *workspace, int wl)
{
	struct lz77_union u_buff;
	int max_len, match_s;
	unsigned char *dst_org = dst;
	unsigned char *s = src;
	unsigned char *se = src+len;
	int i = 0;

	if(!src || !dst || dst_len<4 ||	wl<LZ77_WORKSPACE_SIZE(len)){
		return -1;
	}

	lz77_init_index(src, len, workspace);

	memset(&u_buff, 0, sizeof(u_buff));
	*dst++ = LZ77_COMPRESSED;

	while(s<se){
		while(i<8 && s<se){
			match_s = lz77_search_index(src, s, src+len, &max_len, workspace);

			if(match_s < 0){
				max_len = 1;
				lz77_put_lit(&u_buff, *s, &dst);
			}
			else{
				max_len = lz77_put_codeword(&u_buff, match_s, max_len, &dst);
			}

			s += max_len;
			i++;
		}

		if(i==8){
			lz77_put_union(&dst, &u_buff);
			i = 0;
		}
	}

	lz77_flush(&u_buff, &dst);

	if((dst-dst_org)>len){//uncompressable
		int l=dst_len-4;

		lz77_printf("uncompressble -> fall back to raw data!\n");

		l=l>len?len:l;
		dst = dst_org;

		*dst++ = LZ77_RAW;
		*dst++ = 0;
		*dst++ = l&0xFF;
		*dst++ = l>>8;

		memcpy(dst, src, l);
		dst += l;
	}

	lz77_printf("delfated: %d %d->%d\n", dst_len, len, dst-dst_org);

	lz77_exit_index(workspace);
	return (dst-dst_org);
}

//EXPORT_SYMBOL(lz77_deflate);
//MODULE_LICENSE("Dual BSD/GPL");
