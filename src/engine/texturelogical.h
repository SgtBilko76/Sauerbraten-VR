// Optional logical UV dimensions for higher-resolution world textures.
// Upload size and mip levels remain independent of map-space dimensions.
#ifndef SAUVR_TEXTURELOGICAL_H
#define SAUVR_TEXTURELOGICAL_H
#include <cstring>

static inline bool texturelogicalsize(const char *name, int &width, int &height)
{
    if(!name) return false;
    const char *p = name;
    while(*p == '<')
    {
        const char *end = std::strchr(p, '>');
        if(!end) return false;
        if(std::strncmp(p, "<logical:", 9) == 0)
        {
            p += 9;
            int sizes[2] = {0, 0};
            for(int i = 0; i < 2; ++i)
            {
                if(p == end || *p < '0' || *p > '9') return false;
                while(p < end && *p >= '0' && *p <= '9')
                {
                    sizes[i] = sizes[i]*10 + (*p++ - '0');
                    if(sizes[i] > 16384) return false;
                }
                if(!sizes[i]) return false;
                if(i == 0 && (p == end || *p++ != ',')) return false;
            }
            if(p != end) return false;
            width = sizes[0]; height = sizes[1];
            return true;
        }
        p = end + 1;
    }
    return false;
}
#endif
