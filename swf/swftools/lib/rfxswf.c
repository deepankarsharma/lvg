/* vi: set sts=2 sw=2 :*/
/* rfxswf.c

   Library for creating and reading SWF files or parts of it.
   There's a module directory which provides some extended functionality.
   Most modules are included at the bottom of this file.

   Part of the swftools package.

   Copyright (c) 2000-2003 Rainer Bohme <rfxswf@reflex-studio.de>
   Copyright (c) 2003 Matthias Kramm <kramm@quiss.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include "rfxswf.h"

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#include "./bitio.h"

// internal constants

#define MALLOC_SIZE     128
#define INSERT_RFX_TAG

#define MEMSIZE(l) (((l/MALLOC_SIZE) + 1)*MALLOC_SIZE)

// inline wrapper functions

TAG * swf_NextTag(TAG * t) { return t->next; }
TAG * swf_PrevTag(TAG * t) { return t->prev; }
U16   swf_GetTagID(TAG * t)    { return t->id; }
U32   swf_GetTagLen(TAG * t) { return t->len; }
U8*   swf_GetTagLenPtr(TAG * t) { return &(t->data[t->len]); }
U32   swf_GetTagPos(TAG * t)   { return t->pos; }

void swf_SetTagPos(TAG * t,U32 pos)
{
    swf_ResetReadBits(t);
    if (pos <= t->len)
        t->pos = pos;
#ifdef _DEBUG
    else
    {
        printf("SetTagPos(%d) out of bounds: TagID = %i\n",pos, t->id); fflush(stdout);
    }
#endif
}

char *swf_GetString(TAG *t)
{
    int pos = t->pos;
    while (t->pos < t->len && swf_GetU8(t));
    /* make sure we always have a trailing zero byte */
    if (t->pos == t->len)
    {
        if (t->len == t->memsize)
        {
            swf_ResetWriteBits(t);
            swf_SetU8(t, 0);
            t->len = t->pos;
        }
        t->data[t->len] = 0;
    }
    return (char*)&(t->data[pos]);
}

U8 swf_GetU8(TAG *t)
{
    swf_ResetReadBits(t);
    if ((int)t->pos >= (int)t->len)
    {
#ifdef _DEBUG
        printf("GetU8() out of bounds: TagID = %i\n", t->id); fflush(stdout);
#endif
        return 0;
    }
    return t->data[t->pos++];
}

U16 swf_GetU16(TAG *t)
{
    U16 res;
    swf_ResetReadBits(t);
    if ((int)t->pos > ((int)t->len - 2))
    {
#ifdef _DEBUG
        printf("GetU16() out of bounds: TagID = %i\n", t->id); fflush(stdout);
#endif
        return 0;
    }
    res = t->data[t->pos] | (t->data[t->pos+1]<<8);
    t->pos+=2;
    return res;
}

U32 swf_GetU32(TAG * t)
{
    U32 res;
    swf_ResetReadBits(t);
    if ((int)t->pos > ((int)t->len - 4))
    {
#ifdef _DEBUG
        printf("GetU32() out of bounds: TagID = %i\n", t->id); fflush(stdout);
#endif
        return 0;
    }
    res = t->data[t->pos]        | (t->data[t->pos+1]<<8) |
            (t->data[t->pos+2]<<16) | (t->data[t->pos+3]<<24);
    t->pos+=4;
    return res;
}

int swf_GetBlock(TAG *t, U8 *b, int l)
// returns number of bytes written (<=l)
// b = NULL -> skip data
{
    swf_ResetReadBits(t);
    if ((t->len - t->pos) < l)
        l = t->len - t->pos;
    if (b && l)
        memcpy(b, &t->data[t->pos], l);
    t->pos += l;
    return l;
}

int swf_SetBlock(TAG * t,const U8 * b,int l)
// Appends Block to the end of Tagdata, returns size
{
    U32 newlen = t->len + l;
    swf_ResetWriteBits(t);
    if (newlen > t->memsize)
    {
        U32 newmem  = MEMSIZE(newlen);
        U8 *newdata = (U8*)(realloc(t->data, newmem));
        t->memsize = newmem;
        t->data    = newdata;
    }
    if (b)
        memcpy(&t->data[t->len], b, l);
    else
        memset(&t->data[t->len], 0x00, l);
    t->len+=l;
    return l;
}

