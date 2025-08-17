#define WINDOWS_IGNORE_PACKING_MISMATCH


#define EDITOR												1
#define EDITOR_FORCE_RELOAD_FROM_DISK	1
#define VALIDATE_SHADERS_GLSLANG			0
#define OPENGL_DEBUG									1


#define ANTI_TDR											1
#define LAPTOP_GPU_FIX								0

#define FULLSCREEN										0
#define AUTORES												0
#define VSYNC													1

#define DO_PERFECT_FFMPEG_CAPTURE			0



#ifdef RELEASE
	#define DO_PERFECT_FFMPEG_CAPTURE			0
	#define FULLSCREEN										1
	#define ANTI_TDR											1
	#define EDITOR												0
	#define EDITOR_FORCE_RELOAD_FROM_DISK	0
	#define VALIDATE_SHADERS_GLSLANG			0
	#define OPENGL_DEBUG									0
#endif

//#define XRES													1280
//#define YRES													720

#define XRES													1920
#define YRES													1080



const int SAMPLE_RATE=44100;
const int SONG_LENGTH=(int)((60./90.)*20.0*15.0);
const int SONG_SAMPLES=SAMPLE_RATE*SONG_LENGTH;

#define MUSIC_DURATION								(SONG_LENGTH-10)
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ 
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ 
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ 
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ 
#define PROG_MUSIC										programs[2]

#include "definitions.h"
#include "shaders/all_shaders.h"

#pragma data_seg(".pids")
	//static int frame = 0;
	static int programs[10];
	static GLuint ssbo;
	static float music_time;
	static HDC hDC;
//#pragma data_seg(".pidsb")


#include "debug.h"
#include "music.h"
#include "editor.h"

static void __forceinline init_window() {
#if EDITOR
    #define WINDOW_CLASS_NAME "G"
#else
    #define WINDOW_CLASS_NAME (LPCSTR)0xC018
#endif


	#if EDITOR
		for(int i = 0; i < 20; i++){
			editor_keys_pressed[i] = false;
		}
		HINSTANCE hInstance =	GetModuleHandle(NULL);

    WNDCLASS wc = {};
    wc.lpfnWndProc = editor_winapi_window_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
    RegisterClass(&wc);
		
    int screenWidth = GetSystemMetrics(SM_CXSCREEN); 
    int screenHeight = GetSystemMetrics(SM_CYSCREEN); 

    int windowX = (screenWidth - XRES) / 2;
    int windowY = (screenHeight - YRES) / 2;
	#endif
	#if AUTORES
		xres = GetSystemMetrics(SM_CXSCREEN);
		yres = GetSystemMetrics(SM_CYSCREEN);
	#else
	#endif

	#if FULLSCREEN
		auto disp_settings = ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN);
		ShowCursor(0);

		#if EDITOR
			hDC = GetDC(hwnd = CreateWindow(
				WINDOW_CLASS_NAME, 0, WS_POPUPWINDOW | WS_VISIBLE, 0, 0, XRES, YRES, 0, 0, 0, 0
			));
		#else
			hDC = GetDC(CreateWindow((LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE, 0, 0, XRES, YRES, 0, 0, 0, 0));
		#endif
	#else
		hDC = GetDC(
			#if EDITOR
				hwnd = 
			#endif
			#if EDITOR
				CreateWindow( WINDOW_CLASS_NAME , 0, 
					WS_OVERLAPPEDWINDOW | WS_VISIBLE
					//WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
			, windowX , windowY , XRES, YRES, 0, 0, hInstance, 0)
			#else
				CreateWindow((LPCSTR)0xC018, 0, WS_POPUP | WS_VISIBLE, 0, 0, XRES, YRES, 0, 0, 0, 0)
			#endif
		);
		#if EDITOR
			// Hide icon
			SetClassLongPtr(hwnd, GCLP_HICON, 0);
			SetClassLongPtr(hwnd, GCLP_HICONSM, 0);
			SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE)&~WS_SIZEBOX&~WS_MAXIMIZEBOX
				&~WS_SYSMENU
			);
			//style &= ~WS_SYSMENU;
			BOOL useDarkMode = TRUE;
			DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
			SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		#endif
	#endif


	SetPixelFormat(hDC, ChoosePixelFormat(hDC, &pfd), &pfd);
	wglMakeCurrent(hDC, wglCreateContext(hDC));

	#if OPENGL_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		oglDebugMessageCallback(dbg_gl_message_callback, 0); 
	#endif

	#if VSYNC
		typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int interval);
		auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
		wglSwapIntervalEXT(1);
	#endif
	#if EDITOR
		editor_create_console();
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)editor_console_winapi_message_loop, NULL, 0, NULL);
	#endif
}

static int __forceinline add_program(unsigned int program_type, const char* str, const char* path) {
	#if OPENGL_DEBUG
		shader_strings[shader_count] = str;
		shader_paths[shader_count] = path;
		shader_types[shader_count] = program_type;
		shader_count++;
		return oglCreateShaderProgramv(program_type, 1, &str);
	#else
		return oglCreateShaderProgramv(program_type, 1, &str);
	#endif
}

#include "render.h"

#ifdef _DEBUG
int main(){
#else
#pragma code_seg(".main")
void entrypoint(void) {
#endif
	init_window();
	init_shaders();
	init_resources();

	music_init();
	do {
	#if EDITOR
	  MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) return 0;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	#else
		PeekMessage(0, 0, 0, 0, PM_REMOVE);
	#endif
		main_loop();
		do_editor_stuff();
		SwapBuffers(hDC);
	} while (
	#if OPENGL_DEBUG || EDITOR
		!editor_finished
	#else
		! (GetAsyncKeyState(VK_ESCAPE) || music_time > float(MUSIC_DURATION) - 0.1)
	#endif
	);


	#if EDITOR
		FreeConsole(); 
	#endif
	ExitProcess(0);
}


