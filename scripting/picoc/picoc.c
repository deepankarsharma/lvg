#include "picoc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../src/lvg_header.h"
#include "../../render/gl.h"

#define PICOC_STACK_SIZE (128*1024)              /* space for the the stack */

const char g_lvgDefs[] = "\
typedef unsigned int GLenum;\
typedef unsigned char GLboolean;\
typedef unsigned int GLbitfield;\
typedef void GLvoid;\
typedef signed char GLbyte;\
typedef short GLshort;\
typedef int GLint;\
typedef int GLclampx;\
typedef unsigned char GLubyte;\
typedef unsigned short GLushort;\
typedef unsigned int GLuint;\
typedef int GLsizei;\
typedef float GLfloat;\
typedef float GLclampf;\
typedef double GLdouble;\
typedef double GLclampd;\
typedef char GLchar;\
typedef long GLsizeiptr;\
typedef struct LVGSound\
{\
    short *samples;\
    int num_samples; int orig_rate; int rate; int channels;\
} LVGSound;\
";

#define Int(n) Param[n]->Val->Integer
#define Ptr(n) Param[n]->Val->Pointer
#define Float(n) Param[n]->Val->FP

static void lib_glCreateProgram(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (0 != NumArgs)
        return;
    ReturnValue->Val->Integer = glCreateProgram();
}

static void lib_glCreateShader(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    ReturnValue->Val->Integer = glCreateShader(Int(0));
}

static void lib_glShaderSource(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (4 != NumArgs)
        return;
    glShaderSource(Int(0), Int(1), Ptr(2), Ptr(3));
}

static void lib_glCompileShader(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glCompileShader(Int(0));
}

static void lib_glGetShaderiv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (3 != NumArgs)
        return;
    glGetShaderiv(Int(0), Int(1), Ptr(2));
}

static void lib_glGetShaderInfoLog(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (4 != NumArgs)
        return;
    glGetShaderInfoLog(Int(0), Int(1), Ptr(2), Ptr(3));
}

static void lib_glAttachShader(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    glAttachShader(Int(0), Int(1));
}

static void lib_glLinkProgram(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glLinkProgram(Int(0));
}

static void lib_glGetProgramiv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (3 != NumArgs)
        return;
    glGetProgramiv(Int(0), Int(1), Ptr(2));
}

static void lib_glGetUniformLocation(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    ReturnValue->Val->Integer = glGetUniformLocation(Int(0), Ptr(1));
}

static void lib_glGetAttribLocation(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    ReturnValue->Val->Integer = glGetAttribLocation(Int(0), Ptr(1));
}

static void lib_glGenBuffers(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    glGenBuffers(Int(0), Ptr(1));
}

static void lib_glBindBuffer(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    glBindBuffer(Int(0), Int(1));
}

static void lib_glEnableVertexAttribArray(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glEnableVertexAttribArray(Int(0));
}

static void lib_glVertexAttribPointer(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (6 != NumArgs)
        return;
    glVertexAttribPointer(Int(0), Int(1), Int(2), Int(3), Int(4), Ptr(5));
}

static void lib_glBindTexture(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    glBindTexture(Int(0), Int(1));
}

static void lib_glGenTextures(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    glGenTextures(Int(0), Ptr(1));
}

static void lib_glTexParameteri(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (3 != NumArgs)
        return;
    glTexParameteri(Int(0), Int(1), Int(2));
}

static void lib_glTexImage2D(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (9 != NumArgs)
        return;
    glTexImage2D(Int(0), Int(1), Int(2), Int(3), Int(4), Int(5), Int(6), Int(7), Ptr(8));
}

static void lib_glDetachShader(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    glDetachShader(Int(0), Int(1));
}

static void lib_glDeleteShader(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glDeleteShader(Int(0));
}

static void lib_glDeleteProgram(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glDeleteProgram(Int(0));
}

static void lib_glDeleteTextures(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    glDeleteTextures(Int(0), Ptr(1));
}

static void lib_glDeleteBuffers(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    glDeleteBuffers(Int(0), Ptr(1));
}

static void lib_glEnable(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glEnable(Int(0));
}

