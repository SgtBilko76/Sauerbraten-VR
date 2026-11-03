#include "engine.h"
#include "rendertarget.h"

#ifdef __ANDROID__
// Tried enabling this -- initial on-device check looked fine, but a
// real bot match (more models/bots casting shadows simultaneously than
// the earlier check happened to stress) confirmed it GPU-saturates
// (VrApi's own GPU%=0.87-1.00) and drops to 26-40/72fps with heavy
// stale-frame counts, regardless of resolution (confirmed the same at
// both 1.2x and 1.3x supersampling -- shadowmap itself is the dominant
// cost, not resolution). Back to off. If revisited, needs a real,
// sustained multi-bot match for verification, not just a quick look.
VAR(shadowmap, 0, 0, 1);
#else
VARP(shadowmap, 0, 0, 1);
#endif

extern void cleanshadowmap();
VARFP(shadowmapsize, 7, 9, 11, cleanshadowmap());
VARP(shadowmapradius, 64, 96, 256);
VAR(shadowmapheight, 0, 32, 128);
VARP(shadowmapdist, 128, 256, 512);
VARFP(fpshadowmap, 0, 0, 1, cleanshadowmap());
VARFP(shadowmapprecision, 0, 0, 1, cleanshadowmap());
bvec shadowmapambientcolor(0, 0, 0);
HVARFR(shadowmapambient, 0, 0, 0xFFFFFF,
{
    if(shadowmapambient <= 255) shadowmapambient |= (shadowmapambient<<8) | (shadowmapambient<<16);
    shadowmapambientcolor = bvec((shadowmapambient>>16)&0xFF, (shadowmapambient>>8)&0xFF, shadowmapambient&0xFF);
});
VARP(shadowmapintensity, 0, 60, 100);

VARP(blurshadowmap, 0, 1, 3);
VARP(blursmsigma, 1, 100, 200);

#define SHADOWSKEW 0.7071068f

vec shadowoffset(0, 0, 0), shadowfocus(0, 0, 0), shadowdir(0, SHADOWSKEW, 1);
VAR(shadowmapcasters, 1, 0, 0);
float shadowmapmaxz = 0;

void setshadowdir(int angle)
{
    shadowdir = vec(0, SHADOWSKEW, 1);
    shadowdir.rotate_around_z(angle*RAD);
}

VARFR(shadowmapangle, 0, 0, 360, setshadowdir(shadowmapangle));

void guessshadowdir()
{
    if(shadowmapangle) return;
    vec dir;
    if(!sunlightcolor.iszero()) dir = sunlightdir;
    else
    {
        vec lightpos(0, 0, 0), casterpos(0, 0, 0);
        int numlights = 0, numcasters = 0;
        const vector<extentity *> &ents = entities::getents();
        loopv(ents)
        {
            extentity &e = *ents[i];
            switch(e.type)
            {
                case ET_LIGHT:
                    if(!e.attr1) { lightpos.add(e.o); numlights++; }
                    break;

                case ET_MAPMODEL:
                    casterpos.add(e.o);
                    numcasters++;
                    break;

                default:
                    if(e.type<ET_GAMESPECIFIC) break;
                    casterpos.add(e.o);
                    numcasters++;
                    break;
            }
        }
        if(!numlights || !numcasters) return;
        lightpos.div(numlights);
        casterpos.div(numcasters);
        dir = vec(lightpos).sub(casterpos);
    }
    dir.z = 0;
    if(dir.iszero()) return;
    dir.normalize();
    dir.mul(SHADOWSKEW);
    dir.z = 1;
    shadowdir = dir;
}



// M1 (diseño sombras-vr): un único marco de sombra por frame. Lo calcula gl_drawframe antes de
// visiblecubes() y lo leen la matriz, las cajas de casters y los tests de receptor. No usa yaw ni
// pitch de la cámara (en VR son los de la cabeza). Se ajusta a una rejilla de texeles.
void computeshadowframe()
{
    float texel = 2*shadowmapradius/float(1<<shadowmapsize);
    shadowfocus = camera1->o;
    shadowfocus.x = floor(shadowfocus.x/texel)*texel;
    shadowfocus.y = floor(shadowfocus.y/texel)*texel;
    shadowfocus.z += shadowmapheight;
    shadowoffset = vec(0, 0, 0);
    shadowmapmaxz = shadowfocus.z - shadowmapdist;
}