int swf_SetU8(TAG * t,U8 v)
{
    swf_ResetWriteBits(t);
    if ((t->len + 1) > t->memsize)
        return (swf_SetBlock(t, &v, 1) == 1) ? 0 : -1;
    t->data[t->len++] = v;
    return 0;
}

int swf_SetU16(TAG * t,U16 v)
{
    U8 a[2];
    a[0] = v & 0xff;
    a[1] = v >> 8;

    swf_ResetWriteBits(t);
    if ((t->len + 2) > t->memsize)
        return (swf_SetBlock(t, a, 2) == 2) ? 0 : -1;
    t->data[t->len++] = a[0];
    t->data[t->len++] = a[1];
    return 0;
}

int swf_SetU32(TAG *t, U32 v)
{
    U8 a[4];
    a[0] = v & 0xff;        // to ensure correct handling of non-intel byteorder
    a[1] = (v >> 8) & 0xff;
    a[2] = (v >> 16) & 0xff;
    a[3] = (v >> 24) & 0xff;

    swf_ResetWriteBits(t);
    if ((t->len + 4) > t->memsize)
        return (swf_SetBlock(t,a,4)==4)?0:-1;
    t->data[t->len++] = a[0];
    t->data[t->len++] = a[1];
    t->data[t->len++] = a[2];
    t->data[t->len++] = a[3];
    return 0;
}

U32 swf_GetBits(TAG * t, int nbits)
{
    U32 res = 0;
    if (!nbits)
        return 0;
    if (!t->readBit)
        t->readBit = 0x80;
    while (nbits)
    {
        res <<= 1;
        if (t->pos >= t->len)
        {
#ifdef _DEBUG
            printf("GetBits() out of bounds: TagID = %i, pos=%d, len=%d\n", t->id, t->pos, t->len);
            int i, m = t->len > 10 ? 10 : t->len;
            for (i = -1; i < m; i++)
            {
                printf("(%d)%02x ", i, t->data[i]);
            }
            printf("\n"); fflush(stdout);
#endif
            return res;
        }
        if (t->data[t->pos] & t->readBit)
            res |= 1;
        t->readBit>>=1;
        nbits--;
        if (!t->readBit)
        {
            if (nbits)
                t->readBit = 0x80;
            t->pos++;
        }
    }
    return res;
}

S32 swf_GetSBits(TAG * t, int nbits)
{
    U32 res = swf_GetBits(t, nbits);
    if (res & (1 << (nbits - 1)))
        res |= (0xffffffff << nbits);
    return (S32)res;
}

U32 reader_GetBits(reader_t *reader, int nbits)
{
    return reader_readbits(reader, nbits);
}

S32 reader_GetSBits(reader_t *reader, int nbits)
{
    U32 res = reader_readbits(reader, nbits);
    if (res & (1 << (nbits - 1)))
        res |= (0xffffffff << nbits);
    return (S32)res;
}

// Advanced Data Access Functions

double swf_GetFixed(TAG *t)
{
    U16 low =  swf_GetU16(t);
    U16 high = swf_GetU16(t);
    return high + low*(1/65536.0);
}

float swf_GetFixed8(TAG *t)
{
    U8 low =  swf_GetU8(t);
    U8 high = swf_GetU8(t);
    return (float)(high + low*(1/256.0));
}

U32 swf_GetU30(TAG *tag)
{
    U32 shift = 0;
    U32 s = 0;
    int nr=0;
    while(1)
    {
        U8 b = swf_GetU8(tag);
        nr++;
        s |= (b & 127) << shift;
        shift += 7;
        if (!(b & 128) || shift >= 32)
            break;
    }
    return s;
}

U32 swf_GetABCU32(TAG*tag)
{
    return swf_GetU30(tag);
}

S32 swf_GetABCS32(TAG*tag)
{
    return swf_GetABCU32(tag);
}

#if 0

/*The AVM2 spec is just plain wrong, claiming that S32 values are sign
extended. They're not.
This wastes up to 4 bytes for every negative value. */

