
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define VC_LEANMEAN
#define VC_EXTRALEAN

#include <windows.h>
#include <GL/gl.h>
#include "glext.h"
#include "timeapi.h"

#if EDITOR
	static HWND hwnd;
	static double editor_time = 0.;
	static bool editor_paused = false;
	static bool editor_muted_music = false;
	#include <dwmapi.h>
	#pragma comment(lib, "dwmapi.lib")
#endif

#if LAPTOP_GPU_FIX
extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif


#pragma data_seg(".pixelfmt")
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(pfd), 1, PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
		32, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 32, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};

#pragma data_seg(".screensettings")
	DEVMODE screenSettings = { 
		{0}, 0, 0, sizeof(screenSettings), 0, DM_PELSWIDTH|DM_PELSHEIGHT,
		{0}, 0, 0, 0, 0, 0, {0}, 0, 0, XRES, YRES, 0, 0,
		#if(WINVER >= 0x0400)
			0, 0, 0, 0, 0, 0,
				#if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
				0, 0
			#endif
		#endif
	};

//screenSettings.dmFields=DM_PELSWIDTH|DM_PELSHEIGHT;

#define oglCreateFramebuffers ((PFNGLCREATEFRAMEBUFFERSPROC)wglGetProcAddress("glCreateFramebuffers"))
#define oglNamedFramebufferDrawBuffers ((PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC)wglGetProcAddress("glNamedFramebufferDrawBuffers"))
#define oglDispatchCompute ((PFNGLDISPATCHCOMPUTEPROC)wglGetProcAddress("glDispatchCompute"))
#define oglTexStorage2D ((PFNGLTEXSTORAGE2DPROC)wglGetProcAddress("glTexStorage2D"))
#define oglBindImageTexture ((PFNGLBINDIMAGETEXTUREPROC)wglGetProcAddress("glBindImageTexture"))
#define oglCreateShaderProgramv ((PFNGLCREATESHADERPROGRAMVPROC)wglGetProcAddress("glCreateShaderProgramv"))
#define oglBindFramebuffer ((PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer"))
#define oglGenFramebuffers ((PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers"))
#define oglFramebufferTexture2D ((PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D"))
#define oglNamedFramebufferTexture ((PFNGLNAMEDFRAMEBUFFERTEXTUREPROC)wglGetProcAddress("glNamedFramebufferTexture"))
#define oglBindTextureUnit ((PFNGLBINDTEXTUREUNITPROC)wglGetProcAddress("glBindTextureUnit"))
#define oglDebugMessageCallback ((PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback"))
#define oglDrawBuffers ((PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffers"))
#define oglGetProgramiv ((PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv"))
#define oglGetProgramInfoLog ((PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog"))
#define oglDeleteProgram ((PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram"))
#define oglUseProgram ((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))
#define oglUniform1i ((PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i"))
#define oglUniform2i ((PFNGLUNIFORM2IPROC)wglGetProcAddress("glUniform2i"))
#define oglUniform2f ((PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f"))
#define oglUniform1f ((PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f"))
#define oglCreateBuffers ((PFNGLCREATEBUFFERSPROC)wglGetProcAddress("glCreateBuffers"))
#define oglNamedBufferStorage ((PFNGLNAMEDBUFFERSTORAGEPROC)wglGetProcAddress("glNamedBufferStorage"))
#define oglBindBufferBase ((PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase"))
#define oglGetNamedBufferSubData ((PFNGLGETNAMEDBUFFERSUBDATAPROC)wglGetProcAddress("glGetNamedBufferSubData"))
#define oglGenBuffers ((PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers"))
#define oglBindBuffer ((PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer"))
#define oglBufferData ((PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData"))
#define oglMapBuffer ((PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer"))
#define oglUnmapBuffer ((PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer"))
#define oglClearTexSubImage ((PFNGLCLEARTEXSUBIMAGEPROC)wglGetProcAddress("glClearTexSubImage"))
#define oglGetUniformLocation ((PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation"))
#define oglMemoryBarrier ((PFNGLMEMORYBARRIERPROC)wglGetProcAddress("glMemoryBarrier"))
#define oglBlitNamedFramebuffer ((PFNGLBLITNAMEDFRAMEBUFFERPROC)wglGetProcAddress("glBlitNamedFramebuffer"))





// declare this symbol if your code uses floating point types
extern "C" int _fltused;

#define FAIL_KILL true
#define PID_QUALIFIER const

