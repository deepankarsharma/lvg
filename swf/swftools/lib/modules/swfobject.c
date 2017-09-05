/* swfobject.c

   Object place and move routines

   Extension module for the rfxswf library.
   Part of the swftools package.

   Copyright (c) 2001 Rainer B

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

#include "../rfxswf.h"
#include <assert.h>

char*blendModeNames[] = {"normal","normal2","layer","multiply",
                         "screen","lighten", "darken","add",
                         "substract","difference","invert","alpha",
                         "erase","overlay","hardlight",0};

int isUnitMatrix(MATRIX* m)
{
    /* a matrix with all zeros is also considered
       "unit matrix", as a zeroed out MATRIX structure
       usually means that the caller doesn't want to
       set the matrix */
    if((   (m->sx == 0x10000 && m->sy == 0x10000)
           || (m->sx == 0 && m->sy == 0))
            && ((m->r0|m->r1|m->tx|m->ty) == 0)
            )
        return 1;
    return 0;
}

int isUnitCXForm(CXFORM* cx)
{
    if((cx->a0==256 && cx->r0==256 && cx->g0==256 && cx->b0==256) &&
            (cx->a1==0 && cx->r1==0 && cx->g1==0 && cx->b1==0))
        return 1;
    /* A CXForm of all zeros is, unfortunately, not as unlikely
       as a matrix of all zeros. However, we still treat it
       as non-existent/uniform transform */
    if((cx->a0==0 && cx->r0==0 && cx->g0==0 && cx->b0==0) &&
            (cx->a1==0 && cx->r1==0 && cx->g1==0 && cx->b1==0))
        return 1;
    return 0;
}

static int objectplace(TAG * t,U16 id,U16 depth,MATRIX * m,CXFORM * cx,const char * name, U16 clipaction, U8 blendmode, FILTERLIST*filters)
{ U8 flags,flags2;
    if (!t) return -1;

    if(cx && id && cx->r1==0 && cx->g1==0 && cx->b1==0 && cx->a1==0
            && cx->r0==256 && cx->g0==256 && cx->b0==256 && cx->a0==256)
        cx = 0;

    if(m && id && isUnitMatrix(m))
        m = 0;

    flags = (id?PF_CHAR:0)|(m?PF_MATRIX:0)|(cx?PF_CXFORM:0)|(name?PF_NAME:0)|((m||cx)&&(!id)?PF_MOVE:0)|(clipaction?PF_CLIPDEPTH:0);
    flags2 = (0?PF2_ASBITMAP:0)|(blendmode?PF2_BLENDMODE:0)|(filters?PF2_FILTERS:0);

    swf_SetU8(t,flags);
    if(t->id == ST_PLACEOBJECT3)
        swf_SetU8(t, flags2);
    swf_SetU16(t,depth);
    if (flags&PF_CHAR) swf_SetU16(t,id);
    if (flags&PF_MATRIX) swf_SetMatrix(t,m);
    if (flags&PF_CXFORM) swf_SetCXForm(t,cx,1);
    if (flags&PF_RATIO) swf_SetU16(t,0);
    /* ??? The spec states that name comes first? */
    if (flags&PF_CLIPDEPTH) swf_SetU16(t, clipaction);
    if (flags&PF_NAME) swf_SetString(t,name);

    if (flags2&PF2_BLENDMODE)
        swf_SetU8(t,blendmode);
    return 0;
}
int swf_ObjectPlace(TAG * t,U16 id,U16 depth,MATRIX * m,CXFORM * cx,const char * name)
{
    return objectplace(t,id,depth,m,cx,name,0,0,0);
}
int swf_ObjectPlaceClip(TAG * t,U16 id,U16 depth,MATRIX * m,CXFORM * cx,const char * name, U16 clipaction)
{
    return objectplace(t,id,depth,m,cx,name,clipaction,0,0);
}
int swf_ObjectPlaceBlend(TAG * t,U16 id,U16 depth,MATRIX * m,CXFORM * cx,const char * name, U8 blend)
{
    if(t->id != ST_PLACEOBJECT3)
        fprintf(stderr, "wrong tag- ignoring blend mode\n");
    return objectplace(t,id,depth,m,cx,name,0,blend,0);
}
int swf_ObjectMove(TAG * t,U16 depth,MATRIX * m,CXFORM * cx)
{
    return objectplace(t,0,depth,m,cx,0,0,0,0);
}

