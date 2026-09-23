// rendergl.cpp: core opengl rendering stuff

#include "engine.h"

#ifdef __ANDROID__
#include <dlfcn.h>
#include "androidbridge.h"
#endif

bool hasVAO = false, hasFBO = false, hasAFBO = false, hasDS = false, hasTF = false, hasTRG = false, hasTSW = false, hasS3TC = false, hasFXT1 = false, hasLATC = false, hasRGTC = false, hasAF = false, hasFBB = false, hasUBO = false, hasMBR = false;

VAR(glversion, 1, 0, 0);
VAR(glslversion, 1, 0, 0);
VAR(glcompat, 1, 0, 0);

// OpenGL 1.3
#ifdef WIN32
PFNGLACTIVETEXTUREPROC       glActiveTexture_       = NULL;

PFNGLBLENDEQUATIONEXTPROC glBlendEquation_ = NULL;
PFNGLBLENDCOLOREXTPROC    glBlendColor_    = NULL;

PFNGLTEXIMAGE3DPROC        glTexImage3D_        = NULL;
PFNGLTEXSUBIMAGE3DPROC     glTexSubImage3D_     = NULL;
PFNGLCOPYTEXSUBIMAGE3DPROC glCopyTexSubImage3D_ = NULL;

PFNGLCOMPRESSEDTEXIMAGE3DPROC    glCompressedTexImage3D_    = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC    glCompressedTexImage2D_    = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glCompressedTexSubImage3D_ = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D_ = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEPROC   glGetCompressedTexImage_   = NULL;

PFNGLDRAWRANGEELEMENTSPROC glDrawRangeElements_ = NULL;
#endif

// OpenGL 2.0
#ifndef __APPLE__
PFNGLMULTIDRAWARRAYSPROC   glMultiDrawArrays_   = NULL;
PFNGLMULTIDRAWELEMENTSPROC glMultiDrawElements_ = NULL;

PFNGLBLENDFUNCSEPARATEPROC     glBlendFuncSeparate_     = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC glBlendEquationSeparate_ = NULL;
PFNGLSTENCILOPSEPARATEPROC     glStencilOpSeparate_     = NULL;
PFNGLSTENCILFUNCSEPARATEPROC   glStencilFuncSeparate_   = NULL;
PFNGLSTENCILMASKSEPARATEPROC   glStencilMaskSeparate_   = NULL;

PFNGLGENBUFFERSPROC       glGenBuffers_       = NULL;
PFNGLBINDBUFFERPROC       glBindBuffer_       = NULL;
PFNGLMAPBUFFERPROC        glMapBuffer_        = NULL;
PFNGLUNMAPBUFFERPROC      glUnmapBuffer_      = NULL;
PFNGLBUFFERDATAPROC       glBufferData_       = NULL;
PFNGLBUFFERSUBDATAPROC    glBufferSubData_    = NULL;
PFNGLDELETEBUFFERSPROC    glDeleteBuffers_    = NULL;
PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData_ = NULL;

PFNGLGENQUERIESPROC        glGenQueries_        = NULL;
PFNGLDELETEQUERIESPROC     glDeleteQueries_     = NULL;
PFNGLBEGINQUERYPROC        glBeginQuery_        = NULL;
PFNGLENDQUERYPROC          glEndQuery_          = NULL;
PFNGLGETQUERYIVPROC        glGetQueryiv_        = NULL;
PFNGLGETQUERYOBJECTIVPROC  glGetQueryObjectiv_  = NULL;
PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv_ = NULL;

PFNGLCREATEPROGRAMPROC            glCreateProgram_            = NULL;
PFNGLDELETEPROGRAMPROC            glDeleteProgram_            = NULL;
PFNGLUSEPROGRAMPROC               glUseProgram_               = NULL;
PFNGLCREATESHADERPROC             glCreateShader_             = NULL;
PFNGLDELETESHADERPROC             glDeleteShader_             = NULL;
PFNGLSHADERSOURCEPROC             glShaderSource_             = NULL;
PFNGLCOMPILESHADERPROC            glCompileShader_            = NULL;
PFNGLGETSHADERIVPROC              glGetShaderiv_              = NULL;
PFNGLGETPROGRAMIVPROC             glGetProgramiv_             = NULL;
PFNGLATTACHSHADERPROC             glAttachShader_             = NULL;
PFNGLGETPROGRAMINFOLOGPROC        glGetProgramInfoLog_        = NULL;
PFNGLGETSHADERINFOLOGPROC         glGetShaderInfoLog_         = NULL;
PFNGLLINKPROGRAMPROC              glLinkProgram_              = NULL;
PFNGLGETUNIFORMLOCATIONPROC       glGetUniformLocation_       = NULL;
PFNGLUNIFORM1FPROC                glUniform1f_                = NULL;
PFNGLUNIFORM2FPROC                glUniform2f_                = NULL;
PFNGLUNIFORM3FPROC                glUniform3f_                = NULL;
PFNGLUNIFORM4FPROC                glUniform4f_                = NULL;
PFNGLUNIFORM1FVPROC               glUniform1fv_               = NULL;
PFNGLUNIFORM2FVPROC               glUniform2fv_               = NULL;
PFNGLUNIFORM3FVPROC               glUniform3fv_               = NULL;
PFNGLUNIFORM4FVPROC               glUniform4fv_               = NULL;
PFNGLUNIFORM1IPROC                glUniform1i_                = NULL;
PFNGLUNIFORM2IPROC                glUniform2i_                = NULL;
PFNGLUNIFORM3IPROC                glUniform3i_                = NULL;
PFNGLUNIFORM4IPROC                glUniform4i_                = NULL;
PFNGLUNIFORM1IVPROC               glUniform1iv_               = NULL;
PFNGLUNIFORM2IVPROC               glUniform2iv_               = NULL;
PFNGLUNIFORM3IVPROC               glUniform3iv_               = NULL;
PFNGLUNIFORM4IVPROC               glUniform4iv_               = NULL;
PFNGLUNIFORMMATRIX2FVPROC         glUniformMatrix2fv_         = NULL;
PFNGLUNIFORMMATRIX3FVPROC         glUniformMatrix3fv_         = NULL;
PFNGLUNIFORMMATRIX4FVPROC         glUniformMatrix4fv_         = NULL;
PFNGLBINDATTRIBLOCATIONPROC       glBindAttribLocation_       = NULL;
PFNGLGETACTIVEUNIFORMPROC         glGetActiveUniform_         = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray_  = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray_ = NULL;

PFNGLVERTEXATTRIB1FPROC           glVertexAttrib1f_           = NULL;
PFNGLVERTEXATTRIB1FVPROC          glVertexAttrib1fv_          = NULL;
PFNGLVERTEXATTRIB1SPROC           glVertexAttrib1s_           = NULL;
PFNGLVERTEXATTRIB1SVPROC          glVertexAttrib1sv_          = NULL;
PFNGLVERTEXATTRIB2FPROC           glVertexAttrib2f_           = NULL;
PFNGLVERTEXATTRIB2FVPROC          glVertexAttrib2fv_          = NULL;
PFNGLVERTEXATTRIB2SPROC           glVertexAttrib2s_           = NULL;
PFNGLVERTEXATTRIB2SVPROC          glVertexAttrib2sv_          = NULL;
PFNGLVERTEXATTRIB3FPROC           glVertexAttrib3f_           = NULL;
PFNGLVERTEXATTRIB3FVPROC          glVertexAttrib3fv_          = NULL;
PFNGLVERTEXATTRIB3SPROC           glVertexAttrib3s_           = NULL;
PFNGLVERTEXATTRIB3SVPROC          glVertexAttrib3sv_          = NULL;
PFNGLVERTEXATTRIB4FPROC           glVertexAttrib4f_           = NULL;
PFNGLVERTEXATTRIB4FVPROC          glVertexAttrib4fv_          = NULL;
PFNGLVERTEXATTRIB4SPROC           glVertexAttrib4s_           = NULL;
PFNGLVERTEXATTRIB4SVPROC          glVertexAttrib4sv_          = NULL;
PFNGLVERTEXATTRIB4BVPROC          glVertexAttrib4bv_          = NULL;
PFNGLVERTEXATTRIB4IVPROC          glVertexAttrib4iv_          = NULL;
PFNGLVERTEXATTRIB4UBVPROC         glVertexAttrib4ubv_         = NULL;
PFNGLVERTEXATTRIB4UIVPROC         glVertexAttrib4uiv_         = NULL;
PFNGLVERTEXATTRIB4USVPROC         glVertexAttrib4usv_         = NULL;
PFNGLVERTEXATTRIB4NBVPROC         glVertexAttrib4Nbv_         = NULL;
PFNGLVERTEXATTRIB4NIVPROC         glVertexAttrib4Niv_         = NULL;
PFNGLVERTEXATTRIB4NUBPROC         glVertexAttrib4Nub_         = NULL;
PFNGLVERTEXATTRIB4NUBVPROC        glVertexAttrib4Nubv_        = NULL;
PFNGLVERTEXATTRIB4NUIVPROC        glVertexAttrib4Nuiv_        = NULL;
PFNGLVERTEXATTRIB4NUSVPROC        glVertexAttrib4Nusv_        = NULL;
PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer_      = NULL;

PFNGLDRAWBUFFERSPROC glDrawBuffers_ = NULL;
#endif

// OpenGL 3.0
PFNGLGETSTRINGIPROC           glGetStringi_           = NULL;
PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation_ = NULL;

// GL_EXT_framebuffer_object
PFNGLBINDRENDERBUFFERPROC        glBindRenderbuffer_        = NULL;
PFNGLDELETERENDERBUFFERSPROC     glDeleteRenderbuffers_     = NULL;
PFNGLGENFRAMEBUFFERSPROC         glGenRenderbuffers_        = NULL;
PFNGLRENDERBUFFERSTORAGEPROC     glRenderbufferStorage_     = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC  glCheckFramebufferStatus_  = NULL;
PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer_         = NULL;
PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers_      = NULL;
PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers_         = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC    glFramebufferTexture2D_    = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer_ = NULL;
PFNGLGENERATEMIPMAPPROC          glGenerateMipmap_          = NULL;

// GL_EXT_framebuffer_blit
PFNGLBLITFRAMEBUFFERPROC         glBlitFramebuffer_         = NULL;

// GL_ARB_uniform_buffer_object
PFNGLGETUNIFORMINDICESPROC       glGetUniformIndices_       = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC     glGetActiveUniformsiv_     = NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC    glGetUniformBlockIndex_    = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glGetActiveUniformBlockiv_ = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC     glUniformBlockBinding_     = NULL;
PFNGLBINDBUFFERBASEPROC          glBindBufferBase_          = NULL;
PFNGLBINDBUFFERRANGEPROC         glBindBufferRange_         = NULL;

// GL_ARB_map_buffer_range
PFNGLMAPBUFFERRANGEPROC         glMapBufferRange_         = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC glFlushMappedBufferRange_ = NULL;

// GL_ARB_vertex_array_object
PFNGLBINDVERTEXARRAYPROC    glBindVertexArray_    = NULL;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays_ = NULL;
PFNGLGENVERTEXARRAYSPROC    glGenVertexArrays_    = NULL;
PFNGLISVERTEXARRAYPROC      glIsVertexArray_      = NULL;

void *getprocaddress(const char *name)
{
    void *proc = SDL_GL_GetProcAddress(name);
#ifdef __ANDROID__
    // The EGL spec only guarantees eglGetProcAddress() (what
    // SDL_GL_GetProcAddress() calls into on Android) resolves *extension*
    // functions -- core functions are commonly, but not universally,
    // supported too. On this Adreno driver (Quest 3, confirmed on-device),
    // it returns NULL for plain core GLES3 names like "glBindVertexArray"
    // even though the symbol is right there, already linked in, in the
    // same libGLESv3.so this binary links against (that's how e.g.
    // glDrawBuffers -- called directly, no getprocaddress() involved --
    // already works). Fall back to dlsym against the process's already-
    // loaded libraries, which finds it directly.
    if(!proc) proc = dlsym(RTLD_DEFAULT, name);
#endif
    return proc;
}

VARP(ati_skybox_bug, 0, 0, 1);
VAR(ati_minmax_bug, 0, 0, 1);
VAR(ati_cubemap_bug, 0, 0, 1);
VAR(intel_vertexarray_bug, 0, 0, 1);
VAR(intel_mapbufferrange_bug, 0, 0, 1);
VAR(mesa_swap_bug, 0, 0, 1);
VAR(minimizetcusage, 1, 0, 0);
VAR(useubo, 1, 0, 0);
VAR(usetexcompress, 1, 0, 0);
VAR(rtscissor, 0, 1, 1);
VAR(blurtile, 0, 1, 1);
VAR(rtsharefb, 0, 1, 1);

VAR(dbgexts, 0, 0, 1);

hashset<const char *> glexts;

void parseglexts()
{
    // glGetStringi(GL_EXTENSIONS, i) is a desktop-GL-3.0-core-profile
    // feature (the replacement for the old single-space-separated-string
    // glGetString(GL_EXTENSIONS), which core profile deprecated). GLES
    // never adopted it -- GLES 3.x still uses the classic single-string
    // form -- so glGetStringi_ is left NULL on Android (see
    // gl_checkextensions()'s __ANDROID__ guard on loading it), and this
    // branch must never run there; calling through it would be a
    // null-pointer crash, not a soft failure.
#ifndef __ANDROID__
    if(glversion >= 300)
    {
        GLint numexts = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numexts);
        loopi(numexts)
        {
            const char *ext = (const char *)glGetStringi_(GL_EXTENSIONS, i);
            glexts.add(newstring(ext));
        }
    }
    else
#endif
    {
        const char *exts = (const char *)glGetString(GL_EXTENSIONS);
        for(;;)
        {
            while(*exts == ' ') exts++;
            if(!*exts) break;
            const char *ext = exts;
            while(*exts && *exts != ' ') exts++;
            if(exts > ext) glexts.add(newstring(ext, size_t(exts-ext)));
        }
    }
}

bool hasext(const char *ext)
{
    return glexts.access(ext)!=NULL;
}

// Desktop GL_VERSION/GL_SHADING_LANGUAGE_VERSION strings start with the
// version number itself (e.g. "3.3.0 NVIDIA 550.xx"), but GLES strings
// prefix it with "OpenGL ES "/"OpenGL ES GLSL ES " (e.g. "OpenGL ES 3.2
// ..."), so anchoring the scan at the very start of the string (as a
// plain `sscanf(str, " %u.%u", ...)` does) only works on desktop -- on
// GLES it fails to match at all, falling back to glversion=100 and
// hard-crashing via fatal() below even on a fully capable driver. Scan
// for the first "<digits>.<digits>" anywhere in the string instead, which
// handles both formats without needing a platform branch.
static bool findglversion(const char *str, uint &major, uint &minor)
{
    if(!str) return false;
    for(const char *s = str; *s; s++)
    {
        if(isdigit(*s) && sscanf(s, "%u.%u", &major, &minor) == 2) return true;
    }
    return false;
}