void swf_SetABCS32(TAG*tag, S32 s)
{
  printf("write S32: %d\n", s);
    S32 neg = s<0?-1:0;
    U8 sign = s<0?0x40:0;
    while(1) {
        U8 val = s&0x7f;
        U8 vsign = s&0x40;
        s>>=7;
        neg>>=7;
        if(s==neg && vsign==sign) {
            /* if the value we now write has the same sign as s
               and all the remaining bits are equal to the sign of s
               too, stop writing */
            swf_SetU8(tag, val);
            printf("put %02x\n", val);
            break;
        } else {
            swf_SetU8(tag, 0x80 | val);
            printf("put %02x\n", 0x80|val);
        }
    };
}
int swf_GetS30(TAG*tag)
{
    U32 shift = 0;
    U32 s = 0;
    int nr=0;
    while(1) {
        U8 b = swf_GetU8(tag);
        nr++;
        nt i,m=t->len>10?10:t->len;
        for(i=0;i<m;i++) {
            printf("%02x ", t->data[i]);
        }
        printf("\n");
        s|=(b&127)<<shift;
        shift+=7;
        if(!(b&128) || shift>=32)
        {
            if(b&64) {
                if(shift<32)
                  s|=0xffffffff<<shift;
            }
            break;
        }
    }
    /* It's not uncommon for other applications (Flex for all negative numbers, and
       Flash for -1) to generate a lot more bytes than would be necessary.
       int nr2= swf_SetS30(0, s);
    if(nr!=nr2) {
      printf("Signed value %d stored in %d bytes, I'd store it in %d bytes\n", s, nr, nr2);
    }*/
    return s;
}
#endif

float swf_GetF16(TAG * t)
{
    U16 f1 = swf_GetU16(t);
    if (!(f1 & 0x3ff))
        return 0.0;

    // IEEE 16 is 1-5-10
    // IEEE 32 is 1-8-23
    /* gcc 4.1.2 seems to require a union here. *(float*)u doesn't work */
    union {
        U32 u;
        float f;
    } f2;

    U16 e = (f1 >> 10) & 0x1f;
    U16 m = f1 & 0x3ff;
    /* find highest bit in mantissa */
    int h=0;
    while (!(m & 0x400))
    {
        m <<= 1;
        h++;
    }
    m&=0x3ff;
    e -= h;
    e += 0x6f;

    f2.u = (f1 & 0x8000) << 16; //sign
    f2.u |= e << 23; //exponent
    f2.u |= m << 13; //mantissa
    return *(float*)&f2;
}

float F16toFloat(U16 x)
{
    TAG t;
    t.data = (void*)&x;
    t.readBit = 0;
    t.pos = 0;
    t.len = 2;
    return swf_GetF16(&t);
}

float swf_GetFloat(TAG *tag)
{
    union {
        U32 uint_bits;
        float float_bits;
    } f;
    f.uint_bits = swf_GetU32(tag);
    return f.float_bits;
}

double swf_GetD64(TAG*tag)
{
    /* FIXME: this is not big-endian compatible */
    double value = *(double*)&tag->data[tag->pos];
    swf_GetU32(tag);
    swf_GetU32(tag);
    return value;
}

int swf_GetU24(TAG*tag)
{
    int b1 = swf_GetU8(tag);
    int b2 = swf_GetU8(tag);
    int b3 = swf_GetU8(tag);
    return b3 << 16 | b2 << 8 | b1;
}

int swf_GetS24(TAG*tag)
{
    int b1 = swf_GetU8(tag);
    int b2 = swf_GetU8(tag);
    int b3 = swf_GetU8(tag);
    if (b3 & 0x80)
        return -1-((b3<<16|b2<<8|b1)^0xffffff);
    else
        return b3<<16|b2<<8|b1;
}

void swf_GetRGB(TAG * t, RGBA * col)
{
    RGBA dummy;
    if(!col)
        col = &dummy;
    col->r = swf_GetU8(t);
    col->g = swf_GetU8(t);
    col->b = swf_GetU8(t);
    col->a = 255;
}