void swf_SetPlaceObject(TAG * t,SWFPLACEOBJECT* obj)
{
    if (!t) return ;
    if(t->id == ST_PLACEOBJECT) {
        swf_SetU16(t, obj->id);
        swf_SetU16(t, obj->depth);
        swf_SetMatrix(t, &obj->matrix);
        swf_SetCXForm(t, &obj->cxform, 0);
    } else {
        U8 flags,flags2;
        int m = !isUnitMatrix(&obj->matrix);
        int cx = !isUnitCXForm(&obj->cxform);

        flags = (obj->id?PF_CHAR:0)|(m?PF_MATRIX:0)|(cx?PF_CXFORM:0)|(obj->ratio?PF_RATIO:0)|
                (obj->name?PF_NAME:0)|(obj->move?PF_MOVE:0)|
                (obj->clipdepth?PF_CLIPDEPTH:0);
        flags2 = (0?PF2_ASBITMAP:0)|(obj->blendmode?PF2_BLENDMODE:0)|(obj->filters?PF2_FILTERS:0);

        swf_SetU8(t,flags);
        if(t->id == ST_PLACEOBJECT3)
            swf_SetU8(t,flags2);
        swf_SetU16(t,obj->depth);
        if (flags&PF_CHAR) swf_SetU16(t,obj->id);
        if (flags&PF_MATRIX) swf_SetMatrix(t,&obj->matrix);
        if (flags&PF_CXFORM) swf_SetCXForm(t,&obj->cxform,1);
        if (flags&PF_RATIO) swf_SetU16(t,obj->ratio);

        /* ??? The spec states that name comes first? */
        if (flags&PF_CLIPDEPTH) swf_SetU16(t,obj->clipdepth);
        if (flags&PF_NAME) swf_SetString(t,obj->name);

        if (flags2&PF2_FILTERS) {
            swf_SetU8(t,obj->filters->num);
            int s;
            for(s=0;s<obj->filters->num;s++)
                swf_SetFilter(t,obj->filters->filter[s]);
        }
        if (flags2&PF2_BLENDMODE)
            swf_SetU8(t,obj->blendmode);
        if (flags&PF_ACTIONEVENT) {
            // ...
        }
    }
}