void gl_checkextensions()
{
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    const char *version = (const char *)glGetString(GL_VERSION);
    conoutf(CON_INIT, "Renderer: %s (%s)", renderer, vendor);
    conoutf(CON_INIT, "Driver: %s", version);

    bool mesa = false, intel = false, ati = false, nvidia = false;
    if(strstr(renderer, "Mesa") || strstr(version, "Mesa"))
    {
        mesa = true;
        if(strstr(renderer, "Intel")) intel = true;
    }
    else if(strstr(vendor, "NVIDIA"))
        nvidia = true;
    else if(strstr(vendor, "ATI") || strstr(vendor, "Advanced Micro Devices"))
        ati = true;
    else if(strstr(vendor, "Intel"))
        intel = true;

    uint glmajorversion, glminorversion;
    if(!findglversion(version, glmajorversion, glminorversion)) glversion = 100;
    else glversion = glmajorversion*100 + glminorversion*10;

    if(glversion < 200) fatal("OpenGL 2.0 or greater is required!");

#ifdef WIN32
    glActiveTexture_ =            (PFNGLACTIVETEXTUREPROC)            getprocaddress("glActiveTexture");

    glBlendEquation_ =            (PFNGLBLENDEQUATIONPROC)            getprocaddress("glBlendEquation");
    glBlendColor_ =               (PFNGLBLENDCOLORPROC)               getprocaddress("glBlendColor");

    glTexImage3D_ =               (PFNGLTEXIMAGE3DPROC)               getprocaddress("glTexImage3D");
    glTexSubImage3D_ =            (PFNGLTEXSUBIMAGE3DPROC)            getprocaddress("glTexSubImage3D");
    glCopyTexSubImage3D_ =        (PFNGLCOPYTEXSUBIMAGE3DPROC)        getprocaddress("glCopyTexSubImage3D");

    glCompressedTexImage3D_ =     (PFNGLCOMPRESSEDTEXIMAGE3DPROC)     getprocaddress("glCompressedTexImage3D");
    glCompressedTexImage2D_ =     (PFNGLCOMPRESSEDTEXIMAGE2DPROC)     getprocaddress("glCompressedTexImage2D");
    glCompressedTexSubImage3D_ =  (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)  getprocaddress("glCompressedTexSubImage3D");
    glCompressedTexSubImage2D_ =  (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)  getprocaddress("glCompressedTexSubImage2D");
    glGetCompressedTexImage_ =    (PFNGLGETCOMPRESSEDTEXIMAGEPROC)    getprocaddress("glGetCompressedTexImage");

    glDrawRangeElements_ =        (PFNGLDRAWRANGEELEMENTSPROC)        getprocaddress("glDrawRangeElements");
#endif

#ifndef __APPLE__
    glMultiDrawArrays_ =          (PFNGLMULTIDRAWARRAYSPROC)          getprocaddress("glMultiDrawArrays");
    glMultiDrawElements_ =        (PFNGLMULTIDRAWELEMENTSPROC)        getprocaddress("glMultiDrawElements");

    glBlendFuncSeparate_ =        (PFNGLBLENDFUNCSEPARATEPROC)        getprocaddress("glBlendFuncSeparate");
    glBlendEquationSeparate_ =    (PFNGLBLENDEQUATIONSEPARATEPROC)    getprocaddress("glBlendEquationSeparate");
    glStencilOpSeparate_ =        (PFNGLSTENCILOPSEPARATEPROC)        getprocaddress("glStencilOpSeparate");
    glStencilFuncSeparate_ =      (PFNGLSTENCILFUNCSEPARATEPROC)      getprocaddress("glStencilFuncSeparate");
    glStencilMaskSeparate_ =      (PFNGLSTENCILMASKSEPARATEPROC)      getprocaddress("glStencilMaskSeparate");

    glGenBuffers_ =               (PFNGLGENBUFFERSPROC)               getprocaddress("glGenBuffers");
    glBindBuffer_ =               (PFNGLBINDBUFFERPROC)               getprocaddress("glBindBuffer");
    glMapBuffer_ =                (PFNGLMAPBUFFERPROC)                getprocaddress("glMapBuffer");
    glUnmapBuffer_ =              (PFNGLUNMAPBUFFERPROC)              getprocaddress("glUnmapBuffer");
    glBufferData_ =               (PFNGLBUFFERDATAPROC)               getprocaddress("glBufferData");
    glBufferSubData_ =            (PFNGLBUFFERSUBDATAPROC)            getprocaddress("glBufferSubData");
    glDeleteBuffers_ =            (PFNGLDELETEBUFFERSPROC)            getprocaddress("glDeleteBuffers");
    glGetBufferSubData_ =         (PFNGLGETBUFFERSUBDATAPROC)         getprocaddress("glGetBufferSubData");

    glGetQueryiv_ =               (PFNGLGETQUERYIVPROC)               getprocaddress("glGetQueryiv");
    glGenQueries_ =               (PFNGLGENQUERIESPROC)               getprocaddress("glGenQueries");
    glDeleteQueries_ =            (PFNGLDELETEQUERIESPROC)            getprocaddress("glDeleteQueries");
    glBeginQuery_ =               (PFNGLBEGINQUERYPROC)               getprocaddress("glBeginQuery");
    glEndQuery_ =                 (PFNGLENDQUERYPROC)                 getprocaddress("glEndQuery");
    glGetQueryObjectiv_ =         (PFNGLGETQUERYOBJECTIVPROC)         getprocaddress("glGetQueryObjectiv");
    glGetQueryObjectuiv_ =        (PFNGLGETQUERYOBJECTUIVPROC)        getprocaddress("glGetQueryObjectuiv");

    glCreateProgram_ =            (PFNGLCREATEPROGRAMPROC)            getprocaddress("glCreateProgram");
    glDeleteProgram_ =            (PFNGLDELETEPROGRAMPROC)            getprocaddress("glDeleteProgram");
    glUseProgram_ =               (PFNGLUSEPROGRAMPROC)               getprocaddress("glUseProgram");
    glCreateShader_ =             (PFNGLCREATESHADERPROC)             getprocaddress("glCreateShader");
    glDeleteShader_ =             (PFNGLDELETESHADERPROC)             getprocaddress("glDeleteShader");
    glShaderSource_ =             (PFNGLSHADERSOURCEPROC)             getprocaddress("glShaderSource");
    glCompileShader_ =            (PFNGLCOMPILESHADERPROC)            getprocaddress("glCompileShader");
    glGetShaderiv_ =              (PFNGLGETSHADERIVPROC)              getprocaddress("glGetShaderiv");
    glGetProgramiv_ =             (PFNGLGETPROGRAMIVPROC)             getprocaddress("glGetProgramiv");
    glAttachShader_ =             (PFNGLATTACHSHADERPROC)             getprocaddress("glAttachShader");
    glGetProgramInfoLog_ =        (PFNGLGETPROGRAMINFOLOGPROC)        getprocaddress("glGetProgramInfoLog");
    glGetShaderInfoLog_ =         (PFNGLGETSHADERINFOLOGPROC)         getprocaddress("glGetShaderInfoLog");
    glLinkProgram_ =              (PFNGLLINKPROGRAMPROC)              getprocaddress("glLinkProgram");
    glGetUniformLocation_ =       (PFNGLGETUNIFORMLOCATIONPROC)       getprocaddress("glGetUniformLocation");
    glUniform1f_ =                (PFNGLUNIFORM1FPROC)                getprocaddress("glUniform1f");
    glUniform2f_ =                (PFNGLUNIFORM2FPROC)                getprocaddress("glUniform2f");
    glUniform3f_ =                (PFNGLUNIFORM3FPROC)                getprocaddress("glUniform3f");
    glUniform4f_ =                (PFNGLUNIFORM4FPROC)                getprocaddress("glUniform4f");
    glUniform1fv_ =               (PFNGLUNIFORM1FVPROC)               getprocaddress("glUniform1fv");
    glUniform2fv_ =               (PFNGLUNIFORM2FVPROC)               getprocaddress("glUniform2fv");
    glUniform3fv_ =               (PFNGLUNIFORM3FVPROC)               getprocaddress("glUniform3fv");
    glUniform4fv_ =               (PFNGLUNIFORM4FVPROC)               getprocaddress("glUniform4fv");
    glUniform1i_ =                (PFNGLUNIFORM1IPROC)                getprocaddress("glUniform1i");
    glUniform2i_ =                (PFNGLUNIFORM2IPROC)                getprocaddress("glUniform2i");
    glUniform3i_ =                (PFNGLUNIFORM3IPROC)                getprocaddress("glUniform3i");
    glUniform4i_ =                (PFNGLUNIFORM4IPROC)                getprocaddress("glUniform4i");
    glUniform1iv_ =               (PFNGLUNIFORM1IVPROC)               getprocaddress("glUniform1iv");
    glUniform2iv_ =               (PFNGLUNIFORM2IVPROC)               getprocaddress("glUniform2iv");
    glUniform3iv_ =               (PFNGLUNIFORM3IVPROC)               getprocaddress("glUniform3iv");
    glUniform4iv_ =               (PFNGLUNIFORM4IVPROC)               getprocaddress("glUniform4iv");
    glUniformMatrix2fv_ =         (PFNGLUNIFORMMATRIX2FVPROC)         getprocaddress("glUniformMatrix2fv");
    glUniformMatrix3fv_ =         (PFNGLUNIFORMMATRIX3FVPROC)         getprocaddress("glUniformMatrix3fv");
    glUniformMatrix4fv_ =         (PFNGLUNIFORMMATRIX4FVPROC)         getprocaddress("glUniformMatrix4fv");
    glBindAttribLocation_ =       (PFNGLBINDATTRIBLOCATIONPROC)       getprocaddress("glBindAttribLocation");
    glGetActiveUniform_ =         (PFNGLGETACTIVEUNIFORMPROC)         getprocaddress("glGetActiveUniform");
    glEnableVertexAttribArray_ =  (PFNGLENABLEVERTEXATTRIBARRAYPROC)  getprocaddress("glEnableVertexAttribArray");
    glDisableVertexAttribArray_ = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) getprocaddress("glDisableVertexAttribArray");

    glVertexAttrib1f_ =           (PFNGLVERTEXATTRIB1FPROC)           getprocaddress("glVertexAttrib1f");
    glVertexAttrib1fv_ =          (PFNGLVERTEXATTRIB1FVPROC)          getprocaddress("glVertexAttrib1fv");
    glVertexAttrib1s_ =           (PFNGLVERTEXATTRIB1SPROC)           getprocaddress("glVertexAttrib1s");
    glVertexAttrib1sv_ =          (PFNGLVERTEXATTRIB1SVPROC)          getprocaddress("glVertexAttrib1sv");
    glVertexAttrib2f_ =           (PFNGLVERTEXATTRIB2FPROC)           getprocaddress("glVertexAttrib2f");
    glVertexAttrib2fv_ =          (PFNGLVERTEXATTRIB2FVPROC)          getprocaddress("glVertexAttrib2fv");
    glVertexAttrib2s_ =           (PFNGLVERTEXATTRIB2SPROC)           getprocaddress("glVertexAttrib2s");
    glVertexAttrib2sv_ =          (PFNGLVERTEXATTRIB2SVPROC)          getprocaddress("glVertexAttrib2sv");
    glVertexAttrib3f_ =           (PFNGLVERTEXATTRIB3FPROC)           getprocaddress("glVertexAttrib3f");
    glVertexAttrib3fv_ =          (PFNGLVERTEXATTRIB3FVPROC)          getprocaddress("glVertexAttrib3fv");
    glVertexAttrib3s_ =           (PFNGLVERTEXATTRIB3SPROC)           getprocaddress("glVertexAttrib3s");
    glVertexAttrib3sv_ =          (PFNGLVERTEXATTRIB3SVPROC)          getprocaddress("glVertexAttrib3sv");
    glVertexAttrib4f_ =           (PFNGLVERTEXATTRIB4FPROC)           getprocaddress("glVertexAttrib4f");
    glVertexAttrib4fv_ =          (PFNGLVERTEXATTRIB4FVPROC)          getprocaddress("glVertexAttrib4fv");
    glVertexAttrib4s_ =           (PFNGLVERTEXATTRIB4SPROC)           getprocaddress("glVertexAttrib4s");
    glVertexAttrib4sv_ =          (PFNGLVERTEXATTRIB4SVPROC)          getprocaddress("glVertexAttrib4sv");
    glVertexAttrib4bv_ =          (PFNGLVERTEXATTRIB4BVPROC)          getprocaddress("glVertexAttrib4bv");
    glVertexAttrib4iv_ =          (PFNGLVERTEXATTRIB4IVPROC)          getprocaddress("glVertexAttrib4iv");
    glVertexAttrib4ubv_ =         (PFNGLVERTEXATTRIB4UBVPROC)         getprocaddress("glVertexAttrib4ubv");
    glVertexAttrib4uiv_ =         (PFNGLVERTEXATTRIB4UIVPROC)         getprocaddress("glVertexAttrib4uiv");
    glVertexAttrib4usv_ =         (PFNGLVERTEXATTRIB4USVPROC)         getprocaddress("glVertexAttrib4usv");
    glVertexAttrib4Nbv_ =         (PFNGLVERTEXATTRIB4NBVPROC)         getprocaddress("glVertexAttrib4Nbv");
    glVertexAttrib4Niv_ =         (PFNGLVERTEXATTRIB4NIVPROC)         getprocaddress("glVertexAttrib4Niv");
    glVertexAttrib4Nub_ =         (PFNGLVERTEXATTRIB4NUBPROC)         getprocaddress("glVertexAttrib4Nub");
    glVertexAttrib4Nubv_ =        (PFNGLVERTEXATTRIB4NUBVPROC)        getprocaddress("glVertexAttrib4Nubv");
    glVertexAttrib4Nuiv_ =        (PFNGLVERTEXATTRIB4NUIVPROC)        getprocaddress("glVertexAttrib4Nuiv");
    glVertexAttrib4Nusv_ =        (PFNGLVERTEXATTRIB4NUSVPROC)        getprocaddress("glVertexAttrib4Nusv");
    glVertexAttribPointer_ =      (PFNGLVERTEXATTRIBPOINTERPROC)      getprocaddress("glVertexAttribPointer");

    glDrawBuffers_ =              (PFNGLDRAWBUFFERSPROC)              getprocaddress("glDrawBuffers");
#endif

    // glGetStringi/glBindFragDataLocation are desktop-GL-3.0-core-profile
    // features with no GLES equivalent (see parseglexts()'s comment) --
    // don't bother looking them up on Android, they'd just resolve to
    // NULL anyway, and leaving them NULL is exactly what their (already
    // __ANDROID__-guarded) call sites expect.
#ifndef __ANDROID__
    if(glversion >= 300)
    {
        glGetStringi_ =            (PFNGLGETSTRINGIPROC)          getprocaddress("glGetStringi");
        glBindFragDataLocation_ =  (PFNGLBINDFRAGDATALOCATIONPROC)getprocaddress("glBindFragDataLocation");
    }