void swf_GetRGBA(TAG * t, RGBA * col)
{
    RGBA dummy;
    if (!col)
        col = &dummy;
    col->r = swf_GetU8(t);
    col->g = swf_GetU8(t);
    col->b = swf_GetU8(t);
    col->a = swf_GetU8(t);
}

void swf_GetGradient(TAG *tag, GRADIENT *gradient, char alpha)
{
    int t;
    if (!tag)
    {
        memset(gradient, 0, sizeof(GRADIENT));
        return;
    }
    U8 num = swf_GetU8(tag) & 15;
    if (gradient)
    {
        gradient->num = num;
        gradient->rgba = (RGBA*)calloc(1, sizeof(RGBA)*gradient->num);
        gradient->ratios = (U8*)calloc(1, sizeof(gradient->ratios[0])*gradient->num);
    }
    for (t = 0; t < num; t++)
    {
        U8 ratio = swf_GetU8(tag);
        RGBA color;
        if (!alpha)
            swf_GetRGB(tag, &color);
        else
            swf_GetRGBA(tag, &color);
        if (gradient)
        {
            gradient->ratios[t] = ratio;
            gradient->rgba[t]   = color;
        }
    }
}

void swf_FreeGradient(GRADIENT* gradient)
{
    if (gradient->ratios)
        free(gradient->ratios);
    if (gradient->rgba)
        free(gradient->rgba);
    memset(gradient, 0, sizeof(GRADIENT));
}

int swf_GetRect(TAG *t, SRECT *r)
{
    int nbits;
    SRECT dummy;
    if (!t)
    {
        r->xmin = r->xmax = r->ymin = r->ymax = 0;
        return 0;
    }
    if (!r)
        r = &dummy;
    nbits = (int) swf_GetBits(t, 5);
    r->xmin = swf_GetSBits(t, nbits);
    r->xmax = swf_GetSBits(t, nbits);
    r->ymin = swf_GetSBits(t, nbits);
    r->ymax = swf_GetSBits(t, nbits);
    return 0;
}

int reader_GetRect(reader_t*reader, SRECT *r)
{
    int nbits;
    SRECT dummy;
    if (!r)
        r = &dummy;
    nbits = (int) reader_GetBits(reader, 5);
    r->xmin = reader_GetSBits(reader, nbits);
    r->xmax = reader_GetSBits(reader, nbits);
    r->ymin = reader_GetSBits(reader, nbits);
    r->ymax = reader_GetSBits(reader, nbits);
    return 0;
}

SRECT swf_ClipRect(SRECT border, SRECT r)
{
    if (r.xmax > border.xmax)
        r.xmax = border.xmax;
    if (r.ymax > border.ymax)
        r.ymax = border.ymax;
    if (r.xmax < border.xmin)
        r.xmax = border.xmin;
    if (r.ymax < border.ymin)
        r.ymax = border.ymin;

    if (r.xmin > border.xmax)
        r.xmin = border.xmax;
    if (r.ymin > border.ymax)
        r.ymin = border.ymax;
    if (r.xmin < border.xmin)
        r.xmin = border.xmin;
    if (r.ymin < border.ymin)
        r.ymin = border.ymin;
    return r;
}

void swf_ExpandRect(SRECT *src, SPOINT add)
{
    if ((src->xmin | src->ymin | src->xmax | src->ymax) == 0)
    {
        src->xmin = add.x;
        src->ymin = add.y;
        src->xmax = add.x;
        src->ymax = add.y;
        if ((add.x | add.y) == 0)
            src->xmax++; //make sure the bbox is not NULL anymore
        return;
    }
    if (add.x < src->xmin)
        src->xmin = add.x;
    if (add.x > src->xmax)
        src->xmax = add.x;
    if (add.y < src->ymin)
        src->ymin = add.y;
    if (add.y > src->ymax)
        src->ymax = add.y;
}
void swf_ExpandRect2(SRECT*src, SRECT*add)
{
    if ((add->xmin | add->ymin | add->xmax | add->ymax) == 0)
        return;
    if ((src->xmin | src->ymin | src->xmax | src->ymax) == 0)
        *src = *add;
    if (add->xmin < src->xmin)
        src->xmin = add->xmin;
    if (add->ymin < src->ymin)
        src->ymin = add->ymin;
    if (add->xmax > src->xmax)
        src->xmax = add->xmax;
    if (add->ymax > src->ymax)
        src->ymax = add->ymax;
}
void swf_ExpandRect3(SRECT*src, SPOINT center, int radius)
{
    if ((src->xmin | src->ymin | src->xmax | src->ymax) == 0)
    {
        src->xmin = center.x-radius;
        src->ymin = center.y-radius;
        src->xmax = center.x+radius;
        src->ymax = center.y+radius;
        if ((center.x | center.y | radius) == 0)
            src->xmax++; //make sure the bbox is not NULL anymore
        return;
    }
    if (center.x - radius < src->xmin)
        src->xmin = center.x - radius;
    if (center.x + radius > src->xmax)
        src->xmax = center.x + radius;
    if (center.y - radius < src->ymin)
        src->ymin = center.y - radius;
    if (center.y + radius > src->ymax)
        src->ymax = center.y + radius;
}