static void lib_glBlendEquation(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glBlendEquation(Int(0));
}

static void lib_glBlendFunc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    glBlendFunc(Int(0), Int(1));
}

static void lib_glDisable(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glDisable(Int(0));
}

static void lib_glActiveTexture(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glActiveTexture(Int(0));
}

static void lib_glUseProgram(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glUseProgram(Int(0));
}

static void lib_glUniform1i(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    glUniform1i(Int(0), Int(1));
}

static void lib_glUniformMatrix4fv(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (4 != NumArgs)
        return;
    glUniformMatrix4fv(Int(0), Int(1), Int(2), Ptr(3));
}

static void lib_glBufferData(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (4 != NumArgs)
        return;
    glBufferData(Int(0), Int(1), Ptr(2), Int(3));
}

static void lib_glMapBuffer(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    ReturnValue->Val->Pointer = glMapBuffer(Int(0), Int(1));
}

static void lib_glUnmapBuffer(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    ReturnValue->Val->Integer = glUnmapBuffer(Int(0));
}

static void lib_glScissor(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (4 != NumArgs)
        return;
    glScissor(Int(0), Int(1), Int(2), Int(3));
}

static void lib_glDrawElements(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (4 != NumArgs)
        return;
    glDrawElements(Int(0), Int(1), Int(2), Ptr(3));
}

static void lib_glPushAttrib(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glPushAttrib(Int(0));
}

static void lib_glViewport(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (4 != NumArgs)
        return;
    glViewport(Int(0), Int(1), Int(2), Int(3));
}

static void lib_glMatrixMode(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glMatrixMode(Int(0));
}

static void lib_glPushMatrix(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (0 != NumArgs)
        return;
    glPushMatrix();
}

static void lib_glLoadIdentity(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (0 != NumArgs)
        return;
    glLoadIdentity();
}

static void lib_glOrtho(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (6 != NumArgs)
        return;
    glOrtho(Float(0), Float(1), Float(2), Float(3), Float(4), Float(5));
}

static void lib_glEnableClientState(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (1 != NumArgs)
        return;
    glEnableClientState(Int(0));
}