#endif

    const char *glslstr = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    uint glslmajorversion, glslminorversion;
    if(findglversion(glslstr, glslmajorversion, glslminorversion)) glslversion = glslmajorversion*100 + glslminorversion;

    if(glslversion < 120) fatal("GLSL 1.20 or greater is required!");

    parseglexts();

    GLint val;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
    hwtexsize = val;
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &val);
    hwcubetexsize = val;

    if(glversion >= 300 || hasext("GL_ARB_texture_float") || hasext("GL_ATI_texture_float"))
    {
        hasTF = true;
        if(glversion < 300 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_float extension.");
        shadowmap = 1;
        extern int smoothshadowmappeel;
        smoothshadowmappeel = 1;
    }

    if(glversion >= 300 || hasext("GL_ARB_texture_rg"))
    {
        hasTRG = true;
        if(glversion < 300 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_rg extension.");
    }

    if(glversion >= 300 || hasext("GL_ARB_framebuffer_object"))
    {
        glBindRenderbuffer_        = (PFNGLBINDRENDERBUFFERPROC)       getprocaddress("glBindRenderbuffer");
        glDeleteRenderbuffers_     = (PFNGLDELETERENDERBUFFERSPROC)    getprocaddress("glDeleteRenderbuffers");
        glGenRenderbuffers_        = (PFNGLGENFRAMEBUFFERSPROC)        getprocaddress("glGenRenderbuffers");
        glRenderbufferStorage_     = (PFNGLRENDERBUFFERSTORAGEPROC)    getprocaddress("glRenderbufferStorage");
        glCheckFramebufferStatus_  = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) getprocaddress("glCheckFramebufferStatus");
        glBindFramebuffer_         = (PFNGLBINDFRAMEBUFFERPROC)        getprocaddress("glBindFramebuffer");
        glDeleteFramebuffers_      = (PFNGLDELETEFRAMEBUFFERSPROC)     getprocaddress("glDeleteFramebuffers");
        glGenFramebuffers_         = (PFNGLGENFRAMEBUFFERSPROC)        getprocaddress("glGenFramebuffers");
        glFramebufferTexture2D_    = (PFNGLFRAMEBUFFERTEXTURE2DPROC)   getprocaddress("glFramebufferTexture2D");
        glFramebufferRenderbuffer_ = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)getprocaddress("glFramebufferRenderbuffer");
        glGenerateMipmap_          = (PFNGLGENERATEMIPMAPPROC)         getprocaddress("glGenerateMipmap");
        glBlitFramebuffer_         = (PFNGLBLITFRAMEBUFFERPROC)        getprocaddress("glBlitFramebuffer");
        hasAFBO = hasFBO = hasFBB = hasDS = true;
        if(glversion < 300 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_framebuffer_object extension.");
    }
    else if(hasext("GL_EXT_framebuffer_object"))
    {
        glBindRenderbuffer_        = (PFNGLBINDRENDERBUFFERPROC)       getprocaddress("glBindRenderbufferEXT");
        glDeleteRenderbuffers_     = (PFNGLDELETERENDERBUFFERSPROC)    getprocaddress("glDeleteRenderbuffersEXT");
        glGenRenderbuffers_        = (PFNGLGENFRAMEBUFFERSPROC)        getprocaddress("glGenRenderbuffersEXT");
        glRenderbufferStorage_     = (PFNGLRENDERBUFFERSTORAGEPROC)    getprocaddress("glRenderbufferStorageEXT");
        glCheckFramebufferStatus_  = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) getprocaddress("glCheckFramebufferStatusEXT");
        glBindFramebuffer_         = (PFNGLBINDFRAMEBUFFERPROC)        getprocaddress("glBindFramebufferEXT");
        glDeleteFramebuffers_      = (PFNGLDELETEFRAMEBUFFERSPROC)     getprocaddress("glDeleteFramebuffersEXT");
        glGenFramebuffers_         = (PFNGLGENFRAMEBUFFERSPROC)        getprocaddress("glGenFramebuffersEXT");
        glFramebufferTexture2D_    = (PFNGLFRAMEBUFFERTEXTURE2DPROC)   getprocaddress("glFramebufferTexture2DEXT");
        glFramebufferRenderbuffer_ = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)getprocaddress("glFramebufferRenderbufferEXT");
        glGenerateMipmap_          = (PFNGLGENERATEMIPMAPPROC)         getprocaddress("glGenerateMipmapEXT");
        hasFBO = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_framebuffer_object extension.");

        if(hasext("GL_EXT_framebuffer_blit"))
        {
            glBlitFramebuffer_     = (PFNGLBLITFRAMEBUFFERPROC)        getprocaddress("glBlitFramebufferEXT");
            hasFBB = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_framebuffer_blit extension.");
        }

        if(hasext("GL_EXT_packed_depth_stencil") || hasext("GL_NV_packed_depth_stencil"))
        {
            hasDS = true;
            if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_packed_depth_stencil extension.");
        }
    }
    else fatal("Framebuffer object support is required!");

    extern int fpdepthfx;
    if(ati)
    {
        //conoutf(CON_WARN, "WARNING: ATI cards may show garbage in skybox. (use \"/ati_skybox_bug 1\" to fix)");

        minimizetcusage = 1;
        if(hasTF && hasTRG) fpdepthfx = 1;
        // On Catalyst 10.2, issuing an occlusion query on the first draw using a given cubemap texture causes a nasty crash
        ati_cubemap_bug = 1;
    }
    else if(nvidia)
    {
        reservevpparams = 10;
        rtsharefb = 0; // work-around for strange driver stalls involving when using many FBOs
        extern int filltjoints;
        if(glversion < 300 && !hasext("GL_EXT_gpu_shader4")) filltjoints = 0; // DX9 or less NV cards seem to not cause many sparklies

        if(hasTF && hasTRG) fpdepthfx = 1;
    }
    else
    {
        if(intel)
        {
#ifdef WIN32
            intel_vertexarray_bug = 1;
            // MapBufferRange is buggy on older Intel drivers on Windows
            if(glversion <= 310) intel_mapbufferrange_bug = 1;
#endif
        }

        reservevpparams = 20;

        if(mesa) mesa_swap_bug = 1;
    }

#ifdef __ANDROID__
    // glemu.cpp's end() streams 2D UI/HUD geometry (menu text, icons, ...)
    // into a persistently-mapped ring-buffer VBO via glMapBufferRange with
    // GL_MAP_UNSYNCHRONIZED_BIT, relying on the driver to honor that
    // contract (never let the GPU read a region the CPU is concurrently
    // overwriting). Confirmed on-device (Quest 3, Adreno 740): this
	// produces a consistent, reproducible ghosting artifact -- every
    // glyph/icon shows a second, slightly offset copy -- i.e. exactly what
    // stale-vs-fresh data racing in that ring buffer looks like. Reusing
    // intel_mapbufferrange_bug (originally an older-Intel-driver-on-
    // Windows workaround, see the WIN32 branch above) sidesteps the same
    // fast path here: glemu.cpp's end() falls back to glBufferSubData_
    // instead, which re-uploads the whole range synchronously each call
    // and has no such race.
    intel_mapbufferrange_bug = 1;
#endif

    if(glversion >= 300 || hasext("GL_ARB_map_buffer_range"))
    {
        glMapBufferRange_         = (PFNGLMAPBUFFERRANGEPROC)        getprocaddress("glMapBufferRange");
        glFlushMappedBufferRange_ = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)getprocaddress("glFlushMappedBufferRange");
        hasMBR = true;
        if(glversion < 300 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_map_buffer_range.");
    }

    if(glversion >= 310 || hasext("GL_ARB_uniform_buffer_object"))
    {
        glGetUniformIndices_       = (PFNGLGETUNIFORMINDICESPROC)      getprocaddress("glGetUniformIndices");
        glGetActiveUniformsiv_     = (PFNGLGETACTIVEUNIFORMSIVPROC)    getprocaddress("glGetActiveUniformsiv");
        glGetUniformBlockIndex_    = (PFNGLGETUNIFORMBLOCKINDEXPROC)   getprocaddress("glGetUniformBlockIndex");
        glGetActiveUniformBlockiv_ = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)getprocaddress("glGetActiveUniformBlockiv");
        glUniformBlockBinding_     = (PFNGLUNIFORMBLOCKBINDINGPROC)    getprocaddress("glUniformBlockBinding");
        glBindBufferBase_          = (PFNGLBINDBUFFERBASEPROC)         getprocaddress("glBindBufferBase");
        glBindBufferRange_         = (PFNGLBINDBUFFERRANGEPROC)        getprocaddress("glBindBufferRange");

        useubo = 1;
        hasUBO = true;
        if(glversion < 310 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_uniform_buffer_object extension.");
    }

    if(glversion >= 300 || hasext("GL_ARB_vertex_array_object"))
    {
        glBindVertexArray_ =    (PFNGLBINDVERTEXARRAYPROC)   getprocaddress("glBindVertexArray");
        glDeleteVertexArrays_ = (PFNGLDELETEVERTEXARRAYSPROC)getprocaddress("glDeleteVertexArrays");
        glGenVertexArrays_ =    (PFNGLGENVERTEXARRAYSPROC)   getprocaddress("glGenVertexArrays");
        glIsVertexArray_ =      (PFNGLISVERTEXARRAYPROC)     getprocaddress("glIsVertexArray");
        hasVAO = true;
        if(glversion < 300 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_vertex_array_object extension.");
    }
    else if(hasext("GL_APPLE_vertex_array_object"))
    {
        glBindVertexArray_ =    (PFNGLBINDVERTEXARRAYPROC)   getprocaddress("glBindVertexArrayAPPLE");
        glDeleteVertexArrays_ = (PFNGLDELETEVERTEXARRAYSPROC)getprocaddress("glDeleteVertexArraysAPPLE");
        glGenVertexArrays_ =    (PFNGLGENVERTEXARRAYSPROC)   getprocaddress("glGenVertexArraysAPPLE");
        glIsVertexArray_ =      (PFNGLISVERTEXARRAYPROC)     getprocaddress("glIsVertexArrayAPPLE");
        hasVAO = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_APPLE_vertex_array_object extension.");
    }

    if(glversion >= 330 || hasext("GL_ARB_texture_swizzle") || hasext("GL_EXT_texture_swizzle"))
    {
        hasTSW = true;
        if(glversion < 330 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_swizzle extension.");
    }

    if(hasext("GL_EXT_texture_compression_s3tc"))
    {
        hasS3TC = true;
#ifdef __APPLE__
        usetexcompress = 1;
#else
        if(!mesa) usetexcompress = 2;
#endif
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_texture_compression_s3tc extension.");
    }
    else if(hasext("GL_EXT_texture_compression_dxt1") && hasext("GL_ANGLE_texture_compression_dxt3") && hasext("GL_ANGLE_texture_compression_dxt5"))
    {
        hasS3TC = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_texture_compression_dxt1 extension.");
    }
    if(hasext("GL_3DFX_texture_compression_FXT1"))
    {
        hasFXT1 = true;
        if(mesa) usetexcompress = max(usetexcompress, 1);
        if(dbgexts) conoutf(CON_INIT, "Using GL_3DFX_texture_compression_FXT1.");
    }
    if(hasext("GL_EXT_texture_compression_latc"))
    {
        hasLATC = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_texture_compression_latc extension.");
    }
    if(glversion >= 300 || hasext("GL_ARB_texture_compression_rgtc") || hasext("GL_EXT_texture_compression_rgtc"))
    {
        hasRGTC = true;
        if(glversion < 300 && dbgexts) conoutf(CON_INIT, "Using GL_ARB_texture_compression_rgtc extension.");
    }

    if(hasext("GL_EXT_texture_filter_anisotropic"))
    {
        GLint val;
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &val);
        hwmaxaniso = val;
        hasAF = true;
        if(dbgexts) conoutf(CON_INIT, "Using GL_EXT_texture_filter_anisotropic extension.");
    }

    if(glversion >= 300 || hasext("GL_EXT_gpu_shader4"))
    {
        // on DX10 or above class cards (i.e. GF8 or RadeonHD) enable expensive features
        extern int grass, glare, maxdynlights, depthfxsize, blurdepthfx, texcompress;
        grass = 1;
        waterfallrefract = 1;
        glare = 1;
        maxdynlights = MAXDYNLIGHTS;
        depthfxsize = 10;
        blurdepthfx = 0;
        texcompress = max(texcompress, 1024 + 1);
    }
}

void glext(char *ext)
{
    intret(hasext(ext) ? 1 : 0);
}
COMMAND(glext, "s");

void gl_resize()
{
    glViewport(0, 0, screenw, screenh);
}

void gl_init()
{
    glClearColor(0, 0, 0, 0);
    glClearDepth(1);
    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_LINE_SMOOTH);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);

    gle::setup();

    setupshaders();

    setuptexcompress();

    gl_resize();
}

VAR(wireframe, 0, 0, 1);

ICOMMAND(getcamyaw, "", (), floatret(camera1->yaw));
ICOMMAND(getcampitch, "", (), floatret(camera1->pitch));
ICOMMAND(getcamroll, "", (), floatret(camera1->roll));
ICOMMAND(getcampos, "", (),
{
    defformatstring(pos, "%s %s %s", floatstr(camera1->o.x), floatstr(camera1->o.y), floatstr(camera1->o.z));
    result(pos);
});

vec worldpos, camdir, camright, camup;

#ifdef __ANDROID__
// Per-eye pose/FOV for the eye about to be rendered, set by
// android_sauer_set_eye() (called from vr_glue/sauerquest_vr_bootstrap.c's
// per-eye loop) immediately before android_sauer_drawframe(). See the
// project plan's stereo-rendering phase and androidbridge.h's comment on
// android_sauer_set_eye() for the exact units/convention of each field.
static vec androidEyeOffsetMeters(0, 0, 0);
static float androidEyeYaw = 0, androidEyePitch = 0, androidEyeRoll = 0;
static float androidEyeTanL = -1, androidEyeTanR = 1, androidEyeTanU = 1, androidEyeTanD = -1;

// units-per-meter converting OpenXR's real-world tracked motion into
// Sauerbraten world units -- rough estimate from the default player
// capsule (ents.h: eyeheight=14, radius=4.1 units, plausible for a
// ~1.6-1.7m/~0.3-0.4m human), exposed as a cvar since getting this exactly
// right needs on-device tuning ("does leaning a real 10cm forward move
// the view a sensible amount"), not something derivable purely from
// source. Was 10 -- confirmed on-device as slightly too large: a
// too-large scale exaggerates the per-eye IPD offset applied in
// setcammatrix() below beyond the real IPD's true equivalent in world
// units, which shows up specifically as near objects (including the
// viewmodel) being harder to stereo-fuse ("slightly doubled") while
// distant objects look fine, since a fixed angular/scale error matters
// far more for close parallax than far. 8 (VARP is integer-only) is
// closer to the eyeheight-based estimate above (14 units / ~1.65m ~=
// 8.5) than 10 was.
VAR(vrworldscale, 1, 8, 1000);

void android_sauer_set_eye(float dx, float dy, float dz,
                            float yaw, float pitch, float roll,
                            float tanLeft, float tanRight, float tanUp, float tanDown)
{
    androidEyeOffsetMeters = vec(dx, dy, dz);
    androidEyeYaw = yaw;
    androidEyePitch = pitch;
    androidEyeRoll = roll;
    androidEyeTanL = tanLeft;
    androidEyeTanR = tanRight;
    androidEyeTanU = tanUp;
    androidEyeTanD = tanDown;
}