int swf_GetMatrix(TAG *t, MATRIX *m)
{
    MATRIX dummy;
    int nbits;

    if (!m)
        m = &dummy;

    if (!t)
    {
        m->sx = m->sy = 0x10000;
        m->r0 = m->r1 = 0;
        m->tx = m->ty = 0;
        return -1;
    }

    swf_ResetReadBits(t);

    if (swf_GetBits(t, 1))
    {
        nbits = swf_GetBits(t, 5);
        m->sx = swf_GetSBits(t, nbits);
        m->sy = swf_GetSBits(t, nbits);
    } else
        m->sx = m->sy = 0x10000;

    if (swf_GetBits(t, 1))
    {
        nbits = swf_GetBits(t, 5);
        m->r0 = swf_GetSBits(t, nbits);
        m->r1 = swf_GetSBits(t, nbits);
    } else
        m->r0 = m->r1 = 0x0;

    nbits = swf_GetBits(t, 5);
    m->tx = swf_GetSBits(t, nbits);
    m->ty = swf_GetSBits(t, nbits);
    return 0;
}

int swf_GetCXForm(TAG * t,CXFORM * cx,U8 alpha)
{
    CXFORM cxf;
    int hasadd, hasmul, nbits;

    if (!cx)
        cx = &cxf;

    cx->a0 = cx->r0 = cx->g0 = cx->b0 = 256;
    cx->a1 = cx->r1 = cx->g1 = cx->b1 = 0;

    if (!t)
        return 0;

    swf_ResetReadBits(t);
    hasadd = swf_GetBits(t, 1);
    hasmul = swf_GetBits(t, 1);
    nbits  = swf_GetBits(t, 4);

    if (hasmul)
    {
        cx->r0 = (S16)swf_GetSBits(t, nbits);
        cx->g0 = (S16)swf_GetSBits(t, nbits);
        cx->b0 = (S16)swf_GetSBits(t, nbits);
        if (alpha)
            cx->a0 = (S16)swf_GetSBits(t, nbits);
    }
    if (hasadd)
    {
        cx->r1 = (S16)swf_GetSBits(t, nbits);
        cx->g1 = (S16)swf_GetSBits(t, nbits);
        cx->b1 = (S16)swf_GetSBits(t, nbits);
        if (alpha)
            cx->a1 = (S16)swf_GetSBits(t, nbits);
    }
    return 0;
}

// Tag List Manipulating Functions

TAG *swf_InsertTag(TAG *after, U16 id)
{
    TAG *t = (TAG *)calloc(1, sizeof(TAG));
    t->id = id;

    if (after)
    {
        t->prev  = after;
        t->next  = after->next;
        after->next = t;
        if (t->next)
            t->next->prev = t;
    }
    return t;
}

TAG *swf_InsertTagBefore(SWF *swf, TAG *before, U16 id)
{
    TAG *t = (TAG *)calloc(1, sizeof(TAG));
    t->id = id;

    if (before)
    {
        t->next  = before;
        t->prev  = before->prev;
        before->prev = t;
        if (t->prev) t->prev->next = t;
    }
    if (swf && swf->firstTag == before)
    {
        swf->firstTag = t;
    }
    return t;
}