static void lib_lvgGetFileContents(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (2 != NumArgs)
        return;
    ReturnValue->Val->Pointer = lvgGetFileContents(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

static void lib_lvgLoadMP3(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (4 != NumArgs)
        return;
    ReturnValue->Val->Pointer = lvgLoadMP3(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Pointer, Param[3]->Val->Pointer);
}

static void lib_lvgPlaySound(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    if (5 != NumArgs)
        return;
    lvgPlaySound(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Integer, Param[3]->Val->Integer, Param[4]->Val->Integer);
}

static const struct LibraryFunction g_lvgLib[] =
{
    /* GL2 */
    { lib_glCreateProgram, "GLuint glCreateProgram();" },
    { lib_glCreateShader, "GLuint glCreateShader(GLenum type);" },
    { lib_glShaderSource, "void glShaderSource(GLuint shader, GLsizei count, char **string, GLint *length);" },
    { lib_glCompileShader, "void glCompileShader(GLuint shader);" },
    { lib_glGetShaderiv, "void glGetShaderiv(GLuint shader, GLenum pname, GLint *params);" },
    { lib_glGetShaderInfoLog, "void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);" },
    { lib_glAttachShader, "void glAttachShader(GLuint program, GLuint shader);" },
    { lib_glLinkProgram, "void glLinkProgram(GLuint program);" },
    { lib_glGetProgramiv, "void glGetProgramiv(GLuint program, GLenum pname, GLint *params);" },
    { lib_glGetUniformLocation, "GLint glGetUniformLocation(GLuint program, GLchar *name);" },
    { lib_glGetAttribLocation, "GLint glGetAttribLocation(GLuint program, GLchar *name);" },
    { lib_glGenBuffers, "void glGenBuffers(GLsizei n, GLuint *buffers);" },
    { lib_glBindBuffer, "void glBindBuffer(GLenum target, GLuint buffer);" },
    { lib_glEnableVertexAttribArray, "void glEnableVertexAttribArray(GLuint index);" },
    { lib_glVertexAttribPointer, "void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, void *pointer);" },
    { lib_glBindTexture, "void glBindTexture(GLenum target, GLuint texture);" },
    { lib_glGenTextures, "void glGenTextures(GLsizei n, GLuint *textures);" },
    { lib_glTexParameteri, "void glTexParameteri(GLenum target, GLenum pname, GLint param);" },
    { lib_glTexImage2D, "void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, void *pixels);" },
    { lib_glDetachShader, "void glDetachShader(GLuint program, GLuint shader);" },
    { lib_glDeleteShader, "void glDeleteShader(GLuint shader);" },
    { lib_glDeleteProgram, "void glDeleteProgram(GLuint program);" },
    { lib_glDeleteTextures, "void glDeleteTextures(GLsizei n, GLuint *textures);" },
    { lib_glDeleteBuffers, "void glDeleteBuffers(GLsizei n, GLuint *buffers);" },
    { lib_glEnable, "void glEnable(GLenum cap);" },
    { lib_glBlendEquation, "void glBlendEquation(GLenum mode);" },
    { lib_glBlendFunc, "void glBlendFunc(GLenum sfactor, GLenum dfactor);" },
    { lib_glDisable, "void glDisable(GLenum cap);" },
    { lib_glActiveTexture, "void glActiveTexture(GLenum texture);" },
    { lib_glUseProgram, "void glUseProgram(GLuint program);" },
    { lib_glUniform1i, "void glUniform1i(GLint location, GLint v0);" },
    { lib_glUniformMatrix4fv, "void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, GLfloat *value);" },
    { lib_glBufferData, "void glBufferData(GLenum target, GLsizeiptr size, void *data, GLenum usage);" },
    { lib_glMapBuffer, "char *glMapBuffer(GLenum target, GLenum access);" },
    { lib_glUnmapBuffer, "GLboolean glUnmapBuffer(GLenum target);" },
    { lib_glScissor, "void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);" },
    { lib_glDrawElements, "void glDrawElements(GLenum mode, GLsizei count, GLenum type, void *indices);" },
    { lib_glPushAttrib, "void glPushAttrib(GLbitfield mask);" },
    { lib_glViewport, "void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);" },
    { lib_glMatrixMode, "void glMatrixMode(GLenum mode);" },
    { lib_glPushMatrix, "void glPushMatrix();" },
    { lib_glLoadIdentity, "void glLoadIdentity();" },
    { lib_glOrtho, "void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);" },
    { lib_glEnableClientState, "void glEnableClientState(GLenum array);" },
#ifdef USE_GL3
    { lib_glGenVertexArrays, "void glGenVertexArrays();" },
    { lib_glBindVertexArray, "void glBindVertexArray();" },
    { lib_glMapBufferRange, "void glMapBufferRange();" },
#endif
    /* LVG API */
    { lib_lvgGetFileContents, "char *lvgGetFileContents(char *fname, int *size);" },
    { lib_lvgLoadMP3, "short *lvgLoadMP3(char *file, int *rate, int *channels, int *num_samples);" },
    { lib_lvgPlaySound, "void lvgPlaySound(LVGSound *sound, int flags, int start_sample, int end_sample, int loops);" },
    { NULL, NULL }
};

int loadScript()
{
    int StackSize = PICOC_STACK_SIZE;
    Picoc pc;

    PicocInitialise(&pc, StackSize);
    PicocIncludeAllSystemHeaders(&pc);
    PicocParse(&pc, "lvg.h", g_lvgDefs, strlen(g_lvgDefs), TRUE, TRUE, FALSE, FALSE);
    LibraryAdd(&pc, &pc.GlobalTable, "lvg library", &g_lvgLib[0]);
    //PicocParseInteractive(&pc);

    if (PicocPlatformSetExitPoint(&pc))
    {
        PicocCleanup(&pc);
        return pc.PicocExitValue;
    }

    PicocPlatformScanFile(&pc, "main.c");

    //if (!DontRunMain)
    //    PicocCallMain(&pc, argc - ParamCount, &argv[ParamCount]);

    PicocCleanup(&pc);
    return pc.PicocExitValue;
}