// Snap-turn locomotion's persistent rotation, added on top of raw head
// yaw (androidEyeYaw) wherever camera1->yaw gets set below -- the
// body-yaw/head-yaw split noted as still-pending in earlier phases (no
// locomotion existed yet then). A snap turn doesn't move the real head
// at all, so it has to live in a value that isn't overwritten every
// frame by fresh head tracking, unlike androidEyeYaw itself.
static float androidBodyYaw = 0;

void android_sauer_snap_turn(float degrees)
{
    androidBodyYaw += degrees;
}

// See iengine.h for the full explanation. androidEyeYaw is this frame's
// already-known raw head yaw (set earlier this same frame by
// android_sauer_set_eye()), so solving androidBodyYaw = spawnyaw -
// androidEyeYaw makes their sum (used everywhere camera1->yaw gets set,
// see setcammatrix() below) equal spawnyaw immediately, without waiting
// for a snap-turn or any further head movement.
void android_sauer_align_spawn_yaw(float spawnyaw)
{
    androidBodyYaw = spawnyaw - androidEyeYaw;
}

// Right controller's current aim pose, set once per frame by
// android_sauer_set_aim() (sauerquest_vr_bootstrap.c) -- decouples
// weapon aiming/shooting from head direction (the confirmed scope for
// this port). Same head-relative-offset-in-meters convention as
// androidEyeOffsetMeters above.
static vec androidAimOffsetMeters(0, 0, 0);
static float androidAimYaw = 0, androidAimPitch = 0, androidAimRoll = 0;
static bool androidAimActive = false;

void android_sauer_set_aim(float dx, float dy, float dz,
                            float yaw, float pitch, float roll, int active)
{
    androidAimOffsetMeters = vec(dx, dy, dz);
    androidAimYaw = yaw;
    androidAimPitch = pitch;
    androidAimRoll = roll;
    androidAimActive = active != 0;
}

// Resolves the controller's head-relative offset into a world-space
// position/direction, exactly the same "rotate a head-local offset by
// camera1's current yaw/pitch/roll, then scale by vrworldscale and add
// to camera1->o" technique setcammatrix() uses for the per-eye render
// position a few lines down -- kept as its own function (rather than
// reusing cammatrix directly) since callers of this (drawhudmodel() in
// fpsgame/render.cpp, via iengine.h) run well after cammatrix has
// translation baked into it, not just rotation. Returns false (leaving
// pos/dir/yaw/pitch untouched) on desktop or when the controller isn't
// currently tracked, so callers can fall back to head-aim unconditionally.
bool androidgetaim(vec &pos, vec &dir, float &yaw, float &pitch)
{
    if(!androidAimActive) return false;
    matrix4 headrot;
    headrot.identity();
    headrot.rotate_around_y(camera1->roll*RAD);
    headrot.rotate_around_x(camera1->pitch*-RAD);
    headrot.rotate_around_z(camera1->yaw*-RAD);
    vec offsetWorld;
    headrot.transposedtransformnormal(androidAimOffsetMeters, offsetWorld);
    pos = vec(camera1->o).add(offsetWorld.mul(float(vrworldscale)));
    // androidAimYaw is the controller's own raw tracked orientation
    // (real-world/stage space) -- unlike camera1->yaw, it has no idea
    // snap-turn (androidBodyYaw) has virtually rotated the world, so it
    // needs the same offset added here or the gun/arm visibly stops
    // turning with everything else on a snap turn (confirmed on-device).
    yaw = androidAimYaw + androidBodyYaw;
    pitch = androidAimPitch;
    vecfromyawpitch(yaw, pitch, 1, 0, dir);
    return true;
}
#else
// Desktop stub -- fpsgame/render.cpp's drawhudmodel() calls this
// unconditionally (see iengine.h), always falling back to head-aim here.
bool androidgetaim(vec &pos, vec &dir, float &yaw, float &pitch) { return false; }

// Desktop stub -- fpsgame/fps.cpp's spawnplayer() calls this
// unconditionally (see iengine.h); a no-op here since head yaw already
// *is* d->yaw on desktop, by construction of mouse-look.
void android_sauer_align_spawn_yaw(float spawnyaw) {}
#endif