bool shadowmapping = false;


matrix4 shadowmatrix;

VARP(shadowmapbias, 0, 5, 1024);
VARP(shadowmappeelbias, 0, 20, 1024);
#ifdef __ANDROID__
// M1: el mapa no guarda la profundidad del mundo (paredes y suelo); evita el acné de cada superficie.
VAR(smdepthpeel, 0, 0, 1);
#else
VAR(smdepthpeel, 0, 1, 1);
#endif
VAR(smoothshadowmappeel, 1, 0, 0);

static struct shadowmaptexture : rendertarget
{
    const GLenum *colorformats() const
    {
        static const GLenum rgbafmts[] = { GL_RGBA16F, GL_RGBA16, GL_RGBA, GL_RGBA8, GL_FALSE };
        return &rgbafmts[fpshadowmap && hasTF ? 0 : (shadowmapprecision ? 1 : 2)];
    }

    bool swaptexs() const { return true; }

    bool scissorblur(int &x, int &y, int &w, int &h)
    {
        x = max(int(floor((scissorx1+1)/2*vieww)) - 2*blursize, 2);
        y = max(int(floor((scissory1+1)/2*viewh)) - 2*blursize, 2);
        w = min(int(ceil((scissorx2+1)/2*vieww)) + 2*blursize, vieww-2) - x;
        h = min(int(ceil((scissory2+1)/2*viewh)) + 2*blursize, viewh-2) - y;
        return true;
    }

    bool scissorrender(int &x, int &y, int &w, int &h)
    {
        x = y = 2;
        w = vieww - 2*2;
        h = viewh - 2*2;
        return true;
    }