int swf_GetPlaceObject(TAG *tag, SWFPLACEOBJECT *obj, int version)
{
    if (obj)
        memset(obj, 0, sizeof(SWFPLACEOBJECT));

    if (!tag)
    {
        swf_GetMatrix(0, &obj->matrix);
        swf_GetCXForm(0, &obj->cxform, 1);
        return PF_MATRIX | PF_CXFORM;
    }
    swf_SetTagPos(tag, 0);

    if (tag->id == ST_PLACEOBJECT)
    {
        obj->id = swf_GetU16(tag);
        obj->depth = swf_GetU16(tag);
        swf_GetMatrix(tag, &obj->matrix);
        swf_GetCXForm(tag, &obj->cxform, 0);
        return PF_CHAR | PF_MATRIX | PF_CXFORM;
    } else if (tag->id == ST_PLACEOBJECT2 || tag->id == ST_PLACEOBJECT3)
    {
        U8 flags, flags2 = 0;
        flags = swf_GetU8(tag);
        if (tag->id == ST_PLACEOBJECT3)
            flags2 = swf_GetU8(tag);
        memset(obj, 0, sizeof(SWFPLACEOBJECT));
        swf_GetMatrix(0, &obj->matrix);
        swf_GetCXForm(0, &obj->cxform, 1);

        obj->flags = flags;
        obj->depth = swf_GetU16(tag);
        if ((flags2 & PF2_CLASS_NAME) || ((flags & PF_CHAR) && (flags2 & PF2_HAS_IMAGE)))
        {
            int t = 0, l = strlen((const char *)&tag->data[tag->pos]);
            U8 *data = (U8*)malloc(l+1);
            obj->class_name = (char*)data;
            while ((data[t++] = swf_GetU8(tag)));
        }
        if (flags & PF_MOVE) obj->move = 1;
        if (flags & PF_CHAR) obj->id = swf_GetU16(tag);
        if (flags & PF_MATRIX) swf_GetMatrix(tag, &obj->matrix);
        if (flags & PF_CXFORM) swf_GetCXForm(tag, &obj->cxform,1);
        if (flags & PF_RATIO) obj->ratio = swf_GetU16(tag);
        if (flags & PF_NAME)
        {
            swf_ResetReadBits(tag);
            int t = 0, l = strlen((const char *)&tag->data[tag->pos]);
            U8 *data = (U8*)malloc(l+1);
            obj->name = (char*)data;
            while((data[t++] = swf_GetU8(tag)));
        }
        if (flags & PF_CLIPDEPTH)
            obj->clipdepth = swf_GetU16(tag);
        if (flags2 & PF2_FILTERS)
        {
            int num_filters = swf_GetU8(tag);
            for (int i = 0; i < num_filters; i++)
            {
                int filter = swf_GetU8(tag);
                if (0 == filter)
                {   // DROPSHADOWFILTER
                    swf_GetRGBA(tag, 0);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU16(tag);
                    swf_GetU8(tag);
                } else if (1 == filter)
                {   // BLURFILTER
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU8(tag);
                } else if (2 == filter)
                {   // GLOWFILTER
                    swf_GetRGBA(tag, 0);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU16(tag);
                    swf_GetU8(tag);
                } else if (3 == filter)
                {   // BEVELFILTER
                    swf_GetRGBA(tag, 0);
                    swf_GetRGBA(tag, 0);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU16(tag);
                    swf_GetU8(tag);
                } else if (4 == filter)
                {   // GRADIENTGLOWFILTER
                    int num_colors = swf_GetU8(tag);
                    for (int j = 0; j < num_colors; j++)
                        swf_GetRGBA(tag, 0);
                    for (int j = 0; j < num_colors; j++)
                        swf_GetU8(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU16(tag);
                    swf_GetU8(tag);
                    num_colors = swf_GetU8(tag);
                    for (int j = 0; j < num_colors; j++)
                        swf_GetRGBA(tag, 0);
                    for (int j = 0; j < num_colors; j++)
                        swf_GetU8(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU16(tag);
                    swf_GetU8(tag);
                } else if (5 == filter)
                {   // CONVOLUTIONFILTER
                    int mat_x = swf_GetU8(tag);
                    int mat_y = swf_GetU8(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    for (int y = 0; y < mat_y; y++)
                        for (int x = 0; x < mat_x; x++)
                            swf_GetU32(tag);
                    swf_GetRGBA(tag, 0);
                    swf_GetU8(tag);
                } else if (6 == filter)
                {   // COLORMATRIXFILTER
                    for (int j = 0; j < 20; j++)
                        swf_GetU32(tag);
                } else if (7 == filter)
                {   // GRADIENTBEVELFILTER
                    int num_colors = swf_GetU8(tag);
                    for (int j = 0; j < num_colors; j++)
                        swf_GetRGBA(tag, 0);
                    for (int j = 0; j < num_colors; j++)
                        swf_GetU8(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU16(tag);
                    swf_GetU8(tag);
                    num_colors = swf_GetU8(tag);
                    for (int j = 0; j < num_colors; j++)
                        swf_GetRGBA(tag, 0);
                    for (int j = 0; j < num_colors; j++)
                        swf_GetU8(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU32(tag);
                    swf_GetU16(tag);
                    swf_GetU8(tag);
                } else
                {
                    assert(0);
                }
            }
        }
        if (flags2 & PF2_BLENDMODE)
            obj->blendmode = swf_GetU8(tag);
        if (flags2 & PF2_ASBITMAP)
        {
            /*int bcache_enabled = */swf_GetU8(tag);
            swf_GetRGBA(tag, 0);
        }
        if (flags & PF_ACTIONEVENT)
        {
            int reserved = swf_GetU16(tag); // unused
            assert(!reserved);
            U32 allevent_flags = swf_GetU16(tag);
            if (version >= 6)
                allevent_flags |= swf_GetU16(tag) << 16;
            while (1)
            {
                U32 event_flags = swf_GetU16(tag);
                if (version >= 6)
                    event_flags |= swf_GetU16(tag) << 16;
                if (!event_flags)
                    break;
                /*int actions_size = */swf_GetU32(tag);
                if (event_flags & (2 << 16))
                {
                    /*int key_code = */swf_GetU8(tag);
                }
                obj->actions = swf_ActionGet(tag);
            }
        } else
            obj->actions = 0;
        return flags;
    } else
        fprintf(stderr, "rfxswf: Bad Tag: %d not a placeobject\n", tag->id);
    return 0;
}

void swf_PlaceObjectFree(SWFPLACEOBJECT* obj)
{
    if (obj->name)
        free(obj->name);
    obj->name = 0;
}
