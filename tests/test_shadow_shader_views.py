"""Exercise the real shader source assembly with intercepted GL calls (no GPU)."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class ShadowShaderViews(unittest.TestCase):
    def test_real_compiler_source_assembly(self):
        source = (ROOT / 'src/engine/shader.cpp').read_text()
        rewrite = source[source.index('static bool isidentchar'):source.index('\n#endif', source.index('static bool isidentchar'))]
        compiler = source[source.index('static void compileglslshader'):source.index('\nVAR(dbgubo')]
        harness = r'''
#include <cassert>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
typedef unsigned char uchar;
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef char GLchar;
const int GL_VERTEX_SHADER=1, GL_FRAGMENT_SHADER=2, GL_COMPILE_STATUS=3;
int vrmultiviewactive=1, glslversion=320, glversion=320, dbgshader=0;
#define loopi(n) for(int i=0;i<int(n);++i)
#define loopj(n) for(int j=0;j<int(n);++j)
#define formatstring(buf, ...) snprintf(buf, sizeof(buf), __VA_ARGS__)
template<class T> struct vector { std::vector<T> v; void add(T x){v.push_back(x);} T *getbuf(){return v.data();} };
std::string captured;
GLuint glCreateShader_(GLenum){return 1;}
void glShaderSource_(GLuint, int n, const GLchar **p, const GLint *) { captured.clear(); for(int i=0;i<n;++i) captured += p[i]; }
void glCompileShader_(GLuint){}
void glGetShaderiv_(GLuint, GLenum, GLint *success){*success=1;}
void glDeleteShader_(GLuint){}
void showglslinfo(GLenum, GLuint, const char *, const char **, int){}
'''
        harness += rewrite + '\n' + compiler
        harness += r'''
int main() {
 GLuint obj=0;
 const char *single="//:singleview\nuniform mat4 modelmatrix;\nvoid main(){gl_Position=modelmatrix*vec4(1.0);}";
 compileglslshader(GL_VERTEX_SHADER,obj,single,"shadowmapcaster");
 assert(captured.find("num_views") == std::string::npos);
 assert(captured.find("gl_ViewID_OVR") == std::string::npos);
 assert(captured.find("uniform mat4 modelmatrix;") != std::string::npos);
 compileglslshader(GL_VERTEX_SHADER,obj,single,"<variant:3,0>shadowmapcaster");
 assert(captured.find("gl_ViewID_OVR") == std::string::npos);
 compileglslshader(GL_FRAGMENT_SHADER,obj,"//:singleview\nvoid main(){gl_FragColor=vec4(1.0);}","blurx2");
 assert(captured.find("GL_OVR_multiview2") == std::string::npos);
 const char *world="uniform mat4 camprojmatrix;\nvoid main(){gl_Position=camprojmatrix*vec4(1.0);}";
 compileglslshader(GL_VERTEX_SHADER,obj,world,"<shadowmap>stdworld");
 assert(captured.find("layout(num_views = 2)") != std::string::npos);
 assert(captured.find("uniform mat4 camprojmatrix[2];") != std::string::npos);
 assert(captured.find("camprojmatrix[gl_ViewID_OVR]") != std::string::npos);
 compileglslshader(GL_FRAGMENT_SHADER,obj,"void main(){gl_FragColor=vec4(1.0);}","<shadowmap>stdworld");
 assert(captured.find("GL_OVR_multiview2") != std::string::npos);
 vrmultiviewactive=0;
 compileglslshader(GL_VERTEX_SHADER,obj,world,"stdworld");
 assert(captured.find("num_views") == std::string::npos);
 assert(captured.find("camprojmatrix[2]") == std::string::npos);
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            cpp = pathlib.Path(tmp)/'views.cpp'
            cpp.write_text(harness)
            binary = pathlib.Path(tmp)/'views'
            subprocess.run(['g++','-D__ANDROID__','-std=c++11','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)

    def test_both_asset_copies_cover_all_single_view_passes(self):
        desktop = (ROOT/'data/glsl.cfg').read_text()
        packaged = (ROOT/'android/app/src/main/assets/data/glsl.cfg').read_text()
        self.assertEqual(desktop, packaged)
        sections = [
            ('shadowmapcastervertexshader = [','shader 0 "shadowmapcaster"',1),
            ('shader 0 "shadowmapcaster"','shader 0 "shadowmapreceiver"',1),
            ('shader 0 "shadowmapreceiver"','notexturemodelvertexshader = [',2),
            ('blurshader = [','loop i 7 [',2),
        ]
        for start,end,count in sections:
            section = desktop[desktop.index(start):desktop.index(end,desktop.index(start)+len(start))]
            self.assertEqual(section.count('//:singleview'),count,start)
        self.assertIn('(shadowmapcastervertexshader (skelanimdefs',desktop)

if __name__ == '__main__': unittest.main()