    void doclear()
    {
        glClearColor(0, 0, 0, 0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }

    bool dorender()
    {
        // M1: la matriz sale de shadowfocus (calculado por computeshadowframe), no del yaw de la cámara.
        vec skewdir(shadowdir);
        shadowmatrix.ortho(-shadowmapradius, shadowmapradius, -shadowmapradius, shadowmapradius, -shadowmapdist, shadowmapdist);
        shadowmatrix.mul(matrix3(vec(1, 0, 0), vec(0, 1, 0), vec(skewdir.x, skewdir.y, 1)));
        shadowmatrix.translate(vec(shadowfocus).neg());
        GLOBALPARAM(shadowmatrix, shadowmatrix);

        gle::colorf(0, 0, 0);

        GLOBALPARAMF(shadowmapbias, -shadowmapbias/float(shadowmapdist), 1 - (shadowmapbias + (smoothshadowmappeel ? 0 : shadowmappeelbias))/float(shadowmapdist));

        shadowmapcasters = 0;
        shadowmapmaxz = shadowfocus.z - shadowmapdist;
        shadowmapping = true;
        rendergame();
        shadowmapping = false;
        shadowmapmaxz = min(shadowmapmaxz, shadowfocus.z);

        if(shadowmapcasters && smdepthpeel) 
        {
            int sx, sy, sw, sh;
            bool scissoring = rtscissor && scissorblur(sx, sy, sw, sh) && sw > 0 && sh > 0;
            if(scissoring) glScissor(sx, sy, sw, sh);
            if(!rtscissor || scissoring) rendershadowmapreceivers();
        }

        return shadowmapcasters>0;
    }

    bool flipdebug() const { return false; }

    void dodebug(int w, int h)
    {
        if(shadowmapcasters)
        {
            glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
            debugscissor(w, h);
            glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);
            debugblurtiles(w, h);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
    }
} shadowmaptex;

void cleanshadowmap()
{
    shadowmaptex.cleanup(true);
}

void calcshadowmapbb(const vec &o, float xyrad, float zrad, float &x1, float &y1, float &x2, float &y2)
{
    // M1: mismo marco que la matriz (centrado en shadowfocus, sin yaw ni pitch).
    vec skewdir(shadowdir);
    vec ro(o);
    ro.sub(shadowfocus);
    ro.x += ro.z * skewdir.x;
    ro.y += ro.z * skewdir.y;
    vec high(ro), low(ro);
    high.x += zrad * skewdir.x;
    high.y += zrad * skewdir.y;
    low.x -= zrad * skewdir.x;
    low.y -= zrad * skewdir.y;
    x1 = (min(high.x, low.x) - xyrad) / shadowmapradius;
    y1 = (min(high.y, low.y) - xyrad) / shadowmapradius;
    x2 = (max(high.x, low.x) + xyrad) / shadowmapradius;
    y2 = (max(high.y, low.y) + xyrad) / shadowmapradius;
}


bool addshadowmapcaster(const vec &o, float xyrad, float zrad)
{
    if(o.z + zrad <= shadowfocus.z - shadowmapdist || o.z - zrad >= shadowfocus.z) return false;

    shadowmapmaxz = max(shadowmapmaxz, o.z + zrad);

    float x1, y1, x2, y2;
    calcshadowmapbb(o, xyrad, zrad, x1, y1, x2, y2);

    if(!shadowmaptex.addblurtiles(x1, y1, x2, y2, 2)) return false;

    shadowmapcasters++;
    return true;
}

bool isshadowmapreceiver(vtxarray *va)
{
    if(!shadowmap || !shadowmapcasters) return false;

    if(va->shadowmapmax.z <= shadowfocus.z - shadowmapdist || va->shadowmapmin.z >= shadowmapmaxz) return false;

    float xyrad = SQRT2*0.5f*max(va->shadowmapmax.x-va->shadowmapmin.x, va->shadowmapmax.y-va->shadowmapmin.y),
          zrad = 0.5f*(va->shadowmapmax.z-va->shadowmapmin.z),
          x1, y1, x2, y2;
    if(xyrad<0 || zrad<0) return false;

    vec center = vec(va->shadowmapmin).add(vec(va->shadowmapmax)).mul(0.5f);
    calcshadowmapbb(center, xyrad, zrad, x1, y1, x2, y2);

    return shadowmaptex.checkblurtiles(x1, y1, x2, y2, 2);

#if 0
    // cheaper inexact test
    float dz = va->o.z + va->size/2 - shadowfocus.z;
    float cx = shadowfocus.x + dz*shadowdir.x, cy = shadowfocus.y + dz*shadowdir.y;
    float skew = va->size/2*SHADOWSKEW;
    if(!shadowmap || !shadowmaptex ||
       va->o.z + va->size <= shadowfocus.z - shadowmapdist || va->o.z >= shadowmapmaxz ||
       va->o.x + va->size <= cx - shadowmapradius-skew || va->o.x >= cx + shadowmapradius+skew || 
       va->o.y + va->size <= cy - shadowmapradius-skew || va->o.y >= cy + shadowmapradius+skew) 
        return false;
    return true;
#endif
}

bool isshadowmapcaster(const vec &o, float rad)
{
    // cheaper inexact test
    float dz = o.z - shadowfocus.z;
    float cx = shadowfocus.x + dz*shadowdir.x, cy = shadowfocus.y + dz*shadowdir.y;
    float skew = rad*SHADOWSKEW;
    if(!shadowmapping ||
       o.z + rad <= shadowfocus.z - shadowmapdist || o.z - rad >= shadowfocus.z ||
       o.x + rad <= cx - shadowmapradius-skew || o.x - rad >= cx + shadowmapradius+skew ||
       o.y + rad <= cy - shadowmapradius-skew || o.y - rad >= cy + shadowmapradius+skew)
        return false;
    return true;
}

void pushshadowmap()
{
    if(!shadowmap || !shadowmaptex.rendertex) return;

    glActiveTexture_(GL_TEXTURE0+15);
    glBindTexture(GL_TEXTURE_2D, shadowmaptex.rendertex);

    matrix4 m = shadowmatrix;
    m.projective(-1, 1-shadowmapbias/float(shadowmapdist));
    GLOBALPARAM(shadowmapproject, m);

    glActiveTexture_(GL_TEXTURE0);

    float r, g, b;
	if(!shadowmapambient)
	{
		if(skylightcolor[0] || skylightcolor[1] || skylightcolor[2])
		{
			r = max(25.0f, 0.4f*ambientcolor[0] + 0.6f*max(ambientcolor[0], skylightcolor[0]));
			g = max(25.0f, 0.4f*ambientcolor[1] + 0.6f*max(ambientcolor[1], skylightcolor[1]));
			b = max(25.0f, 0.4f*ambientcolor[2] + 0.6f*max(ambientcolor[2], skylightcolor[2]));
		}
		else 
        {
            r = max(25.0f, 2.0f*ambientcolor[0]);
            g = max(25.0f, 2.0f*ambientcolor[1]);
            b = max(25.0f, 2.0f*ambientcolor[2]);
        }
	}
    else { r = shadowmapambientcolor[0]; g = shadowmapambientcolor[1]; b = shadowmapambientcolor[2]; }
    // M1 (oscuridad constante): la luz dentro de la sombra se reduce por shadowmapintensity (0..100).
    // Valor fijo, sin fundidos por frame: no cambia de un frame a otro.
    float darken = shadowmapintensity/100.0f;
    r *= 1 - darken; g *= 1 - darken; b *= 1 - darken;
    GLOBALPARAMF(shadowmapambient, r/255.0f, g/255.0f, b/255.0f);
}

void popshadowmap()
{
    if(!shadowmap || !shadowmaptex.rendertex) return;
    // M1: soltar la unidad 15; si el mapa queda enlazado, otros pases lo leen como textura.
    glActiveTexture_(GL_TEXTURE0+15);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture_(GL_TEXTURE0);
}

// SauVREngine: tiempo de CPU del shadowmap en el frame (µs), para la telemetría shadow=.
uint64_t vrshadowus = 0;
// Diagnóstico: nombres de los modelos que llegan al pase de sombras en el último segundo (telemetría smcast=).
char vrsmcast[768] = "";
void vrsmnote(const char *mdl)
{
    if(strstr(vrsmcast, mdl)) return;
    if(strlen(vrsmcast) + strlen(mdl) + 2 < sizeof(vrsmcast)) { strcat(vrsmcast, mdl); strcat(vrsmcast, " "); }
}

float vrshjump = 0;
int vrshflip = 0, vrsmerr = 0;

void rendershadowmap()
{
    if(!shadowmap) return;

    // M1: el pase guarda y restaura TODO su estado: framebuffer, viewport, scissor, máscaras,
    // profundidad, culling, blend, unidad activa y programa. Antes dejaba el por defecto.
    GLint fb = 0, vp[4], box[4], prog = 0, activetex = 0, cullmode = 0, depthfunc = 0;
    GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST), cull = glIsEnabled(GL_CULL_FACE), blend = glIsEnabled(GL_BLEND), depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean colmask[4], depthmask = GL_TRUE;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fb);
    glGetIntegerv(GL_VIEWPORT, vp);
    glGetIntegerv(GL_SCISSOR_BOX, box);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activetex);
    glGetIntegerv(GL_CULL_FACE_MODE, &cullmode);
    glGetIntegerv(GL_DEPTH_FUNC, &depthfunc);
    glGetBooleanv(GL_COLOR_WRITEMASK, colmask);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthmask);
    Shader *prevshader = Shader::lastshader;

    // SauVREngine: sin recortes por tiles ni scissor (dejaban sectores rectangulares).
    int oldrtscissor = rtscissor, oldblurtile = blurtile;
    rtscissor = 0; blurtile = 0;
    uint64_t t0 = SDL_GetPerformanceCounter();
    shadowmaptex.render(1<<shadowmapsize, 1<<shadowmapsize, blurshadowmap, blursmsigma/100.0f);
    vrshadowus += (SDL_GetPerformanceCounter() - t0) * 1000000 / SDL_GetPerformanceFrequency();
    // Diagnóstico M1: salto de la caja entre frames y errores GL del pase.
    { static vec lastfocus(0, 0, 0); vrshjump = max(vrshjump, shadowfocus.dist(lastfocus)); lastfocus = shadowfocus; }
    for(GLenum e; (e = glGetError()) != GL_NO_ERROR; ) vrsmerr++;
    rtscissor = oldrtscissor; blurtile = oldblurtile;

    glBindFramebuffer_(GL_FRAMEBUFFER, fb);
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    if(scissor) { glEnable(GL_SCISSOR_TEST); glScissor(box[0], box[1], box[2], box[3]); } else glDisable(GL_SCISSOR_TEST);
    if(cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    glCullFace(cullmode);
    if(blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if(depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthFunc(depthfunc);
    glColorMask(colmask[0], colmask[1], colmask[2], colmask[3]);
    glDepthMask(depthmask);
    glUseProgram_(prog);
    glActiveTexture_(activetex);
    Shader::lastshader = prevshader;
}

VAR(debugsm, 0, 0, 1);

void viewshadowmap()
{
    if(!shadowmap) return;
    shadowmaptex.debug();
}

