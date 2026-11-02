#include "../src/engine/texturelogical.h"
#include <cassert>
int main()
{
    int w = 512, h = 512;
    assert(texturelogicalsize("<logical:128,256>sauvr/a.jpg", w, h));
    assert(w == 128 && h == 256);
    assert(texturelogicalsize("<mad:1,1><logical:256,128>a.jpg", w, h));
    const char *bad[] = {"a.jpg", "a<logical:1,1>.jpg", "<logical:0,1>a", "<logical:-1,1>a", "<logical:1,1x>a", "<logical:99999999999,1>a", "<logical:1,>a", "<logical:1,1", "<logical:1,1,1>a"};
    for(unsigned i=0; i<sizeof(bad)/sizeof(*bad); ++i)
    {
        assert(!texturelogicalsize(bad[i], w, h));
        assert(w == 256 && h == 128);
    }
    // Same UV scale and pixel offset before and after increasing upload resolution.
    assert(1.0f/w == 1.0f/256 && 7.0f/h == 7.0f/128);
}