void swf_ClearTag(TAG * t)
{
    if (t->data)
        free(t->data);
    t->data = 0;
    t->pos = 0;
    t->len = 0;
    t->readBit = 0;
    t->writeBit = 0;
    t->memsize = 0;
}

void swf_ResetTag(TAG *tag, U16 id)
{
    tag->len = tag->pos = tag->readBit = tag->writeBit = 0;
    tag->id = id;
}

TAG *swf_CopyTag(TAG *tag, TAG *to_copy)
{
    tag = swf_InsertTag(tag, to_copy->id);
    swf_SetBlock(tag, to_copy->data, to_copy->len);
    return tag;
}

TAG *swf_DeleteTag(SWF *swf, TAG *t)
{
    TAG *next = t->next;

    if (swf && swf->firstTag==t)
        swf->firstTag = t->next;
    if (t->prev)
        t->prev->next = t->next;
    if (t->next)
        t->next->prev = t->prev;

    if (t->data)
        free(t->data);
    free(t);
    return next;
}

TAG *swf_ReadTag(reader_t *reader, TAG *prev)
{
    TAG * t;
    U16 raw;
    U32 len;
    int id;

    if (reader->read(reader, &raw, 2) != 2)
        return NULL;
    raw = LE_16_TO_NATIVE(raw);

    len = raw & 0x3f;
    id  = raw >> 6;

    if (len == 0x3f)
    {
        len = reader_readU32(reader);
    }

    if (id == ST_DEFINESPRITE)
        len = 2*sizeof(U16); // Sprite handling fix: Flatten sprite tree

    t = (TAG *)calloc(1, sizeof(TAG));

    t->len = len;
    t->id  = id;

    if (t->len)
    {
        t->data = (U8*)malloc(t->len + 4); // pad for mp3 bitbuf
        t->memsize = t->len;
        if (reader->read(reader, t->data, t->len) != t->len)
        {
#ifdef _DEBUG
            printf("rfxswf: Warning: Short read (tagid %d). File truncated?\n", t->id); fflush(stdout);
#endif
            free(t->data);
            free(t);
            return NULL;
        }
    }

    if (prev)
    {
        t->prev  = prev;
        prev->next = t;
    }
    return t;
}

void swf_UnFoldSprite(TAG * t)
{
    U16 id,tmp;
    U32 len;
    TAG*next = t;
    U16 spriteid,spriteframes;
    int level;
    if (t->id != ST_DEFINESPRITE)
        return;
    if (t->len <= 4) // not folded
        return;

    swf_SetTagPos(t, 0);

    spriteid = swf_GetU16(t); //id
    spriteframes = swf_GetU16(t); //frames

    level = 1;

    while (1)
    {
        TAG*it = 0;
        tmp = swf_GetU16(t);
        len = tmp&0x3f;
        id  = tmp>>6;
        if (id == ST_END)
            level--;
        if (id == ST_DEFINESPRITE && len <= 4)
            level++;

        if (len == 0x3f)
            len = swf_GetU32(t);
        it = swf_InsertTag(next, id);
        next = it;
        it->len = len;
        it->id  = id;
        if (it->len)
        {
            it->data = (U8*)malloc(it->len);
            it->memsize = it->len;
            swf_GetBlock(t, it->data, it->len);
        }

        if (!level)
            break;
    }

    free(t->data); t->data = 0;
    t->memsize = t->len = t->pos = 0;

    swf_SetU16(t, spriteid);
    swf_SetU16(t, spriteframes);
}

