/*  bitio.c
    part of swftools
    implementation of bitio.h.

    Copyright (C) 2003 Matthias Kramm <kramm@quiss.org>

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>
//#define __USE_LARGEFILE64
#include <fcntl.h>
#include <errno.h>

#include "../config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_ZLIB
#include <zlib.h>
#define ZLIB_BUFFER_SIZE 16384
#endif
#include "./bitio.h"

/* ---------------------------- null reader ------------------------------- */

static int reader_nullread(reader_t*r, void* data, int len)
{
    memset(data, 0, len);
    return len;
}
static void reader_nullread_dealloc(reader_t*r)
{
    memset(r, 0, sizeof(reader_t));
}
static int reader_nullseek(reader_t*r, int pos)
{
    return pos;
}
void reader_init_nullreader(reader_t*r)
{
    r->read = reader_nullread;
    r->seek = reader_nullseek;
    r->dealloc = reader_nullread_dealloc;
    r->internal = 0;
    r->type = READER_TYPE_NULL;
    r->mybyte = 0;
    r->bitpos = 8;
    r->pos = 0;
}
/* ---------------------------- file reader ------------------------------- */

static int reader_fileread(reader_t*reader, void* data, int len)
{
    int ret = read((int)(intptr_t)reader->internal, data, len);
    if(ret>=0)
        reader->pos += ret;
    return ret;
}
static void reader_fileread_dealloc(reader_t*r)
{
    if(r->type == READER_TYPE_FILE2) {
        close((ptroff_t)r->internal);
    }
    memset(r, 0, sizeof(reader_t));
}
static int reader_fileread_seek(reader_t*r, int pos)
{
    return lseek((ptroff_t)r->internal, pos, SEEK_SET);
}
void reader_init_filereader(reader_t*r, int handle)
{
    r->read = reader_fileread;
    r->seek = reader_fileread_seek;
    r->dealloc = reader_fileread_dealloc;
    r->internal = (void*)(intptr_t)handle;
    r->type = READER_TYPE_FILE;
    r->mybyte = 0;
    r->bitpos = 8;
    r->pos = 0;
}
int reader_init_filereader2(reader_t*r, const char*filename)
{
#ifdef HAVE_OPEN64
    int fi = open64
#else
    int fi = open
#endif
            (filename,
#ifdef O_BINARY
             O_BINARY|
#endif
             O_RDONLY);
    reader_init_filereader(r, fi);
    r->type = READER_TYPE_FILE2;
    return fi;
}

/* ---------------------------- mem reader ------------------------------- */

typedef struct _memread
{
    unsigned char*data;
    int length;
} memread_t;

static int reader_memread(reader_t*reader, void* data, int len)
{
    memread_t*mr = (memread_t*)reader->internal;

    if(mr->length - reader->pos < len) {
        len = mr->length - reader->pos;
    }
    if(!len) return 0;
    memcpy(data, &mr->data[reader->pos], len);
    reader->pos += len;
    return len;
}
static int reader_memseek(reader_t*reader, int pos)
{
    memread_t*mr = (memread_t*)reader->internal;
    if(pos>=0 && pos<=mr->length) {
        reader->pos = pos;
        return pos;
    } else {
        return -1;
    }
}
static void reader_memread_dealloc(reader_t*reader)
{
    if(reader->internal)
        free(reader->internal);
    memset(reader, 0, sizeof(reader_t));
}
void reader_init_memreader(reader_t*r, void*newdata, int newlength)
{
    memread_t*mr = (memread_t*)malloc(sizeof(memread_t));
    mr->data = (unsigned char*)newdata;
    mr->length = newlength;
    r->read = reader_memread;
    r->seek = reader_memseek;
    r->dealloc = reader_memread_dealloc;
    r->internal = (void*)mr;
    r->type = READER_TYPE_MEM;
    r->mybyte = 0;
    r->bitpos = 8;
    r->pos = 0;
}