void setcammatrix()
{
    // move from RH to Z-up LH quake style worldspace
    cammatrix = viewmatrix;
#ifdef __ANDROID__
    // drawtex != DRAWTEX_NONE means this call is for one of the
    // render-to-texture passes (envmap/minimap/modelpreview) that
    // temporarily repoint camera1 at their own throwaway physent with a
    // fixed pose (e.g. modelpreview::start(): camera.o=(0,0,0), yaw=0,
    // pitch=modelpreviewpitch, roll=0 -- the spinning cube-badge/model
    // icon on the main menu and elsewhere). Applying the head-tracked
    // override in that case hijacks their intentionally-fixed camera
    // with the real head pose instead -- confirmed on-device as the
    // actual cause of a reported "way too much difference between the
    // eyes" (the badge's orientation/position was swinging with head
    // yaw/eye offset instead of staying put), not a stereo-math bug in
    // the per-eye offset itself. Fall through to the normal
    // camera1-driven path below for these, exactly like desktop.
    if(!drawtex)
    {
    // Head orientation drives the view directly (absolute, not the
    // mouse-delta accumulation mousemove() normally does -- see
    // main.cpp's checkinput(), never called on Android since there's no
    // SDL window generating mouse-motion events to begin with). Also
    // mirror it onto camera1 itself so other code that reads
    // camera1->yaw/pitch/roll directly (not just this function) sees the
    // same head-tracked values -- e.g. the crosshair raycast a few lines
    // down, and drawhudmodel()'s viewmodel orientation, until Phase 6
    // replaces both with the controller's own transform.
    // androidBodyYaw is snap-turn locomotion's persistent rotation (see
    // its own comment) -- added on top of the raw tracked head yaw here,
    // the one place camera1's actual facing direction gets established,
    // so every reader of camera1->yaw (aim, movement, this cammatrix
    // rotation itself) automatically sees turns applied consistently.
    float totalYaw = androidEyeYaw + androidBodyYaw;
    camera1->yaw = totalYaw;
    camera1->pitch = androidEyePitch;
    camera1->roll = androidEyeRoll;
    cammatrix.rotate_around_y(androidEyeRoll*RAD);
    cammatrix.rotate_around_x(androidEyePitch*-RAD);
    cammatrix.rotate_around_z(totalYaw*-RAD);
    // androidEyeOffsetMeters is head-relative (it rotates with wherever
    // the player is currently looking -- the IPD offset always points
    // "to the left/right of view", not to some fixed compass direction),
    // but camera1->o is a world-space position, so the offset must be
    // rotated into world space before being added to it -- otherwise, on
    // any frame where head yaw/pitch/roll isn't ~0, the offset partly
    // lands on the *depth* axis instead of staying purely lateral, which
    // (confirmed on-device: reported as "too much difference between the
    // eyes") throws off both eyes' effective distance from nearby
    // geometry, not just their apparent left-right separation.
    //
    // It must NOT be rotated by cammatrix, though, even though cammatrix
    // does hold the head rotation: cammatrix starts from viewmatrix (the
    // world->GL "quake style" axis swap at the top of this function), so
    // transposedtransformnormal() through it consumes a vector in GL
    // *view* axes -- +X right, +Y down, +Z forward, which is exactly
    // what the camdir/camright/camup lines below define it as.
    // android_sauer_set_eye()'s contract (androidbridge.h) is the
    // opposite: dx/dy/dz arrive already remapped into Sauerbraten's own
    // world axes (X lateral, Y forward, Z up). Passing world-axis
    // components through the view-axis transform ran the lateral
    // component through viewmatrix's own -1 on X and negated it, putting
    // each eye on the *other* eye's side -- reversed stereo. That still
    // fuses at a distance (parallax there is near zero) and fails
    // precisely where parallax is largest, which is what every remaining
    // "near things are doubled" report was. It was never an IPD/scale
    // error, so no amount of vrworldscale/hudgunvrdist tuning could have
    // fixed it. Rotating by a viewmatrix-free head rotation instead --
    // the same matrix androidgetaim() above builds for the controller
    // offset, which shares this identical world-axis contract and has
    // always been correct -- puts both offsets on one convention.
    matrix4 eyerot;
    eyerot.identity();
    eyerot.rotate_around_y(androidEyeRoll*RAD);
    eyerot.rotate_around_x(androidEyePitch*-RAD);
    eyerot.rotate_around_z(totalYaw*-RAD);
    vec eyeOffsetWorld;
    eyerot.transposedtransformnormal(androidEyeOffsetMeters, eyeOffsetWorld);
    // camera1->o is the player's logical/gameplay position (movement,
    // collision -- untouched here); the per-eye positional offset from
    // real head tracking is added only to the render-camera translation,
    // the same "body root + head-relative offset" split most VR rigs use.
    cammatrix.translate(vec(camera1->o).add(eyeOffsetWorld.mul(float(vrworldscale))).neg());
    }
    else
    {
        // drawtex != DRAWTEX_NONE: use whatever camera1 was temporarily
        // repointed to as-is, same as desktop -- see the comment above.
        cammatrix.rotate_around_y(camera1->roll*RAD);
        cammatrix.rotate_around_x(camera1->pitch*-RAD);
        cammatrix.rotate_around_z(camera1->yaw*-RAD);
        cammatrix.translate(vec(camera1->o).neg());
    }
#else
    cammatrix.rotate_around_y(camera1->roll*RAD);
    cammatrix.rotate_around_x(camera1->pitch*-RAD);
    cammatrix.rotate_around_z(camera1->yaw*-RAD);
    cammatrix.translate(vec(camera1->o).neg());
#endif

    cammatrix.transposedtransformnormal(vec(viewmatrix.b), camdir);
    cammatrix.transposedtransformnormal(vec(viewmatrix.a).neg(), camright);
    cammatrix.transposedtransformnormal(vec(viewmatrix.c), camup);

    if(!drawtex)
    {
        // The shoot/crosshair target: head-aimed by default, but the
        // right controller's own aim pose (decoupled from head
        // direction -- this port's confirmed scope for weapon aiming)
        // takes over when tracked. androidgetaim() leaves these
        // untouched (returning false) on desktop or when the controller
        // isn't currently tracked, so this is a single code path for
        // both platforms.
        vec aimorigin = camera1->o, aimdir = camdir;
        float aimyaw, aimpitch;
        androidgetaim(aimorigin, aimdir, aimyaw, aimpitch);
        if(raycubepos(aimorigin, aimdir, worldpos, 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
            worldpos = vec(aimdir).mul(2*worldsize).add(aimorigin); // if nothing is hit, just far away in the aim direction
    }
}

void setcamprojmatrix(bool init = true, bool flush = false)
{
    if(init)
    {
        setcammatrix();
    }

    camprojmatrix.muld(projmatrix, cammatrix);

    if(init)
    {
        invcammatrix.invert(cammatrix);
        invcamprojmatrix.invert(camprojmatrix);
    }

    GLOBALPARAM(camprojmatrix, camprojmatrix);

    if(fogging)
    {
        vec fogplane(cammatrix.c);
        fogplane.x /= projmatrix.a.x;
        fogplane.y /= projmatrix.b.y;
        fogplane.z /= projmatrix.c.w;
        GLOBALPARAMF(fogplane, fogplane.x, fogplane.y, 0, fogplane.z);
    }
    else
    {
        vec2 lineardepthscale = projmatrix.lineardepthscale();
        GLOBALPARAMF(fogplane, 0, 0, lineardepthscale.x, lineardepthscale.y);
    }

    if(flush && Shader::lastshader) Shader::lastshader->flushparams();
}

matrix4 hudmatrix, hudmatrixstack[64];
int hudmatrixpos = 0;

void resethudmatrix()
{
    hudmatrixpos = 0;
    GLOBALPARAM(hudmatrix, hudmatrix);
}

void pushhudmatrix()
{
    if(hudmatrixpos >= 0 && hudmatrixpos < int(sizeof(hudmatrixstack)/sizeof(hudmatrixstack[0]))) hudmatrixstack[hudmatrixpos] = hudmatrix;
    ++hudmatrixpos;
}

void flushhudmatrix(bool flushparams)
{
    GLOBALPARAM(hudmatrix, hudmatrix);
    if(flushparams && Shader::lastshader) Shader::lastshader->flushparams();
}

void pophudmatrix(bool flush, bool flushparams)
{
    --hudmatrixpos;
    if(hudmatrixpos >= 0 && hudmatrixpos < int(sizeof(hudmatrixstack)/sizeof(hudmatrixstack[0])))
    {
        hudmatrix = hudmatrixstack[hudmatrixpos];
        if(flush) flushhudmatrix(flushparams);
    }
}

void pushhudscale(float sx, float sy)
{
    if(!sy) sy = sx;
    pushhudmatrix();
    hudmatrix.scale(sx, sy, 1);
    flushhudmatrix();
}

void pushhudtranslate(float tx, float ty, float sx, float sy)
{
    if(!sy) sy = sx;
    pushhudmatrix();
    hudmatrix.translate(tx, ty, 0);
    if(sy) hudmatrix.scale(sx, sy, 1);
    flushhudmatrix();
}

float curfov = 100, curavatarfov = 65, fovy, aspect;
int farplane;
VARP(zoominvel, 0, 250, 5000);
VARP(zoomoutvel, 0, 100, 5000);
VARP(zoomfov, 10, 35, 60);
VARP(fov, 10, 100, 150);
VAR(avatarzoomfov, 10, 25, 60);
VAR(avatarfov, 10, 65, 150);
FVAR(avatardepth, 0, 0.5f, 1);
FVARNP(aspect, forceaspect, 0, 0, 1e3f);

static float zoomprogress = 0;
VAR(zoom, -1, 0, 1);

void disablezoom()
{
    zoom = 0;
    zoomprogress = 0;
}

void computezoom()
{
    if(!zoom) { zoomprogress = 0; curfov = fov; curavatarfov = avatarfov; return; }
    if(zoom > 0) zoomprogress = zoominvel ? min(zoomprogress + float(elapsedtime) / zoominvel, 1.0f) : 1;
    else
    {
        zoomprogress = zoomoutvel ? max(zoomprogress - float(elapsedtime) / zoomoutvel, 0.0f) : 0;
        if(zoomprogress <= 0) zoom = 0;
    }
    curfov = zoomfov*zoomprogress + fov*(1 - zoomprogress);
    curavatarfov = avatarzoomfov*zoomprogress + avatarfov*(1 - zoomprogress);
}

FVARP(zoomsens, 1e-3f, 1, 1000);
FVARP(zoomaccel, 0, 0, 1000);
VARP(zoomautosens, 0, 1, 1);
FVARP(sensitivity, 1e-3f, 3, 1000);
FVARP(sensitivityscale, 1e-3f, 1, 1000);
VARP(invmouse, 0, 0, 1);
FVARP(mouseaccel, 0, 0, 1000);

VAR(thirdperson, 0, 0, 2);
FVAR(thirdpersondistance, 0, 20, 50);
FVAR(thirdpersonup, -25, 0, 25);
FVAR(thirdpersonside, -25, 0, 25);
physent *camera1 = NULL;
bool detachedcamera = false;
bool isthirdperson() { return player!=camera1 || detachedcamera || reflecting; }

void fixcamerarange()
{
    const float MAXPITCH = 90.0f;
    if(camera1->pitch>MAXPITCH) camera1->pitch = MAXPITCH;
    if(camera1->pitch<-MAXPITCH) camera1->pitch = -MAXPITCH;
    while(camera1->yaw<0.0f) camera1->yaw += 360.0f;
    while(camera1->yaw>=360.0f) camera1->yaw -= 360.0f;
}

void mousemove(int dx, int dy)
{
    if(!game::allowmouselook()) return;
    float cursens = sensitivity, curaccel = mouseaccel;
    if(zoom)
    {
        if(zoomautosens)
        {
            cursens = float(sensitivity*zoomfov)/fov;
            curaccel = float(mouseaccel*zoomfov)/fov;
        }
        else
        {
            cursens = zoomsens;
            curaccel = zoomaccel;
        }
    }
    if(curaccel && curtime && (dx || dy)) cursens += curaccel * sqrtf(dx*dx + dy*dy)/curtime;
    cursens /= 33.0f*sensitivityscale;
    camera1->yaw += dx*cursens;
    camera1->pitch -= dy*cursens*(invmouse ? -1 : 1);
    fixcamerarange();
    if(camera1!=player && !detachedcamera)
    {
        player->yaw = camera1->yaw;
        player->pitch = camera1->pitch;
    }
}

void recomputecamera()
{
    game::setupcamera();
    computezoom();

    bool allowthirdperson = game::allowthirdperson();
    bool shoulddetach = (allowthirdperson && thirdperson > 1) || game::detachcamera();
    if((!allowthirdperson || !thirdperson) && !shoulddetach)
    {
        camera1 = player;
        detachedcamera = false;
    }
    else
    {
        static physent tempcamera;
        camera1 = &tempcamera;
        if(detachedcamera && shoulddetach) camera1->o = player->o;
        else
        {
            *camera1 = *player;
            detachedcamera = shoulddetach;
        }
        camera1->reset();
        camera1->type = ENT_CAMERA;
        camera1->move = -1;
        camera1->eyeheight = camera1->aboveeye = camera1->radius = camera1->xradius = camera1->yradius = 2;

        matrix3 orient;
        orient.identity();
        orient.rotate_around_z(camera1->yaw*RAD);
        orient.rotate_around_x(camera1->pitch*RAD);
        orient.rotate_around_y(camera1->roll*-RAD);
        vec dir = vec(orient.b).neg(), side = vec(orient.a).neg(), up = orient.c;

        if(game::collidecamera())
        {
            movecamera(camera1, dir, thirdpersondistance, 1);
            movecamera(camera1, dir, clamp(thirdpersondistance - camera1->o.dist(player->o), 0.0f, 1.0f), 0.1f);
            if(thirdpersonup)
            {
                vec pos = camera1->o;
                float dist = fabs(thirdpersonup);
                if(thirdpersonup < 0) up.neg();
                movecamera(camera1, up, dist, 1);
                movecamera(camera1, up, clamp(dist - camera1->o.dist(pos), 0.0f, 1.0f), 0.1f);
            }
            if(thirdpersonside)
            {
                vec pos = camera1->o;
                float dist = fabs(thirdpersonside);
                if(thirdpersonside < 0) side.neg();
                movecamera(camera1, side, dist, 1);
                movecamera(camera1, side, clamp(dist - camera1->o.dist(pos), 0.0f, 1.0f), 0.1f);
            }
        }
        else
        {
            camera1->o.add(vec(dir).mul(thirdpersondistance));
            if(thirdpersonup) camera1->o.add(vec(up).mul(thirdpersonup));
            if(thirdpersonside) camera1->o.add(vec(side).mul(thirdpersonside));
        }
    }

    setviewcell(camera1->o);
}

extern const matrix4 viewmatrix(vec(-1, 0, 0), vec(0, 0, 1), vec(0, -1, 0));
matrix4 cammatrix, projmatrix, camprojmatrix, invcammatrix, invcamprojmatrix;

FVAR(nearplane, 0.01f, 0.54f, 2.0f);

vec calcavatarpos(const vec &pos, float dist)
{
    vec eyepos;
    cammatrix.transform(pos, eyepos);
    GLdouble ydist = nearplane * tan(curavatarfov/2*RAD), xdist = ydist * aspect;
    vec4 scrpos;
    scrpos.x = eyepos.x*nearplane/xdist;
    scrpos.y = eyepos.y*nearplane/ydist;
    scrpos.z = (eyepos.z*(farplane + nearplane) - 2*nearplane*farplane) / (farplane - nearplane);
    scrpos.w = -eyepos.z;

    vec worldpos = invcamprojmatrix.perspectivetransform(scrpos);
    vec dir = vec(worldpos).sub(camera1->o).rescale(dist);
    return dir.add(camera1->o);
}

VAR(reflectclip, 0, 6, 64);
VAR(reflectclipavatar, -64, 0, 64);

matrix4 clipmatrix, noclipmatrix;

void renderavatar()
{
    if(isthirdperson()) return;

#ifdef __ANDROID__
    // Desktop swaps in a separate, narrower, symmetric perspective just
    // for the held weapon/viewmodel (curavatarfov/aspect, plus a Z-scale
    // squeeze via avatardepth) so it doesn't look stretched at a wide
    // desktop FOV, and so it doesn't clip through nearby walls. Neither
    // reason applies in VR -- there's no unnaturally-wide FOV to
    // compensate for (the headset's own per-eye FOV already matches real
    // optics) -- and critically, this projection is IDENTICAL for both
    // eyes, unlike the real per-eye asymmetric projmatrix
    // (androidEyeTanL/R/U/D-derived) everything else in the scene
    // renders through. That's the exact "flat/mismatched content warped
    // differently per eye by the compositor's own per-eye lens
    // correction" bug category already fixed several times elsewhere
    // this project (main menu, crosshair, gameplay HUD, pause menu) --
    // confirmed via a one-eye-closed-at-a-time test that this was indeed
    // the root cause of the long-standing "gun doubled" report. Skipping
    // the swap entirely lets the gun render through the same real
    // per-eye projmatrix as every other object, which already fuses
    // correctly for everything else in the game.
    game::renderavatar();
#else
    matrix4 oldprojmatrix = projmatrix;
    projmatrix.perspective(curavatarfov, aspect, nearplane, farplane);
    projmatrix.scalez(avatardepth);
    setcamprojmatrix(false);

    game::renderavatar();

    projmatrix = oldprojmatrix;
    setcamprojmatrix(false);
#endif
}

FVAR(polygonoffsetfactor, -1e4f, -3.0f, 1e4f);
FVAR(polygonoffsetunits, -1e4f, -3.0f, 1e4f);
FVAR(depthoffset, -1e4f, 0.01f, 1e4f);

matrix4 nooffsetmatrix;

void enablepolygonoffset(GLenum type, float scale)
{
    if(!depthoffset)
    {
        glPolygonOffset(polygonoffsetfactor * scale, polygonoffsetunits * scale);
        glEnable(type);
        return;
    }

    bool clipped = reflectz < 1e15f && reflectclip;

    nooffsetmatrix = projmatrix;
    projmatrix.d.z += depthoffset * scale * (clipped ? noclipmatrix.c.z : projmatrix.c.z);
    setcamprojmatrix(false, true);
}

void disablepolygonoffset(GLenum type)
{
    if(!depthoffset)
    {
        glDisable(type);
        return;
    }

    projmatrix = nooffsetmatrix;
    setcamprojmatrix(false, true);
}

void calcspherescissor(const vec &center, float size, float &sx1, float &sy1, float &sx2, float &sy2)
{
    vec worldpos(center), e;
    if(reflecting) worldpos.z = 2*reflectz - worldpos.z;
    cammatrix.transform(worldpos, e);
    if(e.z > 2*size) { sx1 = sy1 = 1; sx2 = sy2 = -1; return; }
    float zzrr = e.z*e.z - size*size,
          dx = e.x*e.x + zzrr, dy = e.y*e.y + zzrr,
          focaldist = 1.0f/tan(fovy*0.5f*RAD);
    sx1 = sy1 = -1;
    sx2 = sy2 = 1;
    #define CHECKPLANE(c, dir, focaldist, low, high) \
    do { \
        float nzc = (cz*cz + 1) / (cz dir drt) - cz, \
              pz = (d##c)/(nzc*e.c - e.z); \
        if(pz > 0) \
        { \
            float c = (focaldist)*nzc, \
                  pc = pz*nzc; \
            if(pc < e.c) low = c; \
            else if(pc > e.c) high = c; \
        } \
    } while(0)
    if(dx > 0)
    {
        float cz = e.x/e.z, drt = sqrtf(dx)/size;
        CHECKPLANE(x, -, focaldist/aspect, sx1, sx2);
        CHECKPLANE(x, +, focaldist/aspect, sx1, sx2);
    }
    if(dy > 0)
    {
        float cz = e.y/e.z, drt = sqrtf(dy)/size;
        CHECKPLANE(y, -, focaldist, sy1, sy2);
        CHECKPLANE(y, +, focaldist, sy1, sy2);
    }
}

static int scissoring = 0;
static GLint oldscissor[4];

int pushscissor(float sx1, float sy1, float sx2, float sy2)
{
    scissoring = 0;

    if(sx1 <= -1 && sy1 <= -1 && sx2 >= 1 && sy2 >= 1) return 0;

    sx1 = max(sx1, -1.0f);
    sy1 = max(sy1, -1.0f);
    sx2 = min(sx2, 1.0f);
    sy2 = min(sy2, 1.0f);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int sx = viewport[0] + int(floor((sx1+1)*0.5f*viewport[2])),
        sy = viewport[1] + int(floor((sy1+1)*0.5f*viewport[3])),
        sw = viewport[0] + int(ceil((sx2+1)*0.5f*viewport[2])) - sx,
        sh = viewport[1] + int(ceil((sy2+1)*0.5f*viewport[3])) - sy;
    if(sw <= 0 || sh <= 0) return 0;

    if(glIsEnabled(GL_SCISSOR_TEST))
    {
        glGetIntegerv(GL_SCISSOR_BOX, oldscissor);
        sw += sx;
        sh += sy;
        sx = max(sx, int(oldscissor[0]));
        sy = max(sy, int(oldscissor[1]));
        sw = min(sw, int(oldscissor[0] + oldscissor[2])) - sx;
        sh = min(sh, int(oldscissor[1] + oldscissor[3])) - sy;
        if(sw <= 0 || sh <= 0) return 0;
        scissoring = 2;
    }
    else scissoring = 1;

    glScissor(sx, sy, sw, sh);
    if(scissoring<=1) glEnable(GL_SCISSOR_TEST);

    return scissoring;
}

void popscissor()
{
    if(scissoring>1) glScissor(oldscissor[0], oldscissor[1], oldscissor[2], oldscissor[3]);
    else if(scissoring) glDisable(GL_SCISSOR_TEST);
    scissoring = 0;
}

static GLuint screenquadvbo = 0;

static void setupscreenquad()
{
    if(!screenquadvbo)
    {
        glGenBuffers_(1, &screenquadvbo);
        gle::bindvbo(screenquadvbo);
        vec2 verts[4] = { vec2(1, -1), vec2(-1, -1), vec2(1, 1), vec2(-1, 1) };
        glBufferData_(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        gle::clearvbo();
    }
}

static void cleanupscreenquad()
{
    if(screenquadvbo) { glDeleteBuffers_(1, &screenquadvbo); screenquadvbo = 0; }
}

void screenquad()
{
    setupscreenquad();
    gle::bindvbo(screenquadvbo);
    gle::enablevertex();
    gle::vertexpointer(sizeof(vec2), (const vec2 *)0, GL_FLOAT, 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gle::disablevertex();
    gle::clearvbo();
}

static LocalShaderParam screentexcoord[2] = { LocalShaderParam("screentexcoord0"), LocalShaderParam("screentexcoord1") };

static inline void setscreentexcoord(int i, float w, float h, float x = 0, float y = 0)
{
    screentexcoord[i].setf(w*0.5f, h*0.5f, x + w*0.5f, y + fabs(h)*0.5f);
}

void screenquad(float sw, float sh)
{
    setscreentexcoord(0, sw, sh);
    screenquad();
}

void screenquadflipped(float sw, float sh)
{
    setscreentexcoord(0, sw, -sh);
    screenquad();
}

void screenquad(float sw, float sh, float sw2, float sh2)
{
    setscreentexcoord(0, sw, sh);
    setscreentexcoord(1, sw2, sh2);
    screenquad();
}

void screenquadoffset(float x, float y, float w, float h)
{
    setscreentexcoord(0, w, h, x, y);
    screenquad();
}

void screenquadoffset(float x, float y, float w, float h, float x2, float y2, float w2, float h2)
{
    setscreentexcoord(0, w, h, x, y);
    setscreentexcoord(1, w2, h2, x2, y2);
    screenquad();
}

#define HUDQUAD(x1, y1, x2, y2, sx1, sy1, sx2, sy2) { \
    gle::defvertex(2); \
    gle::deftexcoord0(); \
    gle::begin(GL_TRIANGLE_STRIP); \
    gle::attribf(x2, y1); gle::attribf(sx2, sy1); \
    gle::attribf(x1, y1); gle::attribf(sx1, sy1); \
    gle::attribf(x2, y2); gle::attribf(sx2, sy2); \
    gle::attribf(x1, y2); gle::attribf(sx1, sy2); \
    gle::end(); \
}

void hudquad(float x, float y, float w, float h, float tx, float ty, float tw, float th)
{
    HUDQUAD(x, y, x+w, y+h, tx, ty, tx+tw, ty+th);
}

VARR(fog, 16, 4000, 1000024);
bvec fogcolor(0x80, 0x99, 0xB3);
HVARFR(fogcolour, 0, 0x8099B3, 0xFFFFFF,
{
    fogcolor = bvec((fogcolour>>16)&0xFF, (fogcolour>>8)&0xFF, fogcolour&0xFF);
});

static float findsurface(int fogmat, const vec &v, int &abovemat)
{
    fogmat &= MATF_VOLUME;
    ivec o(v), co;
    int csize;
    do
    {
        cube &c = lookupcube(o, 0, co, csize);
        int mat = c.material&MATF_VOLUME;
        if(mat != fogmat)
        {
            abovemat = isliquid(mat) ? c.material : MAT_AIR;
            return o.z;
        }
        o.z = co.z + csize;
    }
    while(o.z < worldsize);
    abovemat = MAT_AIR;
    return worldsize;
}

static void blendfog(int fogmat, float blend, float logblend, float &start, float &end, vec &fogc)
{
    switch(fogmat&MATF_VOLUME)
    {
        case MAT_WATER:
        {
            const bvec &wcol = getwatercolor(fogmat);
            int wfog = getwaterfog(fogmat);
            fogc.madd(wcol.tocolor(), blend);
            end += logblend*min(fog, max(wfog*4, 32));
            break;
        }

        case MAT_LAVA:
        {
            const bvec &lcol = getlavacolor(fogmat);
            int lfog = getlavafog(fogmat);
            fogc.madd(lcol.tocolor(), blend);
            end += logblend*min(fog, max(lfog*4, 32));
            break;
        }

        default:
            fogc.madd(fogcolor.tocolor(), blend);
            start += logblend*(fog+64)/8;
            end += logblend*fog;
            break;
    }
}

vec oldfogcolor(0, 0, 0), curfogcolor(0, 0, 0);
float oldfogstart = 0, oldfogend = 1000000, curfogstart = 0, curfogend = 1000000;

void setfogcolor(const vec &v)
{
    GLOBALPARAM(fogcolor, v);
}

void zerofogcolor()
{
    setfogcolor(vec(0, 0, 0));
}

void resetfogcolor()
{
    setfogcolor(curfogcolor);
}

void pushfogcolor(const vec &v)
{
    oldfogcolor = curfogcolor;
    curfogcolor = v;
    resetfogcolor();
}

void popfogcolor()
{
    curfogcolor = oldfogcolor;
    resetfogcolor();
}

void setfogdist(float start, float end)
{
    GLOBALPARAMF(fogparams, 1/(end - start), end/(end - start));
}

void clearfogdist()
{
    setfogdist(0, 1000000);
}

void resetfogdist()
{
    setfogdist(curfogstart, curfogend);
}

void pushfogdist(float start, float end)
{
    oldfogstart = curfogstart;
    oldfogend = curfogend;
    curfogstart = start;
    curfogend = end;
    resetfogdist();
}

void popfogdist()
{
    curfogstart = oldfogstart;
    curfogend = oldfogend;
    resetfogdist();
}

static void resetfog()
{
    resetfogcolor();
    resetfogdist();

    glClearColor(curfogcolor.r, curfogcolor.g, curfogcolor.b, 1.0f);
}

static void setfog(int fogmat, float below = 1, int abovemat = MAT_AIR)
{
    float logscale = 256, logblend = log(1 + (logscale - 1)*below) / log(logscale);

    curfogstart = curfogend = 0;
    curfogcolor = vec(0, 0, 0);
    blendfog(fogmat, below, logblend, curfogstart, curfogend, curfogcolor);
    if(below < 1) blendfog(abovemat, 1-below, 1-logblend, curfogstart, curfogend, curfogcolor);

    resetfog();
}

static void setnofog(const vec &color = vec(0, 0, 0))
{
    curfogstart = 0;
    curfogend = 1000000;
    curfogcolor = color;

    resetfog();
}

static void blendfogoverlay(int fogmat, float blend, vec &overlay)
{
    float maxc;
    switch(fogmat&MATF_VOLUME)
    {
        case MAT_WATER:
        {
            const bvec &wcol = getwatercolor(fogmat);
            maxc = max(wcol.r, max(wcol.g, wcol.b));
            overlay.madd(vec(wcol.r, wcol.g, wcol.b).div(min(32.0f + maxc*7.0f/8.0f, 255.0f)).max(0.4f), blend);
            break;
        }

        case MAT_LAVA:
        {
            const bvec &lcol = getlavacolor(fogmat);
            maxc = max(lcol.r, max(lcol.g, lcol.b));
            overlay.madd(vec(lcol.r, lcol.g, lcol.b).div(min(32.0f + maxc*7.0f/8.0f, 255.0f)).max(0.4f), blend);
            break;
        }

        default:
            overlay.add(blend);
            break;
    }
}

void drawfogoverlay(int fogmat, float fogblend, int abovemat)
{
    SETSHADER(fogoverlay);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    vec overlay(0, 0, 0);
    blendfogoverlay(fogmat, fogblend, overlay);
    blendfogoverlay(abovemat, 1-fogblend, overlay);

    gle::color(overlay);
    screenquad();

    glDisable(GL_BLEND);
}

bool renderedgame = false;

void rendergame(bool mainpass)
{
    game::rendergame(mainpass);
    if(!shadowmapping) renderedgame = true;
}

VARP(skyboxglare, 0, 1, 1);

void drawglare()
{
    glaring = true;
    refracting = -1;

    pushfogcolor(vec(0, 0, 0));

    glClearColor(0, 0, 0, 1);
    glClear((skyboxglare && !shouldclearskyboxglare() ? 0 : GL_COLOR_BUFFER_BIT) | GL_DEPTH_BUFFER_BIT);

    rendergeom();

    if(skyboxglare) drawskybox(farplane, false);

    renderreflectedmapmodels();
    rendergame();
    renderavatar();

    renderwater();
    rendermaterials();
    renderalphageom();
    renderparticles();

    popfogcolor();

    refracting = 0;
    glaring = false;
}

VARP(reflectmms, 0, 1, 1);
VARR(refractsky, 0, 0, 1);

matrix4 noreflectmatrix;

void drawreflection(float z, bool refract, int fogdepth, const bvec &col)
{
    reflectz = z < 0 ? 1e16f : z;
    reflecting = !refract;
    refracting = refract ? (z < 0 || camera1->o.z >= z ? -1 : 1) : 0;
    fading = waterrefract && waterfade && z>=0;
    fogging = refracting<0 && z>=0;
    refractfog = fogdepth;

    if(fogging)
    {
        pushfogdist(camera1->o.z - z, camera1->o.z - (z - max(refractfog, 1)));
        pushfogcolor(col.tocolor());
    }
    else
    {
        vec color(0, 0, 0);
        float start = 0, end = 0;
        blendfog(MAT_AIR, 1, 1, start, end, color);
        pushfogdist(start, end);
        pushfogcolor(color);
    }

    if(fading)
    {
        float scale = fogging ? -0.25f : 0.25f, offset = 2*fabs(scale) - scale*z;
        GLOBALPARAMF(waterfadeparams, scale, offset, -scale, offset + camera1->o.z*scale);
    }

    if(reflecting)
    {
        noreflectmatrix = cammatrix;
        cammatrix.reflectz(z);

        glFrontFace(GL_CCW);
    }

    if(reflectclip && z>=0)
    {
        float zoffset = reflectclip/4.0f, zclip;
        if(refracting<0)
        {
            zclip = z+zoffset;
            if(camera1->o.z<=zclip) zclip = z;
        }
        else
        {
            zclip = z-zoffset;
            if(camera1->o.z>=zclip && camera1->o.z<=z+4.0f) zclip = z;
            if(reflecting) zclip = 2*z - zclip;
        }
        plane clipplane;
        invcammatrix.transposedtransform(plane(0, 0, refracting>0 ? 1 : -1, refracting>0 ? -zclip : zclip), clipplane);
        clipmatrix.clip(clipplane, projmatrix);
        noclipmatrix = projmatrix;
        projmatrix = clipmatrix;
    }

    setcamprojmatrix(false, true);

    renderreflectedgeom(refracting<0 && z>=0 && caustics, fogging);

    if(reflecting || refracting>0 || (refracting<0 && refractsky) || z<0)
    {
        if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        if(reflectclip && z>=0)
        {
            projmatrix = noclipmatrix;
            setcamprojmatrix(false, true);
        }
        drawskybox(farplane, false);
        if(reflectclip && z>=0)
        {
            projmatrix = clipmatrix;
            setcamprojmatrix(false, true);
        }
        if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    }
    else if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

    renderdecals();

    if(reflectmms) renderreflectedmapmodels();
    rendergame();

    if(refracting && z>=0 && !isthirdperson() && fabs(camera1->o.z-z) <= 0.5f*(player->eyeheight + player->aboveeye))
    {
        matrix4 oldprojmatrix = projmatrix, avatarproj;
        avatarproj.perspective(curavatarfov, aspect, nearplane, farplane);
        if(reflectclip)
        {
            matrix4 avatarclip;
            plane clipplane;
            invcammatrix.transposedtransform(plane(0, 0, refracting, reflectclipavatar/4.0f - refracting*z), clipplane);
            avatarclip.clip(clipplane, avatarproj);
            projmatrix = avatarclip;
        }
        else projmatrix = avatarproj;
        setcamprojmatrix(false, true);
        game::renderavatar();
        projmatrix = oldprojmatrix;
        setcamprojmatrix(false, true);
    }

    if(refracting) rendergrass();
    rendermaterials();
    renderalphageom(fogging);
    renderparticles();

    if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    if(reflectclip && z>=0) projmatrix = noclipmatrix;

    if(reflecting)
    {
        cammatrix = noreflectmatrix;

        glFrontFace(GL_CW);
    }

    popfogdist();
    popfogcolor();

    reflectz = 1e16f;
    refracting = 0;
    reflecting = fading = fogging = false;

    setcamprojmatrix(false, true);
}

int drawtex = 0;

void drawcubemap(int size, const vec &o, float yaw, float pitch, const cubemapside &side, bool onlysky)
{
    drawtex = DRAWTEX_ENVMAP;

    physent *oldcamera = camera1;
    static physent cmcamera;
    cmcamera = *player;
    cmcamera.reset();
    cmcamera.type = ENT_CAMERA;
    cmcamera.o = o;
    cmcamera.yaw = yaw;
    cmcamera.pitch = pitch;
    cmcamera.roll = 0;
    camera1 = &cmcamera;
    setviewcell(camera1->o);

    int fogmat = lookupmaterial(o)&(MATF_VOLUME|MATF_INDEX);

    setfog(fogmat);

    int farplane = worldsize*2;

    projmatrix.perspective(90.0f, 1.0f, nearplane, farplane);
    if(!side.flipx || !side.flipy) projmatrix.scalexy(!side.flipx ? -1 : 1, !side.flipy ? -1 : 1);
    if(side.swapxy)
    {
        swap(projmatrix.a.x, projmatrix.a.y);
        swap(projmatrix.b.x, projmatrix.b.y);
        swap(projmatrix.c.x, projmatrix.c.y);
        swap(projmatrix.d.x, projmatrix.d.y);
    }
    setcamprojmatrix();

    xtravertsva = xtraverts = glde = gbatches = 0;

    visiblecubes();

    if(onlysky) drawskybox(farplane, false, true);
    else
    {
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        if(limitsky()) drawskybox(farplane, true);

        rendergeom();

        if(!limitsky()) drawskybox(farplane, false);

//      queryreflections();

        rendermapmodels();
        renderalphageom();

//      drawreflections();

//      renderwater();
//      rendermaterials();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
    }

    camera1 = oldcamera;
    drawtex = 0;
}

VAR(modelpreviewfov, 10, 20, 100);
VAR(modelpreviewpitch, -90, -15, 90);

namespace modelpreview
{
    physent *oldcamera;
    physent camera;

    float oldaspect, oldfovy, oldfov;
    int oldfarplane;
    matrix4 oldprojmatrix;

    void start(int x, int y, int w, int h, bool background)
    {
        drawtex = DRAWTEX_MODELPREVIEW;

        glViewport(x, y, w, h);
        glScissor(x, y, w, h);
        glEnable(GL_SCISSOR_TEST);

        oldcamera = camera1;
        camera = *camera1;
        camera.reset();
        camera.type = ENT_CAMERA;
        camera.o = vec(0, 0, 0);
        camera.yaw = 0;
        camera.pitch = modelpreviewpitch;
        camera.roll = 0;
        camera1 = &camera;

        oldaspect = aspect;
        oldfovy = fovy;
        oldfov = curfov;
        oldfarplane = farplane;
        oldprojmatrix = projmatrix;

        aspect = w/float(h);
        fovy = modelpreviewfov;
        curfov = 2*atan2(tan(fovy/2*RAD), 1/aspect)/RAD;
        farplane = 1024;

        clearfogdist();
        zerofogcolor();
        glClearColor(0, 0, 0, 1);

        glClear((background ? GL_COLOR_BUFFER_BIT : 0) | GL_DEPTH_BUFFER_BIT);

        projmatrix.perspective(fovy, aspect, nearplane, farplane);
        setcamprojmatrix();

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
    }

    void end()
    {
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        resetfogdist();
        resetfogcolor();
        glClearColor(curfogcolor.r, curfogcolor.g, curfogcolor.b, 1);

        aspect = oldaspect;
        fovy = oldfovy;
        curfov = oldfov;
        farplane = oldfarplane;

        camera1 = oldcamera;
        drawtex = 0;

        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, screenw, screenh);

        projmatrix = oldprojmatrix;
        setcamprojmatrix();
    }
}

vec calcmodelpreviewpos(const vec &radius, float &yaw)
{
    yaw = fmod(lastmillis/10000.0f*360.0f, 360.0f);
    float dist = 1.15f*max(radius.magnitude2()/aspect, radius.magnitude())/sinf(fovy/2*RAD);
    return vec(0, dist, 0).rotate_around_x(camera1->pitch*RAD);
}

GLuint minimaptex = 0;
vec minimapcenter(0, 0, 0), minimapradius(0, 0, 0), minimapscale(0, 0, 0);

void clearminimap()
{
    if(minimaptex) { glDeleteTextures(1, &minimaptex); minimaptex = 0; }
}

VARR(minimapheight, 0, 0, 2<<16);
bvec minimapcolor(0, 0, 0);
HVARFR(minimapcolour, 0, 0, 0xFFFFFF,
{
    minimapcolor = bvec((minimapcolour>>16)&0xFF, (minimapcolour>>8)&0xFF, minimapcolour&0xFF);
});
VARR(minimapclip, 0, 0, 1);
VARFP(minimapsize, 7, 8, 10, { if(minimaptex) drawminimap(); });

void bindminimap()
{
    glBindTexture(GL_TEXTURE_2D, minimaptex);
}

void clipminimap(ivec &bbmin, ivec &bbmax, cube *c = worldroot, const ivec &co = ivec(0, 0, 0), int size = worldsize>>1)
{
    loopi(8)
    {
        ivec o(i, co, size);
        if(c[i].children) clipminimap(bbmin, bbmax, c[i].children, o, size>>1);
        else if(!isentirelysolid(c[i]) && (c[i].material&MATF_CLIP)!=MAT_CLIP)
        {
            loopk(3) bbmin[k] = min(bbmin[k], o[k]);
            loopk(3) bbmax[k] = max(bbmax[k], o[k] + size);
        }
    }
}

void drawminimap()
{
    if(!game::needminimap()) { clearminimap(); return; }

    renderprogress(0, "generating mini-map...", 0, !renderedframe);

    int size = 1<<minimapsize, sizelimit = min(hwtexsize, min(screenw, screenh));
    while(size > sizelimit) size /= 2;
    if(!minimaptex) glGenTextures(1, &minimaptex);

    ivec bbmin(worldsize, worldsize, worldsize), bbmax(0, 0, 0);
    loopv(valist)
    {
        vtxarray *va = valist[i];
        loopk(3)
        {
            if(va->geommin[k]>va->geommax[k]) continue;
            bbmin[k] = min(bbmin[k], va->geommin[k]);
            bbmax[k] = max(bbmax[k], va->geommax[k]);
        }
    }
    if(minimapclip)
    {
        ivec clipmin(worldsize, worldsize, worldsize), clipmax(0, 0, 0);
        clipminimap(clipmin, clipmax);
        loopk(2) bbmin[k] = max(bbmin[k], clipmin[k]);
        loopk(2) bbmax[k] = min(bbmax[k], clipmax[k]);
    }

    minimapradius = vec(bbmax).sub(vec(bbmin)).mul(0.5f);
    minimapcenter = vec(bbmin).add(minimapradius);
    minimapradius.x = minimapradius.y = max(minimapradius.x, minimapradius.y);
    minimapscale = vec((0.5f - 1.0f/size)/minimapradius.x, (0.5f - 1.0f/size)/minimapradius.y, 1.0f);

    drawtex = DRAWTEX_MINIMAP;

    physent *oldcamera = camera1;
    static physent cmcamera;
    cmcamera = *player;
    cmcamera.reset();
    cmcamera.type = ENT_CAMERA;
    cmcamera.o = vec(minimapcenter.x, minimapcenter.y, max(minimapcenter.z + minimapradius.z + 1, float(minimapheight)));
    cmcamera.yaw = 0;
    cmcamera.pitch = -90;
    cmcamera.roll = 0;
    camera1 = &cmcamera;
    setviewcell(vec(-1, -1, -1));

    projmatrix.ortho(-minimapradius.x, minimapradius.x, -minimapradius.y, minimapradius.y, 0, camera1->o.z + 1);
    projmatrix.a.mul(-1);
    setcamprojmatrix();

    setnofog(minimapcolor.tocolor());

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, size, size);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glFrontFace(GL_CCW);

    xtravertsva = xtraverts = glde = gbatches = 0;

    visiblecubes(false);
    queryreflections();
    drawreflections();

    loopi(minimapheight > 0 && minimapheight < minimapcenter.z + minimapradius.z ? 2 : 1)
    {
        if(i)
        {
            glClear(GL_DEPTH_BUFFER_BIT);
            camera1->o.z = minimapheight;
            setcamprojmatrix();
        }
        rendergeom();
        rendermapmodels();
        renderwater();
        rendermaterials();
        renderalphageom();
    }

    glFrontFace(GL_CW);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glViewport(0, 0, screenw, screenh);

    camera1 = oldcamera;
    drawtex = 0;

    glBindTexture(GL_TEXTURE_2D, minimaptex);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5, 0, 0, size, size, 0);
    setuptexparameters(minimaptex, NULL, 3, 1, GL_RGB5, GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat border[4] = { minimapcolor.x/255.0f, minimapcolor.y/255.0f, minimapcolor.z/255.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool deferdrawtextures = false;

void drawtextures()
{
    if(minimized) { deferdrawtextures = true; return; }
    deferdrawtextures = false;
    genenvmaps();
    drawminimap();
}

GLuint motiontex = 0;
int motionw = 0, motionh = 0, lastmotion = 0;

void cleanupmotionblur()
{
    if(motiontex) { glDeleteTextures(1, &motiontex); motiontex = 0; }
    motionw = motionh = 0;
    lastmotion = 0;
}

VARFP(motionblur, 0, 0, 1, { if(!motionblur) cleanupmotionblur(); });
VARP(motionblurmillis, 1, 5, 1000);
FVARP(motionblurscale, 0, 0.5f, 1);

void addmotionblur()
{
    if(!motionblur || max(screenw, screenh) > hwtexsize) return;

    if(game::ispaused()) { lastmotion = 0; return; }

    if(!motiontex || motionw != screenw || motionh != screenh)
    {
        if(!motiontex) glGenTextures(1, &motiontex);
        motionw = screenw;
        motionh = screenh;
        lastmotion = 0;
        createtexture(motiontex, motionw, motionh, NULL, 3, 0, GL_RGB);
    }

    glBindTexture(GL_TEXTURE_2D, motiontex);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SETSHADER(screenrect);

    gle::colorf(1, 1, 1, lastmotion ? pow(motionblurscale, max(float(lastmillis - lastmotion)/motionblurmillis, 1.0f)) : 0);
    screenquad(1, 1);

    glDisable(GL_BLEND);

    if(lastmillis - lastmotion >= motionblurmillis)
    {
        lastmotion = lastmillis - lastmillis%motionblurmillis;

        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screenw, screenh);
    }
}

bool dopostfx = false;

void invalidatepostfx()
{
    dopostfx = false;
}

int xtraverts, xtravertsva;

void gl_drawframe()
{
    if(deferdrawtextures) drawtextures();

    updatedynlights();

    int w = screenw, h = screenh;
    aspect = forceaspect ? forceaspect : w/float(h);
    fovy = 2*atan2(tan(curfov/2*RAD), aspect)/RAD;

    int fogmat = lookupmaterial(camera1->o)&(MATF_VOLUME|MATF_INDEX), abovemat = MAT_AIR;
    float fogblend = 1.0f, causticspass = 0.0f;
    if(isliquid(fogmat&MATF_VOLUME))
    {
        float z = findsurface(fogmat, camera1->o, abovemat) - WATER_OFFSET;
        if(camera1->o.z < z + 1) fogblend = min(z + 1 - camera1->o.z, 1.0f);
        else fogmat = abovemat;
        if(caustics && (fogmat&MATF_VOLUME)==MAT_WATER && camera1->o.z < z)
            causticspass = min(z - camera1->o.z, 1.0f);
    }
    else fogmat = MAT_AIR;
    setfog(fogmat, fogblend, abovemat);
    if(fogmat!=MAT_AIR)
    {
        float blend = abovemat==MAT_AIR ? fogblend : 1.0f;
        fovy += blend*sinf(lastmillis/1000.0)*2.0f;
        aspect += blend*sinf(lastmillis/1000.0+M_PI)*0.1f;
    }

    farplane = worldsize*2;

#ifdef __ANDROID__
    // Real per-eye asymmetric FOV from xrLocateViews (android_sauer_set_eye(),
    // called once per eye just before this function) instead of the
    // symmetric fov-cvar-derived frustum desktop/mouse-look uses.
    projmatrix.frustum(nearplane*androidEyeTanL, nearplane*androidEyeTanR,
                        nearplane*androidEyeTanD, nearplane*androidEyeTanU,
                        nearplane, farplane);
#else
    projmatrix.perspective(fovy, aspect, nearplane, farplane);
#endif
    setcamprojmatrix();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    // Confirmed on-device via a temporary diagnostic (since removed):
    // GL_BLEND was already enabled at this exact point, before this
    // function does anything of its own -- leaked forward from whatever
    // UI/HUD blending the previous frame's gl_drawhud() left active
    // (exact leak site not isolated; gl_drawhud() and several effects
    // upstream of it enable/disable GL_BLEND in several places, any one
    // of which skipping its own disable on some code path would explain
    // it). With blending still on, opaque world geometry gets composited
    // against whatever's already in the framebuffer instead of fully
    // overwriting it -- reported as walls and other geometry being
    // partially see-through. Resetting explicitly here, rather than
    // continuing to hunt the exact leak through gl_drawhud()'s many
    // enable/disable pairs, guarantees correct state for world rendering
    // regardless of what leaked in.
    glDisable(GL_BLEND);

    xtravertsva = xtraverts = glde = gbatches = 0;

    visiblecubes();

    glClear(GL_DEPTH_BUFFER_BIT|(wireframe && editmode ? GL_COLOR_BUFFER_BIT : 0));

    if(wireframe && editmode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if(limitsky()) drawskybox(farplane, true);

    rendergeom(causticspass);

    extern int outline;
    if(!wireframe && editmode && outline) renderoutline();

    queryreflections();

    generategrass();

    if(!limitsky()) drawskybox(farplane, false);

    renderdecals(true);

    rendermapmodels();
    rendergame(true);
    renderavatar();

    if(wireframe && editmode) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    drawglaretex();
    drawdepthfxtex();
    drawreflections();

    if(wireframe && editmode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    renderwater();
    rendergrass();

    rendermaterials();
    renderalphageom();

    if(wireframe && editmode) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    renderparticles(true);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    addmotionblur();
    addglare();
    if(isliquid(fogmat&MATF_VOLUME)) drawfogoverlay(fogmat, fogblend, abovemat);
    renderpostfx();

    gl_drawhud();

    renderedgame = false;
}

#ifdef __ANDROID__
// Confirmed on-device: the main menu (using ordinary SDR assets, same
// brightness/color values as intended for a desktop monitor viewed from
// a normal distance in a lit room) reads as too bright/washed out
// through a VR lens a few centimeters from the eye in an otherwise dark
// headset -- a well-known perceptual difference when migrating desktop
// content to VR, not a rendering bug (ruled out: the background draws
// at plain full brightness with standard blending, no wrong colorspace,
// no double-layering). A flat darkening overlay drawn on top of
// everything is the standard comfort fix. Percentage of full black;
// tune live via /vrmenudim.
VAR(vrmenudim, 0, 75, 100);

// Same fix, separately tunable, for actual gameplay (gl_drawframe(), via
// gl_drawhud() below) -- confirmed on-device as the same perceptual
// effect, just less obvious against gameplay's typically darker/more
// detailed textures than the menu's bright flat background, until
// specifically looked for. Also folds in the "no dynamic lighting
// visible" report: with the whole scene overexposed, a dynamic light's
// actual contrast against its surroundings gets crushed toward white
// long before the light itself would -- this isn't lighting being
// disabled, it's a symptom of the same overbrightness. Tune live via
// /vrgamedim.
VAR(vrgamedim, 0, 20, 100);

static void drawvrdim(int amount, int w, int h)
{
    if(amount <= 0) return;
    hudnotextureshader->set();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gle::colorf(0, 0, 0, amount/100.0f);
    hudquad(0, 0, w, h);
    glDisable(GL_BLEND);
}
#endif

void gl_drawmainmenu()
{
    xtravertsva = xtraverts = glde = gbatches = 0;

    renderbackground(NULL, NULL, NULL, NULL, true, true);
    renderpostfx();

    gl_drawhud();

#ifdef __ANDROID__
    int w = screenw, h = screenh;
    if(forceaspect) w = int(ceil(h*forceaspect));
    hudmatrix.ortho(0, w, h, 0, -1, 1);
    resethudmatrix();
    drawvrdim(vrmenudim, w, h);
#endif
}

VARNP(damagecompass, usedamagecompass, 0, 1, 1);
VARP(damagecompassfade, 1, 1000, 10000);
VARP(damagecompasssize, 1, 30, 100);
VARP(damagecompassalpha, 1, 25, 100);
VARP(damagecompassmin, 1, 25, 1000);
VARP(damagecompassmax, 1, 200, 1000);

float damagedirs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

void damagecompass(int n, const vec &loc)
{
    if(!usedamagecompass || minimized) return;
    vec delta(loc);
    delta.sub(camera1->o);
    float yaw = 0, pitch;
    if(delta.magnitude() > 4)
    {
        vectoyawpitch(delta, yaw, pitch);
        yaw -= camera1->yaw;
    }
    if(yaw >= 360) yaw = fmod(yaw, 360);
    else if(yaw < 0) yaw = 360 - fmod(-yaw, 360);
    int dir = (int(yaw+22.5f)%360)/45;
    damagedirs[dir] += max(n, damagecompassmin)/float(damagecompassmax);
    if(damagedirs[dir]>1) damagedirs[dir] = 1;
}

void drawdamagecompass(int w, int h)
{
    hudnotextureshader->set();

    int dirs = 0;
    float size = damagecompasssize/100.0f*min(h, w)/2.0f;
    loopi(8) if(damagedirs[i]>0)
    {
        if(!dirs)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            gle::colorf(1, 0, 0, damagecompassalpha/100.0f);
            gle::defvertex();
            gle::begin(GL_TRIANGLES);
        }
        dirs++;

        float logscale = 32,
              scale = log(1 + (logscale - 1)*damagedirs[i]) / log(logscale),
              offset = -size/2.0f-min(h, w)/4.0f;
        matrix4x3 m;
        m.identity();
        m.settranslation(w/2, h/2, 0);
        m.rotate_around_z(i*45*RAD);
        m.translate(0, offset, 0);
        m.scale(size*scale);

        gle::attrib(m.transform(vec2(1, 1)));
        gle::attrib(m.transform(vec2(-1, 1)));
        gle::attrib(m.transform(vec2(0, 0)));

        // fade in log space so short blips don't disappear too quickly
        scale -= float(curtime)/damagecompassfade;
        damagedirs[i] = scale > 0 ? (pow(logscale, scale) - 1) / (logscale - 1) : 0;
    }
    if(dirs) gle::end();
}

int damageblendmillis = 0;

VARFP(damagescreen, 0, 1, 1, { if(!damagescreen) damageblendmillis = 0; });
VARP(damagescreenfactor, 1, 7, 100);
VARP(damagescreenalpha, 1, 45, 100);
VARP(damagescreenfade, 0, 125, 1000);
VARP(damagescreenmin, 1, 10, 1000);
VARP(damagescreenmax, 1, 100, 1000);

void damageblend(int n)
{
    if(!damagescreen || minimized) return;
    if(lastmillis > damageblendmillis) damageblendmillis = lastmillis;
    damageblendmillis += clamp(n, damagescreenmin, damagescreenmax)*damagescreenfactor;
}

void drawdamagescreen(int w, int h)
{
    if(lastmillis >= damageblendmillis) return;

    hudshader->set();

    static Texture *damagetex = NULL;
    if(!damagetex) damagetex = textureload("packages/hud/damage.png", 3);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, damagetex->id);
    float fade = damagescreenalpha/100.0f;
    if(damageblendmillis - lastmillis < damagescreenfade)
        fade *= float(damageblendmillis - lastmillis)/damagescreenfade;
    gle::colorf(fade, fade, fade, fade);

    hudquad(0, 0, w, h);
}

void cleardamagescreen()
{
    damageblendmillis = 0;
    loopi(8) damagedirs[i] = 0;
}

VAR(hidestats, 0, 0, 1);
VAR(hidehud, 0, 0, 1);

VARP(crosshairsize, 0, 30, 50);
VARP(cursorsize, 0, 20, 50);
VARP(crosshairfx, 0, 1, 1);
VARP(crosshaircolors, 0, 1, 1);

#define MAXCROSSHAIRS 4
static Texture *crosshairs[MAXCROSSHAIRS] = { NULL, NULL, NULL, NULL };

void loadcrosshair(const char *name, int i)
{
    if(i < 0 || i >= MAXCROSSHAIRS) return;
	crosshairs[i] = name ? textureload(name, 3, true) : notexture;
    if(crosshairs[i] == notexture)
    {
        name = game::defaultcrosshair(i);
        if(!name) name = "packages/crosshairs/default.png";
        crosshairs[i] = textureload(name, 3, true);
    }
}

void loadcrosshair_(const char *name, int *i)
{
	loadcrosshair(name, *i);
}

COMMANDN(loadcrosshair, loadcrosshair_, "si");

ICOMMAND(getcrosshair, "i", (int *i),
{
    const char *name = "";
    if(*i >= 0 && *i < MAXCROSSHAIRS)
    {
        name = crosshairs[*i] ? crosshairs[*i]->name : game::defaultcrosshair(*i);
        if(!name) name = "packages/crosshairs/default.png";
    }
    result(name);
});

void writecrosshairs(stream *f)
{
    loopi(MAXCROSSHAIRS) if(crosshairs[i] && crosshairs[i]!=notexture)
        f->printf("loadcrosshair %s %d\n", escapestring(crosshairs[i]->name), i);
    f->printf("\n");
}

void drawcrosshair(int w, int h)
{
    bool windowhit = g3d_windowhit(true, false);
    if(!windowhit && (hidehud || mainmenu)) return; //(hidehud || player->state==CS_SPECTATOR || player->state==CS_DEAD)) return;

    vec color(1, 1, 1);
    float cx = 0.5f, cy = 0.5f, chsize;
    Texture *crosshair;
    if(windowhit)
    {
        static Texture *cursor = NULL;
        if(!cursor) cursor = textureload("data/guicursor.png", 3, true);
        crosshair = cursor;
        chsize = cursorsize*w/900.0f;
        g3d_cursorpos(cx, cy);
#ifdef __ANDROID__
        // g3d_cursorpos() only knows the flat 2D cursor (cursorx/cursory,
        // set by the main menu's own controller-raycast) -- for the
        // in-game pause menu/scoreboard (real-3D "floating panel" gui
        // windows on Android, see usegui2d's cap in 3dgui.cpp) it falls
        // back to a hardcoded screen-center position that doesn't track
        // the controller at all, confirmed on-device as the cursor
        // appearing to vanish behind the menu panel instead of pointing
        // at whatever it's aimed at. Fixed the same way the weapon
        // crosshair below handles this: project worldpos (this frame's
        // aim raycast hit point against real world geometry, kept
        // updated every frame regardless of whether a menu is open by
        // SauerQuest_UpdateWeaponAim()) through this eye's own
        // camprojmatrix. worldpos itself is where the ray hits a wall,
        // not the (non-physical) menu panel, but perspective projection's
        // screen-space x/y depends only on direction from the eye, not
        // distance -- so this gives the controller's true on-screen aim
        // point regardless. Only reachable when a menu is actually open
        // and not the boot main menu itself, which already has a correct
        // cursorx/cursory from g3d_cursorpos() above.
        if(!mainmenu)
        {
            vec4 clip;
            camprojmatrix.transform(worldpos, clip);
            if(clip.w > 1e-4f && clip.z >= -clip.w)
            {
                cx = 0.5f + (clip.x/clip.w)*0.5f;
                cy = 0.5f - (clip.y/clip.w)*0.5f;
            }
        }
#endif
    }
    else
    {
        int index = game::selectcrosshair(color);
        if(index < 0) return;
        if(!crosshairfx) index = 0;
        if(!crosshairfx || !crosshaircolors) color = vec(1, 1, 1);
        crosshair = crosshairs[index];
        if(!crosshair)
        {
            loadcrosshair(NULL, index);
            crosshair = crosshairs[index];
        }
        chsize = crosshairsize*w/900.0f;
#ifdef __ANDROID__
        // Desktop's crosshair is always screen-center because desktop aims
        // with the head/mouse-look direction itself, so "center of view" and
        // "aim point" are the same thing by construction. This port's
        // controller-aimed weapon (worldpos, set in setcammatrix() from
        // androidgetaim()) decouples them -- leaving cx/cy at (0.5, 0.5)
        // here draws the reticle at the head-view center while the actual
        // shot travels toward wherever the controller is pointed, which
        // visibly diverges the moment the two aren't aligned (confirmed
        // on-device: reported as looking "doubled" and as if the crosshair
        // "moves with my head, not my gun"). Projecting worldpos through
        // this eye's own camprojmatrix instead gives the reticle's true
        // screen position for THIS eye specifically -- since it's a real
        // world-space point run through each eye's own (distinct,
        // asymmetric-FOV) projection, same as any other world object, it
        // naturally gets correct per-eye parallax too.
        if(!mainmenu)
        {
            vec4 clip;
            camprojmatrix.transform(worldpos, clip);
            if(clip.w > 1e-4f && clip.z >= -clip.w)
            {
                cx = 0.5f + (clip.x/clip.w)*0.5f;
                cy = 0.5f - (clip.y/clip.w)*0.5f;
            }
        }
#endif
    }
    if(crosshair->type&Texture::ALPHA) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    else glBlendFunc(GL_ONE, GL_ONE);
    float x = cx*w - (windowhit ? 0 : chsize/2.0f);
    float y = cy*h - (windowhit ? 0 : chsize/2.0f);
    glBindTexture(GL_TEXTURE_2D, crosshair->id);

    hudshader->set();
    gle::color(color);
    hudquad(x, y, chsize, chsize);
}

VARP(wallclock, 0, 0, 1);
VARP(wallclock24, 0, 0, 1);
VARP(wallclocksecs, 0, 0, 1);

static time_t walltime = 0;

VARP(showfps, 0, 1, 1);
VARP(showfpsrange, 0, 0, 1);
VAR(showeditstats, 0, 0, 1);
VAR(statrate, 1, 200, 1000);

FVARP(conscale, 1e-3f, 0.33f, 1e3f);

#ifdef __ANDROID__
// Confirmed on-device: the flat gameplay HUD (health/ammo/weapon icons,
// drawn via game::gameplayhud() below) is laid out in fixed pixel units
// that map 1:1 onto the eye buffer's actual resolution -- correct on a
// desktop monitor (a narrow ~40-50 degree FOV at normal viewing
// distance), but the same pixel-relative sizes now span the Quest's much
// wider per-eye FOV (~100+ degrees), so elements that used to sit
// comfortably in central vision end up both visually oversized and
// pushed out toward the edge of a FOV wider than comfortable eye
// rotation covers -- reported as the HUD being "too near"/"too big" and
// hard to actually look at. Fraction of the original on-screen size the
// HUD plane below is solved to occupy; tune live via /vrhudscale.
VAR(vrhudscale, 8, 20, 100);

// World units in front of the head to place the HUD plane (see
// androidsethudmatrixheadlocked() below) -- purely a comfort/fusion
// distance, since vrhudscale's formula keeps the apparent on-screen size
// constant regardless of this value.
VAR(vrhuddist, 2, 8, 200);

// Head-locked HUD plane, used only around the actual gameplay HUD draw
// calls (game::gameplayhud()/rendertexturepanel() below) -- NOT the base
// hudmatrix used for full-screen effects like the damage flash/compass,
// which still need genuine full-FOV ortho coverage.
//
// Root cause (confirmed on-device): pure screen-space ortho content has
// no real depth, so when submitted through the real per-eye
// asymmetric-FOV stereo projection layer, the compositor's own per-eye
// lens-distortion correction warps it differently for each eye -- each
// eye's own image looks fine alone, but the two never fuse into one
// (confirmed via a one-eye-closed-at-a-time test). This is the exact
// same root-cause category the crosshair fix already solved for a
// single point (drawcrosshair(), projecting worldpos through this eye's
// own camprojmatrix instead of drawing at a fixed screen position) --
// generalized here to an entire plane of content instead of one point.
// A cheaper, purely-2D fix (a constant NDC-space skew on the flat
// hudmatrix, mirroring the crosshair's own math) was tried previously
// for the scoreboard and had no effect -- the fusion problem needs an
// actual 3D point with real per-eye parallax, not a 2D approximation of
// one, which is what this does instead: the same pixel-space coordinates
// game::gameplayhud() already draws with are placed on a real plane
// vrhuddist world units in front of the head and projected through this
// eye's own real projmatrix (the same asymmetric frustum used for
// gameplay itself, still valid at this point in the frame -- proven by
// the crosshair's own working use of camprojmatrix, which is just
// projmatrix combined with cammatrix, at this same late point in the
// frame).
void androidsethudmatrixheadlocked(int w, int h)
{
    float s = vrhudscale/100.0f;
    float halfw = s*vrhuddist*(androidEyeTanR - androidEyeTanL)*0.5f;
    float halfh = s*vrhuddist*(androidEyeTanU - androidEyeTanD)*0.5f;
    hudmatrix = projmatrix;
    hudmatrix.translate(0, 0, -vrhuddist);
    hudmatrix.scale(2*halfw/w, -2*halfh/h, 1);
    hudmatrix.translate(-w*0.5f, -h*0.5f, 0);
}
#endif

void gl_drawhud()
{
    g3d_render();

    int w = screenw, h = screenh;
    if(forceaspect) w = int(ceil(h*forceaspect));

    if(editmode && !hidehud && !mainmenu)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        renderblendbrush();

        rendereditcursor();

        glDepthMask(GL_TRUE);
        glDisable(GL_DEPTH_TEST);
    }

    gettextres(w, h);

    hudmatrix.ortho(0, w, h, 0, -1, 1);
    resethudmatrix();

    gle::colorf(1, 1, 1);

    extern int debugsm;
    if(debugsm)
    {
        extern void viewshadowmap();
        viewshadowmap();
    }

    extern int debugglare;
    if(debugglare)
    {
        extern void viewglaretex();
        viewglaretex();
    }

    extern int debugdepthfx;
    if(debugdepthfx)
    {
        extern void viewdepthfxtex();
        viewdepthfxtex();
    }

    glEnable(GL_BLEND);

    extern void debugparticles();
    debugparticles();

    if(!mainmenu)
    {
        drawdamagescreen(w, h);
        drawdamagecompass(w, h);
    }

    hudshader->set();

    int conw = int(w/conscale), conh = int(h/conscale), abovehud = conh - FONTH, limitgui = abovehud;
    if(!hidehud && !mainmenu)
    {
        if(!hidestats)
        {
            pushhudmatrix();
            hudmatrix.scale(conscale, conscale, 1);
            flushhudmatrix();

            int roffset = 0;
            if(showfps)
            {
                static int lastfps = 0, prevfps[3] = { 0, 0, 0 }, curfps[3] = { 0, 0, 0 };
                if(totalmillis - lastfps >= statrate)
                {
                    memcpy(prevfps, curfps, sizeof(prevfps));
                    lastfps = totalmillis - (totalmillis%statrate);
                }
                int nextfps[3];
                getfps(nextfps[0], nextfps[1], nextfps[2]);
                loopi(3) if(prevfps[i]==curfps[i]) curfps[i] = nextfps[i];
                if(showfpsrange) draw_textf("fps %d+%d-%d", conw-7*FONTH, conh-FONTH*3/2, curfps[0], curfps[1], curfps[2]);
                else draw_textf("fps %d", conw-5*FONTH, conh-FONTH*3/2, curfps[0]);
                roffset += FONTH;
            }

            if(wallclock)
            {
                if(!walltime) { walltime = time(NULL); walltime -= totalmillis/1000; if(!walltime) walltime++; }
                time_t walloffset = walltime + totalmillis/1000;
                struct tm *localvals = localtime(&walloffset);
                static string buf;
                if(localvals && strftime(buf, sizeof(buf), wallclocksecs ? (wallclock24 ? "%H:%M:%S" : "%I:%M:%S%p") : (wallclock24 ? "%H:%M" : "%I:%M%p"), localvals))
                {
                    // hack because not all platforms (windows) support %P lowercase option
                    // also strip leading 0 from 12 hour time
                    char *dst = buf;
                    const char *src = &buf[!wallclock24 && buf[0]=='0' ? 1 : 0];
                    while(*src) *dst++ = tolower(*src++);
                    *dst++ = '\0';
                    draw_text(buf, conw-5*FONTH, conh-FONTH*3/2-roffset);
                    roffset += FONTH;
                }
            }

            if(editmode || showeditstats)
            {
                static int laststats = 0, prevstats[8] = { 0, 0, 0, 0, 0, 0, 0 }, curstats[8] = { 0, 0, 0, 0, 0, 0, 0 };
                if(totalmillis - laststats >= statrate)
                {
                    memcpy(prevstats, curstats, sizeof(prevstats));
                    laststats = totalmillis - (totalmillis%statrate);
                }
                int nextstats[8] =
                {
                    vtris*100/max(wtris, 1),
                    vverts*100/max(wverts, 1),
                    xtraverts/1024,
                    xtravertsva/1024,
                    glde,
                    gbatches,
                    getnumqueries(),
                    rplanes
                };
                loopi(8) if(prevstats[i]==curstats[i]) curstats[i] = nextstats[i];

                abovehud -= 2*FONTH;
                draw_textf("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", FONTH/2, abovehud, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]);
                draw_textf("ond:%d va:%d gl:%d(%d) oq:%d lm:%d rp:%d pvs:%d", FONTH/2, abovehud+FONTH, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6], lightmaps.length(), curstats[7], getnumviewcells());
                limitgui = abovehud;
            }

            if(editmode)
            {
                abovehud -= FONTH;
                draw_textf("cube %s%d%s", FONTH/2, abovehud, selchildcount<0 ? "1/" : "", abs(selchildcount), showmat && selchildmat > 0 ? getmaterialdesc(selchildmat, ": ") : "");

                if(char *editinfo = execidentstr("edithud"))
                {
                    if(editinfo[0])
                    {
                        int tw, th;
                        text_bounds(editinfo, tw, th);
                        th += FONTH-1; th -= th%FONTH;
                        abovehud -= max(th, FONTH);
                        draw_text(editinfo, FONTH/2, abovehud);
                    }
                    DELETEA(editinfo);
                }
            }
            else if(char *gameinfo = execidentstr("gamehud"))
            {
                if(gameinfo[0])
                {
                    int tw, th;
                    text_bounds(gameinfo, tw, th);
                    th += FONTH-1; th -= th%FONTH;
                    roffset += max(th, FONTH);
                    draw_text(gameinfo, conw-max(5*FONTH, 2*FONTH+tw), conh-FONTH/2-roffset);
                }
                DELETEA(gameinfo);
            }

            pophudmatrix();
        }

#ifdef __ANDROID__
        bool vrheadlockhud = !mainmenu;
        if(vrheadlockhud) { pushhudmatrix(); androidsethudmatrixheadlocked(w, h); flushhudmatrix(); }
#endif
        if(hidestats || (!editmode && !showeditstats))
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            game::gameplayhud(w, h);
            limitgui = abovehud = min(abovehud, int(conh*game::abovegameplayhud(w, h)));
        }

        rendertexturepanel(w, h);
#ifdef __ANDROID__
        if(vrheadlockhud) pophudmatrix();
#endif
    }

    glDisable(GL_BLEND);

    g3d_limitscale((2*limitgui - conh) / float(conh));
    // The scoreboard and in-game pause/options menu (both reached via
    // showgui while a game is running -- data/menus.cfg's
    // togglemainmenu) also draw here, through the same doubling bug the
    // gameplay HUD above had. Wrapping this call in the same
    // head-locked-plane hudmatrix trick had NO effect (confirmed
    // on-device): gui::start() (3dgui.cpp) unconditionally rebuilds
    // hudmatrix from scratch for every 2D gui window it draws, discarding
    // whatever was set here first. Fixed instead at the source -- see
    // usegui2d's cap (3dgui.cpp) and scoreboard2d's cap (scoreboard.cpp)
    // on Android, which route these windows into the engine's own
    // pre-existing real-3D gui path instead.
    g3d_render2d();

    glEnable(GL_BLEND);

    hudmatrix.ortho(0, w, h, 0, -1, 1);
    resethudmatrix();

    pushhudmatrix();
    hudmatrix.scale(conscale, conscale, 1);
    flushhudmatrix();
    abovehud -= rendercommand(FONTH/2, abovehud - FONTH/2, conw-FONTH);
    extern int fullconsole;
    if(!hidehud || fullconsole) renderconsole(conw, conh, abovehud - FONTH/2);
    pophudmatrix();

    drawcrosshair(w, h);

#ifdef __ANDROID__
    if(!mainmenu) drawvrdim(vrgamedim, w, h);
#endif

    glDisable(GL_BLEND);
}

void cleanupgl()
{
    cleanupmotionblur();

    clearminimap();

    cleanupscreenquad();

    gle::cleanup();
}