void swf_FoldSprite(TAG * t)
{
    TAG *sprtag = t, *tmp;
    U16 id, frames;
    int level;
    if (t->id != ST_DEFINESPRITE)
        return;
    if (!t->len)
    {
#ifdef _DEBUG
        printf("error: Sprite has no ID!"); fflush(stdout);
#endif
        return;
    }
    if (t->len > 4)
    {
        /* sprite is already folded */
        return;
    }

    t->pos = 0;
    id = swf_GetU16(t);
    free(t->data);
    t->len = t->pos = t->memsize = 0;
    t->data = 0;

    frames = 0;

    t = swf_NextTag(sprtag);
    level = 1;

    do
    {
        if (t->id == ST_SHOWFRAME)
            frames++;
        if (t->id == ST_DEFINESPRITE && t->len <= 4)
            level++;
        if (t->id == ST_END)
            level--;
        t = swf_NextTag(t);
    } while(t && level);
#ifdef _DEBUG
    if (level)
    {
        printf("rfxswf error: sprite doesn't end(1)\n"); fflush(stdout);
    }
#endif

    swf_SetU16(sprtag, id);
    swf_SetU16(sprtag, frames);

    t = swf_NextTag(sprtag);
    level = 1;

    do
    {
        if (t->len < 0x3f &&
           (t->id !=ST_DEFINEBITSLOSSLESS && t->id != ST_DEFINEBITSLOSSLESS2 && t->id != ST_SOUNDSTREAMBLOCK &&
            t->id !=ST_DEFINEBITSJPEG && t->id != ST_DEFINEBITSJPEG2 && t->id != ST_DEFINEBITSJPEG3)
           )
        {
            swf_SetU16(sprtag, t->len|(t->id << 6));
        } else
        {
            swf_SetU16(sprtag, 0x3f|(t->id << 6));
            swf_SetU32(sprtag, t->len);
        }
        if (t->len)
            swf_SetBlock(sprtag, t->data, t->len);
        tmp = t;
        if (t->id == ST_DEFINESPRITE && t->len <= 4)
            level++;
        if (t->id == ST_END)
            level--;
        t = swf_NextTag(t);
        swf_DeleteTag(0, tmp);
    } while (t && level);
#ifdef _DEBUG
    if (level)
    {
        printf("rfxswf error: sprite doesn't end(2)\n"); fflush(stdout);
    }
#endif
}

int swf_IsFolded(TAG * t)
{
    return (t->id == ST_DEFINESPRITE && t->len > 4);
}

void swf_FoldAll(SWF*swf)
{
    TAG *tag = swf->firstTag;
    while (tag)
    {
        if (tag->id == ST_DEFINESPRITE)
        {
            swf_FoldSprite(tag);
        }
        tag = swf_NextTag(tag);
    }
}

void swf_UnFoldAll(SWF*swf)
{
    TAG *tag = swf->firstTag;
    while (tag)
    {
        if (tag->id == ST_DEFINESPRITE)
            swf_UnFoldSprite(tag);
        tag = tag->next;
    }
}

int swf_ReadSWF2(reader_t*reader, SWF *swf)   // Reads SWF to memory (malloc'ed), returns length or <0 if fails
{
    if (!swf)
        return -1;
    memset(swf, 0x00, sizeof(SWF));

    char b[32];                         // read Header
    int len;
    TAG * t;
    TAG t1;
    if ((len = reader->read(reader, b, 8)) < 8)
        return -1;

    if (b[0]!='F' && b[0]!='C')
        return -1;
    if (b[1]!='W')
        return -1;
    if (b[2]!='S')
        return -1;
    swf->fileVersion = b[3];
    swf->compressed  = (b[0]=='C')?1:0;
    swf->fileSize    = GET32(&b[4]);

    if (swf->compressed)
    {
        return -1;
    }
    swf->compressed = 0; // derive from version number from now on

    reader_GetRect(reader, &swf->movieSize);
    reader->read(reader, &swf->frameRate, 2);
    swf->frameRate = LE_16_TO_NATIVE(swf->frameRate);
    reader->read(reader, &swf->frameCount, 2);
    swf->frameCount = LE_16_TO_NATIVE(swf->frameCount);

    /* read tags and connect to list */
    t1.next = 0;
    t = &t1;
    while (t)
    {
        t = swf_ReadTag(reader,t);
        if (t && t->id == ST_FILEATTRIBUTES)
        {
            swf->fileAttributes = swf_GetU32(t);
            swf_ResetReadBits(t);
        }
    }
    swf->firstTag = t1.next;
    if (t1.next)
        t1.next->prev = NULL;

    return reader->pos;
}

void swf_FreeTags(SWF *swf)                 // Frees all malloc'ed memory for tags
{
    TAG * t = swf->firstTag;
    while (t)
    {
        TAG *tnew = t->next;
        if (t->data)
            free(t->data);
        free(t);
        t = tnew;
    }
    swf->firstTag = 0;
}