/* ---------------------------- zzip reader ------------------------------ */
#ifdef HAVE_ZZIP
static int reader_zzip_read(reader_t*reader, void* data, int len)
{
    return zzip_file_read((ZZIP_FILE*)reader->internal, data, len);
}
static void reader_zzip_dealloc(reader_t*reader)
{
    memset(reader, 0, sizeof(reader_t));
}
static int reader_zzip_seek(reader_t*reader, int pos)
{
    return zzip_seek((ZZIP_FILE*)reader->internal, pos, SEEK_SET);
}
void reader_init_zzipreader(reader_t*r,ZZIP_FILE*z)
{
    r->read = reader_zzip_read;
    r->seek = reader_zzip_seek;
    r->dealloc = reader_zzip_dealloc;
    r->internal = z;
    r->type = READER_TYPE_ZZIP;
    r->mybyte = 0;
    r->bitpos = 8;
    r->pos = 0;
}
#endif

/* ---------------------------- mem writer ------------------------------- */

typedef struct _memwrite
{
    unsigned char*data;
    int length;
} memwrite_t;

static int writer_memwrite_write(writer_t*w, void* data, int len)
{
    memwrite_t*mw = (memwrite_t*)w->internal;
    if(mw->length - w->pos > len) {
        memcpy(&mw->data[w->pos], data, len);
        w->pos += len;
        return len;
    } else {
        memcpy(&mw->data[w->pos], data, mw->length - w->pos);
        w->pos = mw->length;
        return mw->length - w->pos;
    }
}
static void writer_memwrite_finish(writer_t*w)
{
    if(w->internal)
        free(w->internal);
    w->internal = 0;
}
static void dummy_flush(writer_t*w)
{
}
void writer_init_memwriter(writer_t*w, void*data, int len)
{
    memwrite_t *mr;
    mr = (memwrite_t*)malloc(sizeof(memwrite_t));
    mr->data = (unsigned char *)data;
    mr->length = len;
    memset(w, 0, sizeof(writer_t));
    w->write = writer_memwrite_write;
    w->flush = dummy_flush;
    w->finish = writer_memwrite_finish;
    w->internal = (void*)mr;
    w->type = WRITER_TYPE_MEM;
    w->bitpos = 0;
    w->mybyte = 0;
    w->pos = 0;
}

/* ------------------------- growing mem writer ------------------------------- */

typedef struct _growmemwrite
{
    unsigned char*data;
    int length;
    U32 grow;
} growmemwrite_t;
static int writer_growmemwrite_write(writer_t*w, void* data, int len)
{
    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    if(!mw->data) {
        fprintf(stderr, "Illegal write operation: data already given away");
        exit(1);
    }
    if(mw->length - w->pos < len) {
        int newlength = mw->length;
        while(newlength - w->pos < len) {
            newlength += mw->grow;
        }
#ifdef NO_REALLOC
        unsigned char*newmem = (unsigned char*)malloc(newlength);
        memcpy(newmem, mw->data, mw->length);
        free(mw->data);
        mw->data = newmem;
#else
        mw->data = (unsigned char*)realloc(mw->data, newlength);
#endif
        mw->length = newlength;
    }
    memcpy(&mw->data[w->pos], data, len);
    w->pos += len;
    return len;
}
static void writer_growmemwrite_finish(writer_t*w)
{
    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    if(mw->data) {
        free(mw->data);mw->data = 0;
    }
    mw->length = 0;
    free(w->internal);mw=0;
    memset(w, 0, sizeof(writer_t));
}
void* writer_growmemwrite_memptr(writer_t*w, int*len)
{
    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    if(len) {
        *len = w->pos;
    }
    return mw->data;
}
void* writer_growmemwrite_getmem(writer_t*w)
{
    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    void*ret = mw->data;
    /* remove own reference so that neither write() nor finish() can free it.
       It's property of the caller now.
    */
    mw->data = 0;
    return ret;
}
void writer_growmemwrite_reset(writer_t*w)
{
    //    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    w->pos = 0;
    w->bitpos = 0;
    w->mybyte = 0;
}
void writer_init_growingmemwriter(writer_t*w, U32 grow)
{
    growmemwrite_t *mw = (growmemwrite_t *)malloc(sizeof(growmemwrite_t));
    mw->length = 4096;
    mw->data = (unsigned char *)malloc(mw->length);
    mw->grow = grow;
    memset(w, 0, sizeof(writer_t));
    w->write = writer_growmemwrite_write;
    w->flush = dummy_flush;
    w->finish = writer_growmemwrite_finish;
    w->internal = (void*)mw;
    w->type = WRITER_TYPE_GROWING_MEM;
    w->bitpos = 0;
    w->mybyte = 0;
    w->pos = 0;
}

/* ---------------------------- file writer ------------------------------- */

typedef struct _filewrite
{
    int handle;
    char free_handle;
} filewrite_t;

static int writer_filewrite_write(writer_t*w, void* data, int len)
{
    filewrite_t * fw= (filewrite_t*)w->internal;
    w->pos += len;
    int l = write(fw->handle, data, len);
    if(l < len)
        fprintf(stderr, "Error writing to file: %d/%d", l, len);
    return l;
}
static void writer_filewrite_finish(writer_t*w)
{
    filewrite_t *mr = (filewrite_t*)w->internal;
    if(mr->free_handle)
        close(mr->handle);
    free(w->internal);
    memset(w, 0, sizeof(writer_t));
}
void writer_init_filewriter(writer_t*w, int handle)
{
    filewrite_t *mr = (filewrite_t *)malloc(sizeof(filewrite_t));
    mr->handle = handle;
    mr->free_handle = 0;
    memset(w, 0, sizeof(writer_t));
    w->write = writer_filewrite_write;
    w->finish = writer_filewrite_finish;
    w->internal = mr;
    w->type = WRITER_TYPE_FILE;
    w->bitpos = 0;
    w->mybyte = 0;
    w->pos = 0;
}
void writer_init_filewriter2(writer_t*w, char*filename)
{
#ifdef HAVE_OPEN64
    int fi = open64
#else
    int fi = open
#endif
            (filename,
#ifdef O_BINARY
             O_BINARY|
#endif
             O_WRONLY|O_CREAT|O_TRUNC, 0644);
    writer_init_filewriter(w, fi);
    ((filewrite_t*)w->internal)->free_handle = 1;
}

/* ---------------------------- null writer ------------------------------- */

static int writer_nullwrite_write(writer_t*w, void* data, int len)
{
    w->pos += len;
    return len;
}
static void writer_nullwrite_finish(writer_t*w)
{
    memset(w, 0, sizeof(writer_t));
}
void writer_init_nullwriter(writer_t*w)
{
    memset(w, 0, sizeof(writer_t));
    w->write = writer_nullwrite_write;
    w->flush = dummy_flush;
    w->finish = writer_nullwrite_finish;
    w->internal = 0;
    w->type = WRITER_TYPE_NULL;
    w->bitpos = 0;
    w->mybyte = 0;
    w->pos = 0;
}
/* ---------------------------- zlibinflate reader -------------------------- */

typedef struct _zlibinflate
{
#ifdef HAVE_ZLIB
    z_stream zs;
    reader_t*input;
    unsigned char readbuffer[ZLIB_BUFFER_SIZE];
#endif
} zlibinflate_t;

#ifdef HAVE_ZLIB
static void zlib_error(int ret, char* msg, z_stream*zs)
{
    fprintf(stderr, "%s: zlib error (%d): last zlib error: %s\n",
            msg,
            ret,
            zs->msg?zs->msg:"unknown");
    if(errno) perror("errno:");
    exit(1);
}

static int reader_zlibinflate(reader_t*reader, void* data, int len)
{
    zlibinflate_t*z = (zlibinflate_t*)reader->internal;
    int ret;
    if(!z) {
        return 0;
    }
    if(!len)
        return 0;

    z->zs.next_out = (Bytef *)data;
    z->zs.avail_out = len;

    while(1) {
        if(!z->zs.avail_in) {
            z->zs.avail_in = z->input->read(z->input, z->readbuffer, ZLIB_BUFFER_SIZE);
            z->zs.next_in = z->readbuffer;
        }
        if(z->zs.avail_in)
            ret = inflate(&z->zs, Z_NO_FLUSH);
        else
            ret = inflate(&z->zs, Z_FINISH);

        if (ret != Z_OK &&
                ret != Z_STREAM_END) zlib_error(ret, "bitio:inflate_inflate", &z->zs);

        if (ret == Z_STREAM_END) {
            int pos = z->zs.next_out - (Bytef*)data;
            ret = inflateEnd(&z->zs);
            if (ret != Z_OK) zlib_error(ret, "bitio:inflate_end", &z->zs);
            free(reader->internal);
            reader->internal = 0;
            reader->pos += pos;
            return pos;
        }
        if(!z->zs.avail_out) {
            break;
        }
    }
    reader->pos += len;
    return len;
}

static int reader_zlibseek(reader_t*reader, int pos)
{
    fprintf(stderr, "Erro: seeking not supported for zlib streams");
    return -1;
}

static void reader_zlibinflate_dealloc(reader_t*reader)
{
    zlibinflate_t*z = (zlibinflate_t*)reader->internal;
    /* test whether read() already did basic deallocation */
    if(reader->internal) {
        inflateEnd(&z->zs);
        free(reader->internal);
    }
    memset(reader, 0, sizeof(reader_t));
}

void reader_init_zlibinflate(reader_t*r, reader_t*input)
{
    zlibinflate_t*z = (zlibinflate_t*)malloc(sizeof(zlibinflate_t));
    memset(z, 0, sizeof(zlibinflate_t));
    int ret;
    memset(r, 0, sizeof(reader_t));
    r->internal = z;
    r->read = reader_zlibinflate;
    r->seek = reader_zlibseek;
    r->dealloc = reader_zlibinflate_dealloc;
    r->type = READER_TYPE_ZLIB;
    r->pos = 0;
    z->input = input;
    memset(&z->zs,0,sizeof(z_stream));
    z->zs.zalloc = Z_NULL;
    z->zs.zfree  = Z_NULL;
    z->zs.opaque = Z_NULL;
    ret = inflateInit(&z->zs);
    if (ret != Z_OK) zlib_error(ret, "bitio:inflate_init", &z->zs);
    reader_resetbits(r);
}
#endif

/* ---------------------------- zlibdeflate writer -------------------------- */

typedef struct _zlibdeflate
{
#ifdef HAVE_ZLIB
    z_stream zs;
    writer_t*output;
    unsigned char writebuffer[ZLIB_BUFFER_SIZE];
#endif
} zlibdeflate_t;

#ifdef HAVE_ZLIB
static int writer_zlibdeflate_write(writer_t*writer, void* data, int len)
{
    zlibdeflate_t*z = (zlibdeflate_t*)writer->internal;
    int ret;
    if(writer->type != WRITER_TYPE_ZLIB) {
        fprintf(stderr, "Wrong writer ID (writer not initialized?)\n");
        return 0;
    }
    if(!z) {
        fprintf(stderr, "zlib not initialized!\n");
        return 0;
    }
    if(!len)
        return 0;

    z->zs.next_in = (Bytef *)data;
    z->zs.avail_in = len;

    while(1) {
        ret = deflate(&z->zs, Z_NO_FLUSH);

        if (ret != Z_OK) zlib_error(ret, "bitio:deflate_deflate", &z->zs);

        if(z->zs.next_out != z->writebuffer) {
            writer->pos += z->zs.next_out - (Bytef*)z->writebuffer;
            z->output->write(z->output, z->writebuffer, z->zs.next_out - (Bytef*)z->writebuffer);
            z->zs.next_out = z->writebuffer;
            z->zs.avail_out = ZLIB_BUFFER_SIZE;
        }

        if(!z->zs.avail_in) {
            break;
        }
    }
    return len;
}

void writer_zlibdeflate_flush(writer_t*writer)
{
    zlibdeflate_t*z = (zlibdeflate_t*)writer->internal;
    int ret;
    if(writer->type != WRITER_TYPE_ZLIB) {
        fprintf(stderr, "Wrong writer ID (writer not initialized?)\n");
        return;
    }
    if(!z) {
        fprintf(stderr, "zlib not initialized!\n");
        return;
    }

    z->zs.next_in = 0;
    z->zs.avail_in = 0;
    while(1) {
        ret = deflate(&z->zs, Z_SYNC_FLUSH);
        if (ret != Z_OK) zlib_error(ret, "bitio:deflate_flush", &z->zs);
        if(z->zs.next_out != z->writebuffer) {
            writer->pos += z->zs.next_out - (Bytef*)z->writebuffer;
            z->output->write(z->output, z->writebuffer, z->zs.next_out - (Bytef*)z->writebuffer);
            z->zs.next_out = z->writebuffer;
            z->zs.avail_out = ZLIB_BUFFER_SIZE;
        }
        /* TODO: how will zlib let us know it needs more buffer space? */
        break;
    }
    return;
}

static void writer_zlibdeflate_finish(writer_t*writer)
{
    zlibdeflate_t*z = (zlibdeflate_t*)writer->internal;
    writer_t*output;
    int ret;
    if(writer->type != WRITER_TYPE_ZLIB) {
        fprintf(stderr, "Wrong writer ID (writer not initialized?)\n");
        return;
    }
    if(!z)
        return;
    output= z->output;
    while(1) {
        ret = deflate(&z->zs, Z_FINISH);
        if (ret != Z_OK &&
                ret != Z_STREAM_END) zlib_error(ret, "bitio:deflate_finish", &z->zs);

        if(z->zs.next_out != z->writebuffer) {
            writer->pos += z->zs.next_out - (Bytef*)z->writebuffer;
            z->output->write(z->output, z->writebuffer, z->zs.next_out - (Bytef*)z->writebuffer);
            z->zs.next_out = z->writebuffer;
            z->zs.avail_out = ZLIB_BUFFER_SIZE;
        }

        if (ret == Z_STREAM_END) {
            break;

        }
    }
    ret = deflateEnd(&z->zs);
    if (ret != Z_OK) zlib_error(ret, "bitio:deflate_end", &z->zs);
    free(writer->internal);
    memset(writer, 0, sizeof(writer_t));
    //output->finish(output);
}

void writer_init_zlibdeflate(writer_t*w, writer_t*output)
{
    zlibdeflate_t*z;
    int ret;
    memset(w, 0, sizeof(writer_t));
    z = (zlibdeflate_t*)malloc(sizeof(zlibdeflate_t));
    memset(z, 0, sizeof(zlibdeflate_t));
    w->internal = z;
    w->write = writer_zlibdeflate_write;
    w->flush = writer_zlibdeflate_flush;
    w->finish = writer_zlibdeflate_finish;
    w->type = WRITER_TYPE_ZLIB;
    w->pos = 0;
    z->output = output;
    memset(&z->zs,0,sizeof(z_stream));
    z->zs.zalloc = Z_NULL;
    z->zs.zfree  = Z_NULL;
    z->zs.opaque = Z_NULL;
    ret = deflateInit(&z->zs, 9);
    if (ret != Z_OK) zlib_error(ret, "bitio:deflate_init", &z->zs);
    w->bitpos = 0;
    w->mybyte = 0;
    z->zs.next_out = z->writebuffer;
    z->zs.avail_out = ZLIB_BUFFER_SIZE;
}
#endif

/* ----------------------- bit handling routines -------------------------- */

void writer_writebit(writer_t*w, int bit)
{
    if(w->bitpos==8)
    {
        w->write(w, &w->mybyte, 1);
        w->bitpos = 0;
        w->mybyte = 0;
    }
    if(bit&1)
        w->mybyte |= 1 << (7 - w->bitpos);
    w->bitpos ++;
}
void writer_writebits(writer_t*w, unsigned int data, int bits)
{
    int t;
    for(t=0;t<bits;t++)
    {
        writer_writebit(w, (data >> (bits-t-1))&1);
    }
}
void writer_resetbits(writer_t*w)
{
    if(w->bitpos)
        w->write(w, &w->mybyte, 1);
    w->bitpos = 0;
    w->mybyte = 0;
}

unsigned int reader_readbit(reader_t*r)
{
    if(r->bitpos==8)
    {
        r->bitpos=0;
        r->read(r, &r->mybyte, 1);
    }
    return (r->mybyte>>(7-r->bitpos++))&1;
}
unsigned int reader_readbits(reader_t*r, int num)
{
    int t;
    int val = 0;
    for(t=0;t<num;t++)
    {
        val<<=1;
        val|=reader_readbit(r);
    }
    return val;
}
void reader_resetbits(reader_t*r)
{
    r->mybyte = 0;
    r->bitpos = 8;

}

U8 reader_readU8(reader_t*r)
{
    U8 b = 0;
    if(r->read(r, &b, 1)<1) {
        fprintf(stderr, "bitio.c:reader_readU8: Read over end of memory region\n");
    }
    return b;
}
S8 reader_readS8(reader_t*r)
{
    S8 b = 0;
    if(r->read(r, &b, 1)<1) {
        fprintf(stderr, "bitio.c:reader_readU8: Read over end of memory region\n");
    }
    return b;
}
U16 reader_readU16(reader_t*r)
{
    U8 b1=0,b2=0;
    if(r->read(r, &b1, 1)<1) {
        fprintf(stderr, "bitio.c:reader_readU16: Read over end of memory region\n");
    }
    if(r->read(r, &b2, 1)<1) {
        fprintf(stderr, "bitio.c:reader_readU16: Read over end of memory region\n");
    }
    return b1|b2<<8;
}
U32 reader_readU32(reader_t*r)
{
    U8 b1=0,b2=0,b3=0,b4=0;
    if(r->read(r, &b1, 1)<1)
        fprintf(stderr, "bitio.c:reader_readU32: Read over end of memory region\n");
    if(r->read(r, &b2, 1)<1)
        fprintf(stderr, "bitio.c:reader_readU32: Read over end of memory region\n");
    if(r->read(r, &b3, 1)<1)
        fprintf(stderr, "bitio.c:reader_readU32: Read over end of memory region\n");
    if(r->read(r, &b4, 1)<1)
        fprintf(stderr, "bitio.c:reader_readU32: Read over end of memory region\n");
    return b1|b2<<8|b3<<16|b4<<24;
}

float reader_readFloat(reader_t*r)
{
#if 1
    float f;
    r->read(r, &f, 4);
    return f;
#else
    U8 b1=0,b2=0,b3=0,b4=0;
    r->read(r, &b1, 1);
    r->read(r, &b2, 1);
    r->read(r, &b3, 1);
    r->read(r, &b4, 1);
    U32 w = (b1|b2<<8|b3<<16|b4<<24);
    return *(MAYALIAS float *)&w;
#endif
}
double reader_readDouble(reader_t*r)
{
#if 1
    double f;
    r->read(r, &f, 8);
    return f;
#else
    U8 b[8];
    r->read(r, b, 8);
    U64 w = ((U64)b[0]|(U64)b[1]<<8|(U64)b[2]<<16|(U64)b[3]<<24|(U64)b[4]<<32|(U64)b[5]<<40|(U64)b[6]<<48|(U64)b[7]<<56);
    return *(double*)&w;
#endif
}
unsigned int read_compressed_uint(reader_t*r)
{
    unsigned int u = 0;
    unsigned int b;
    do {
        b = reader_readU8(r);
        u = u<<7|(b&0x7f);//u = u<<7|b&0x7f;
    } while(b&0x80);
    return u;
}
int read_compressed_int(reader_t*r)
{
    int i = 0;
    int b;

    b = reader_readS8(r);
    i = b&0x7f;

    if(b&0x40)
        i|=0xffffff80; //sign extension

    if(!(b&0x80))
        return i;

    do {
        b = reader_readS8(r);
        i = i<<7|(b&0x7f);//i = i<<7|b&0x7f;
    } while(b&0x80);

    return i;
}