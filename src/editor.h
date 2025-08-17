#pragma once

#if EDITOR
	static HWND editor_hwnd_console;
	static float editor_mouse_x_ndc = 0;
	static float editor_mouse_y_ndc = 0;

	static float editor_mouse_x_ndc_shader = 0;
	static float editor_mouse_y_ndc_shader = 0;

	static bool editor_win_focused = false;
	static bool editor_gui_toggled = true;
	static bool editor_keys_pressed[20] = {};

	static bool editor_window_is_always_on_top = false;


	static bool editor_finished = false;
	static int editor_frame = 0;

	static float editor_loop_start = 0;
	static float editor_loop_end = MUSIC_DURATION - 0.000001;

	static bool editor_loop_popup_finished = false;
	static LARGE_INTEGER editor_timer_start, editor_timer_freq;
	static float editor_average_ms = 0.;

	static bool editor_just_started = true;

	static FILE* editor_ffmpeg = nullptr;
	static bool editor_is_recording = false;

	static bool editor_is_first_launch = true; // TODO: move this away from here

	#define key_space_down  editor_keys_pressed[0]
	#define key_left_down  editor_keys_pressed[1]
	#define key_right_down  editor_keys_pressed[2]
	#define key_lmb_down  editor_keys_pressed[3]
	#define key_aaaaa_down  editor_keys_pressed[4]
	#define key_s_down  editor_keys_pressed[5]
	#define key_l_down editor_keys_pressed[6]
	#define key_r_down editor_keys_pressed[7]
	#define key_v_down editor_keys_pressed[8]
	#define key_d_down editor_keys_pressed[9]
	#define key_m_down editor_keys_pressed[10]
	#define key_a_down editor_keys_pressed[11]
	#include <dbt.h>
#endif



static __forceinline void editor_print_to_console(const char* message) {
#if EDITOR
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(hConsole, message, lstrlenA(message), NULL, NULL);
#endif
}


static __forceinline void dbg_try_validate_shaders() {
#if EDITOR && VALIDATE_SHADERS_GLSLANG
	if (!(key_v_down || editor_just_started)) {
		return;
	}
		
		editor_print_to_console("               --------------------------- \n");
		editor_print_to_console("               ---- Shader Validation ---- \n");
		editor_print_to_console("               --------------------------- \n");

    // char shader_validation_errors[MAX_ERROR_SIZE] = {0};
    for (size_t i = 0; i < shader_count; i++) {	
        // Write shader to a temporary file
        //FILE* file = fopen("shader.glsl", "w");
        FILE* file;
				if(editor_just_started){
					file = fopen("shader.glsl", "w");
					if (!file) {
							MessageBoxA(0, "Failed to create temp shader file.", "Error", MB_OK | MB_ICONERROR);
							return;
					}
					fprintf(file, "%s", shader_strings[i]);
					fclose(file);
				} 

				const char* shaderSuffix;
				switch (shader_types[i]) {
						case GL_VERTEX_SHADER: shaderSuffix = "vert"; break; 
						case GL_FRAGMENT_SHADER: shaderSuffix = "frag"; break; 
						case GL_COMPUTE_SHADER: shaderSuffix = "comp"; break; 
						default: shaderSuffix = "unknown"; break;
				}

        char command[512];
				if(editor_just_started){
					snprintf(
						command, 
						sizeof(command), 
"\"glslangValidator.exe\" -e main --auto-map-bindings --auto-map-locations --glsl-version 460 --no-link -S %s shader.glsl",
						shaderSuffix
					);
				} else {
					snprintf(
						command, 
						sizeof(command), 
"\"glslangValidator.exe\" -e main --auto-map-bindings --auto-map-locations --glsl-version 460 --no-link -S %s \"%s\"",
						shaderSuffix,
						shader_paths[i]
					);
				}

        SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
        HANDLE hReadPipe, hWritePipe;
        CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0);

        STARTUPINFOA si = {sizeof(STARTUPINFO)};
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;

        PROCESS_INFORMATION pi;
        if (CreateProcessA(NULL, command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(hWritePipe);

            char buffer[(4096 * 40)] = {0};
            DWORD bytesRead;
            ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
            buffer[bytesRead] = '\0'; // Null-terminate

            CloseHandle(hReadPipe);

            if (strstr(buffer, "ERROR")) { // If output contains "ERROR", log it
            	MessageBoxA(0, "Validation fail", "Error", MB_OK | MB_ICONERROR);
                // strcat(shader_validation_errors, "------ SHADER ");
                // strcat(shader_validation_errors, shader_paths[i]);
                // strcat(shader_validation_errors, " ------\n");
                // strcat(shader_validation_errors, buffer);
                // strcat(shader_validation_errors, "\n");
            }
        		editor_print_to_console(shader_paths[i]);
						if(bytesRead < 10){
							editor_print_to_console("  -  OK");
						} else {
							editor_print_to_console(buffer);
						}
        		editor_print_to_console("\n");

            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            MessageBoxA(0, "Failed to run glslangValidator.exe", "Error", MB_OK | MB_ICONERROR);
            return;
        }
    }
		editor_print_to_console("\n");
	#endif
}


#if EDITOR


	void editor_create_console() {
		QueryPerformanceFrequency(&editor_timer_freq);
		if (!AllocConsole()) {
			// DWORD dwError = GetLastError();
			// char errorMessage[256];
			// snprintf(errorMessage, sizeof(errorMessage), "AllocConsole failed with error %lu", dwError);
			// MessageBox(NULL, errorMessage, "Error", MB_OK | MB_ICONERROR);
			// return;
		}
		editor_hwnd_console = GetConsoleWindow();
		ShowWindow(editor_hwnd_console, SW_SHOW);
		RECT rect;
		GetWindowRect(hwnd, &rect);
		BOOL useDarkMode = TRUE;
		DwmSetWindowAttribute(editor_hwnd_console, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
		SetWindowPos(editor_hwnd_console, HWND_TOP, rect.left, rect.bottom - 300, rect.right - rect.left, 300, SWP_NOZORDER);
		SetWindowLong(editor_hwnd_console, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE)&~WS_SIZEBOX&~WS_MAXIMIZEBOX
			&~WS_SYSMENU
		);

		SetForegroundWindow(editor_hwnd_console);
	}

	// Main message loop
	void editor_console_winapi_message_loop() {
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	LRESULT CALLBACK editor_winapi_window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
			switch (uMsg) {
				case WM_GETICON:
					return 0;
				case WM_QUERYDRAGICON:
					return 0;

				case WM_DEVICECHANGE:
					if (wParam == DBT_DEVICEREMOVECOMPLETE || wParam == DBT_DEVICEREMOVEPENDING) {
							music_kill();
							music_init(); 
					}
					break;
				case WM_DESTROY:
					PostQuitMessage(0);
					return 0;
						
				case WM_LBUTTONDOWN:  
					if(editor_win_focused){
						key_lmb_down = true;
					}
					return 0;
				case WM_KEYDOWN:
					if(editor_win_focused){
						if (wParam == VK_SPACE) {
							key_space_down = true;
						}
						if (wParam == VK_LEFT) {
							key_left_down = true;
						}
						if (wParam == VK_RIGHT) {
							key_right_down = true;
						}
						if (wParam == VK_OEM_3) {
							key_aaaaa_down = true;
						}
						if (wParam == 'S') {
							key_s_down = true;
						}
						if (wParam == 'L') {
							key_l_down = true;
						}
						if (wParam == 'R') {
							key_r_down = true;
						}
						if (wParam == 'V') {
							key_v_down = true;
						}
						if (wParam == 'D') {
							key_d_down = true;
						}
						if (wParam == 'M') {
							key_m_down = true;
						}
						if (wParam == 'A') {
							key_a_down = true;
						}
					}
					break;
			}
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
#endif

#if EDITOR
	void editor_print_to_console(const char* message);
#endif


static __forceinline void editor_start_timer() {
#if EDITOR
	QueryPerformanceCounter(&editor_timer_start);
#endif
}


static __forceinline void editor_end_timer(const char* label) {
#if EDITOR
	char out_buffer[1024];
	LARGE_INTEGER editor_timer_end;

	QueryPerformanceCounter(&editor_timer_end);

	double elapsed_ms = (double)(editor_timer_end.QuadPart - editor_timer_start.QuadPart) * 1000.0 / editor_timer_freq.QuadPart;
	
	sprintf(out_buffer, "%.2f ms %s", float(elapsed_ms), label);
	//wsprintfA(out_buffer, , elapsed_ms, label);

	editor_print_to_console(out_buffer);
	editor_print_to_console("\n");
#endif
}

static __forceinline void editor_do_fps_counter(){
#if EDITOR
	static bool intialized = false;


	//static LARGE_INTEGER timer_start;
	static LARGE_INTEGER last_time;
	static LARGE_INTEGER curr_time;


	QueryPerformanceCounter(&curr_time);


	//QueryPerformanceCounter(&timer_start);
	
	//if(!initia)


	if(intialized){
		double new_ms = (double)(curr_time.QuadPart - last_time.QuadPart) * 1000.0 / editor_timer_freq.QuadPart;

		float lerp_speed = 0.04;
		editor_average_ms = new_ms * lerp_speed + editor_average_ms * (1.-lerp_speed);
	}

	last_time = curr_time;

	intialized = true;
#endif
}


static void _inline editor_reload_from_disk() {
	#if EDITOR
	editor_start_timer();
	bool prev_shader_failed_compile = shader_failed_compile;
	shader_failed_compile = false;

	// char test_notif[] = "asrdgasdg";
	// MessageBoxA(NULL, test_notif, test_notif, MB_ICONWARNING | MB_CANCELTRYCONTINUE | MB_DEFBUTTON2);
	//printf("shader reload");
	time_t current_time = time(NULL);
	float MODIFICATION_THRESHOLD = 5;  // 5 seconds

	
	int temp_pids[100];
	bool reload_shader[100];
	bool reload_shader_success[100];


	for(int i = 0; i < shader_count; i++) {
		reload_shader[i] = false;
		reload_shader_success[i] = true;
	}

	bool success = true;
	for(int i = 0; i < shader_count; i++) {
		auto file_path = shader_paths[i];
		struct stat file_stat;

		stat(file_path, &file_stat);
		time_t current_time = time(NULL);
		double seconds_since_mod = difftime(current_time, file_stat.st_mtime);

		bool dont_reload = seconds_since_mod > MODIFICATION_THRESHOLD;


		#if EDITOR_FORCE_RELOAD_FROM_DISK
			if(editor_is_first_launch){
				dont_reload = false;
			}
		#endif

		if (dont_reload) {
				continue;
		}
		reload_shader[i] = true;
		FILE* file = fopen(shader_paths[i], "r");
		if (file) {
			// Read file contents
			fseek(file, 0, SEEK_END);
			long file_size = ftell(file);
			fseek(file, 0, SEEK_SET);


			char* content = (char*)malloc(file_size + 1);
			// char* content = (char*)malloc(file_size + 1);
			// char content[50000];
			//static char content[100000];
			size_t bytesRead = fread(content, 1, file_size, file);
			content[bytesRead] = '\0'; // Null-terminate the string
			// if(i == 0) {
			char* content_ptr = &content[0];
			temp_pids[i] = oglCreateShaderProgramv(shader_types[i], 1, &content_ptr);
			// } else {
				// pids[i] = oglCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, content);
			// }
			// printf("File content (%s):\n%s\n", shader_paths[i], content);
			free(content);
			fclose(file);
		} else {
			// Handle file open error
			fprintf(stderr, "Error: Unable to open file %s\n", shader_paths[i]);
		}
		

	}

	bool a_shader_reloaded = false;
	for(int i = 0; i < shader_count; i++){
		if(reload_shader[i] == false){
			continue;
		}
		int pid = temp_pids[i];
		int gl_temp_link_status;
		
		oglGetProgramiv( pid, GL_LINK_STATUS, &gl_temp_link_status);
		if (gl_temp_link_status == 0) {
			char log[1000];
			oglGetProgramInfoLog(pid, 1000 - 1, NULL, log);
			#if EDITOR
				editor_print_to_console("------- Shader Error -------\n");
				editor_print_to_console(shader_paths[i]);
				editor_print_to_console("\n");
				editor_print_to_console(log);
			#else
				printf(shader_paths[i]);
				printf("\n");
				printf("\n");
				printf(log);
				MessageBoxA(NULL, log, shader_paths[i], MB_HELP);
			#endif
			shader_failed_compile = true;
			success = false;
			reload_shader_success[i] = false;
		}
	}

	for(int i = 0; i < shader_count; i++) {
		if(reload_shader[i]){
			a_shader_reloaded = true;
			if(!reload_shader_success[i]){
				oglDeleteProgram(temp_pids[i]);
			} else {
				if(programs[i] == PROG_MUSIC){
					audio_shader_just_reloaded = true;
				}
				oglDeleteProgram(programs[i]);
				programs[i] = temp_pids[i];
				
				#if EDITOR
					editor_print_to_console("               --------------------------- \n");
					editor_print_to_console("               ------- SHADER RELOAD ----- \n");
					editor_print_to_console("               --------------------------- \n");
					editor_print_to_console(shader_paths[i]);
					editor_print_to_console("\n");
					editor_print_to_console("\n");
				#endif
			}
		}
	}

	if(a_shader_reloaded && !shader_failed_compile){
		//char char_buff[100];
		//sprintf("%f.2ms\n")
		editor_end_timer(" - shader compilation");
	}

	if(prev_shader_failed_compile == true && shader_failed_compile == false){
		#if EDITOR
			editor_print_to_console("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		#endif
	}


	#endif
}


static void _inline editor_try_reload() {
	#if OPENGL_DEBUG
		bool dont_reload_from_disk = !GetAsyncKeyState(VK_LCONTROL) || !GetAsyncKeyState(0x53);

		#if EDITOR_FORCE_RELOAD_FROM_DISK
			if(editor_is_first_launch){
				dont_reload_from_disk = false;
			}
		#endif

		if( dont_reload_from_disk ) {
			return;
		}

		Sleep(400); // Genius
		editor_reload_from_disk();


		// Ugly... To get audio reloaded from disk...
		// Ideally minifier should run on every actual launch .-.!!!
		#if EDITOR_FORCE_RELOAD_FROM_DISK
			if(editor_is_first_launch){
				music_render();
				music_editor_seek_and_play_buffer(music_time);
			}
		#endif

		editor_is_first_launch = false; // why here?
	#endif
}



// Global variables to store the input values

// Window procedure to handle the interactions with the custom window
LRESULT CALLBACK editor_loop_popup_window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
#if EDITOR
    switch (uMsg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 103) { // OK button clicked
                char buffer1[16], buffer2[16];
                GetWindowText(GetDlgItem(hwnd, 101), buffer1, sizeof(buffer1));
                GetWindowText(GetDlgItem(hwnd, 102), buffer2, sizeof(buffer2));
								editor_loop_start = atoi(buffer1);
								editor_loop_end = atoi(buffer2);
								editor_loop_popup_finished = true;
                PostMessage(hwnd, WM_CLOSE, 0, 0);
								DestroyWindow(hwnd);
                return 0;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
#endif
	return LRESULT(0);
}

// Function to create the input window and get two integers from the user
static __forceinline void editor_do_loop_popup(const char* title) {
#if EDITOR
		editor_loop_popup_finished = false;
    WNDCLASS wc = {0};
    wc.lpfnWndProc = editor_loop_popup_window_proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "InputDialogClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, "InputDialogClass", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                               CW_USEDEFAULT, CW_USEDEFAULT, 300, 400, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (!hwnd) {
        MessageBoxA(NULL, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
				return;
    }

    CreateWindow("STATIC", "First Value:", WS_VISIBLE | WS_CHILD, 10, 40, 200, 20, hwnd, NULL, NULL, NULL);
    CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 10, 70, 200, 20, hwnd, (HMENU)101, NULL, NULL);

    CreateWindow("STATIC", "Second Value:", WS_VISIBLE | WS_CHILD, 10, 100, 200, 20, hwnd, NULL, NULL, NULL);
    CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 10, 130, 200, 20, hwnd, (HMENU)102, NULL, NULL);

    CreateWindow("BUTTON", "OK", WS_VISIBLE | WS_CHILD, 10, 160, 50, 20, hwnd, (HMENU)103, NULL, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) && !editor_loop_popup_finished) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
#endif
}


// Read OpenGL framebuffer and send to FFMPEG
static __forceinline void editor_ffmpeg_capture_frame() {
#if EDITOR
	// PBOs for non-blocking readback
	static GLuint pboIds[2] = { 0, 0 };
	static int pboIndex = 0;
	static bool pboInitialized = false;

	if(!pboInitialized){
		oglGenBuffers(2, pboIds); // Generate 2 PBOs for double-buffering
		for (int i = 0; i < 2; i++) {
				oglBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[i]);
				oglBufferData(GL_PIXEL_PACK_BUFFER, XRES * YRES * 3, NULL, GL_STREAM_READ);
		}
		oglBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		pboInitialized = true;
	}

	{
    int nextIndex = (pboIndex + 1) % 2; // Swap PBOs
    glReadBuffer(GL_BACK);

    // Bind new PBO and read pixels into it asynchronously
    oglBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[pboIndex]);
    glReadPixels(0, 0, XRES, YRES, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    // Process previous PBO
    oglBindBuffer(GL_PIXEL_PACK_BUFFER, pboIds[nextIndex]);
    unsigned char* pixels = (unsigned char*)oglMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		if (pixels) {
			fwrite(pixels, 1, XRES * YRES * 3, editor_ffmpeg); // Directly write without flipping
			oglUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    //if (pixels) {
    //    // Flip image vertically (since OpenGL has bottom-left origin)
    //    unsigned char* flippedPixels = (unsigned char*)malloc(xres * yres * 3);
    //    for (int y = 0; y < yres; y++) {
    //        memcpy(flippedPixels + (yres - 1 - y) * xres * 3, pixels + y * xres * 3, xres * 3);
    //    }

    //    fwrite(flippedPixels, 1, xres * yres * 3, editor_ffmpeg);
    //    free(flippedPixels);

    //    oglUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    //}
    oglBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    pboIndex = nextIndex; // Swap buffers
	}
#endif
}


static void __forceinline do_editor_stuff(){
	// Focus main window
#if EDITOR
	dbg_try_validate_shaders();
	bool should_toggle_recording = key_r_down;
	if(editor_just_started){
		music_editor_seek_and_play_buffer(editor_loop_start);
	}
	#if DO_PERFECT_FFMPEG_CAPTURE
		if(editor_just_started){
			music_save_wav();
		}
		static bool finished_perfect_ffmpeg_capture = false;
		if(!finished_perfect_ffmpeg_capture){
			if(editor_just_started){
				music_editor_mute();
				should_toggle_recording = true;
			//} else if(editor_time > MUSIC_DURATION){
			//} else if(editor_time > MUSIC_DURATION - 0.2){
			} else if(editor_time > MUSIC_DURATION){
				should_toggle_recording = true;
				finished_perfect_ffmpeg_capture = true;
			}
		}

	#endif
	if(should_toggle_recording){
		if(!editor_is_recording){
			//editor_ffmpeg = _popen(
			//		"ffmpeg -y -f rawvideo -pixel_format rgb24 -video_size 1280x720 -r 60 -i - " // HARDCODED RESOLUTION
			//		"-c:v libx264 -preset fast -crf 18 -pix_fmt yuv420p output.mp4",
			//		"wb"
			//);

			editor_ffmpeg = _popen(
				"ffmpeg -y "                                // Overwrite output file if it exists
				"-f rawvideo "                              // Input format: raw pixel data
				"-thread_queue_size 512 "
				"-pixel_format rgb24 "                      // Pixel format: 24-bit RGB (8 bits per channel)
				"-video_size 1280x720 "                     // Input resolution (must match OpenGL framebuffer)
				"-r 60 "                                    // Frame rate: 30 FPS
				"-i - "                                     // Read input from stdin (piped from our program)
				#if DO_PERFECT_FFMPEG_CAPTURE
					"-i song.wav "  // Input WAV file
				#endif
				"-vf vflip "                                // Flip the image vertically (fix OpenGL bottom-to-top issue)
				//"-vf scale=3840:2160:flags=lanczos,vflip "  // Upscale to 4K with Lanczos filter and flip the image
				"-c:v libx264 "                             // Use H.264 codec for video compression
				"-preset fast "                             // Optimize for faster encoding

				// ----	QUALITY ---- //
				"-crf 18 "                                  // Quality setting (lower = better, 18 = visually lossless)

				"-pix_fmt yuv420p "                         // Convert RGB to YUV 4:2:0 for playback compatibility

				"-g 120 "                                   // Keyframe interval for YouTube (2 seconds at 60 FPS)
				"-c:a aac "                                 // Audio codec: AAC
				"-b:a 320k "                                // Audio bit rate: 320 kbps
				//"-ar 48000 "                                // Audio sample rate: 48 kHz
				"-ar 44100 "                                // Audio sample rate: 48 kHz

				//"outputc.mp4",                               // Output file name
				//"output_bw10.mp4",                               // Output file name
				"output_bw10_v2.mp4",                               // Output file name
				"wb"
			);
			if (!editor_ffmpeg) {
					MessageBox(NULL, "Failed to start FFMPEG!", "Error", MB_ICONERROR);
					//return 1;
			}
			editor_is_recording = true;
		} else {
			_pclose(editor_ffmpeg);
			editor_is_recording = false;
			#if DO_PERFECT_FFMPEG_CAPTURE
					MessageBox(NULL, "Finished FFMPEG capture!", "Woo", MB_ICONERROR);
			#endif
		}
	}

	if(editor_is_recording){
		editor_ffmpeg_capture_frame();
	}

	double music_time = music_get_time_seconds();

	// --- Toggle gui
	if(key_aaaaa_down){
		editor_gui_toggled = !editor_gui_toggled;
	}

	// --- Mouse position
	{
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		ScreenToClient(hwnd, &cursorPos); 
		
    RECT rect;
    GetClientRect(hwnd, &rect); 

    int winWidth = rect.right - rect.left;
    int winHeight = rect.bottom - rect.top;


		#if FULLSCREEN
			winWidth = xres;
			winHeight = yres;
		#else

		#endif

    editor_mouse_x_ndc = (2.0f * cursorPos.x) / winWidth - 1.0f;
    editor_mouse_y_ndc = 1.0f - (2.0f * cursorPos.y) / winHeight;

		if(editor_win_focused && (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0){
			editor_mouse_x_ndc_shader = editor_mouse_x_ndc;
			editor_mouse_y_ndc_shader = editor_mouse_y_ndc;
		}
	}
	{
		editor_win_focused = GetForegroundWindow() == hwnd;
	}
	// --- Position console
	{
	 	RECT rect;
	 	GetWindowRect(hwnd, &rect);

		#if !FULLSCREEN
			SetWindowPos(editor_hwnd_console, HWND_TOP, rect.left, rect.bottom, rect.right - rect.left, 300, SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW);
		#endif
	}

	static bool is_picking_loop_pos = false;
	if(key_l_down){
		//editor_do_loop_popup("Loop start");

		if(is_picking_loop_pos){
			editor_loop_end = editor_time;
			is_picking_loop_pos = false;
			editor_print_to_console("               --------------------------- \n");
			editor_print_to_console("               ------- STARTED LOOP ------ \n");
			editor_print_to_console("               --------------------------- \n");
			editor_print_to_console("\n");
		} else {
			if(editor_loop_start > 0.002 || editor_loop_end < MUSIC_DURATION - 0.001){
				editor_loop_start = 0;
				editor_loop_end = MUSIC_DURATION;
				editor_print_to_console("               --------------------------- \n");
				editor_print_to_console("               -------- RESET LOOP ------- \n");
				editor_print_to_console("               --------------------------- \n");
				editor_print_to_console("\n");
			} else {
				editor_loop_start = editor_time;
				is_picking_loop_pos = true;
			}
		}

		if(editor_loop_start == 0 && editor_loop_end == 0){
			editor_loop_end = MUSIC_DURATION;
		}
		if(editor_loop_start < 0){
			editor_loop_start = 0;
		}

		if(editor_loop_end > MUSIC_DURATION){
			editor_loop_end = MUSIC_DURATION;
		}
		
	}
	if(key_m_down){
		// Toggle mute
		editor_muted_music = !editor_muted_music;
		if(editor_muted_music){
				music_editor_seek_and_play_buffer(editor_time);
		} else {
				music_editor_seek_and_play_buffer(editor_time);
		}
	}


	// --- Toggle pause
	{
		if(key_space_down){ 
			editor_paused = !editor_paused;
			if(!editor_paused){		// --- unpause
				if(!editor_muted_music){
					//music_editor_unmute();
					music_editor_seek_and_play_buffer(editor_time);
				}
			} else {			// --- pause
				editor_time = music_time;
				if(!editor_muted_music){
					//music_editor_mute();
					music_editor_seek_and_play_buffer(music_time);
				}
			}
		}
		if(!editor_paused){
			#if DO_PERFECT_FFMPEG_CAPTURE
				editor_time += 1./60.;
			#else
				editor_time = music_time;
			#endif
		}
	}


	// --- Update titlebar
	{
		char buffer[4650];
		char buffer_min_secs[30];
		const char* str_failed_compile = shader_failed_compile ? "!!!!!! FAILED COMPILE !!!!!!" : "";
		const char* str_win_always_on_top = editor_window_is_always_on_top ? "[A] " : "";
		const char* str_muted = editor_muted_music ? "[M] " : "";
		const char* str_paused = editor_paused ? "[P] " : "";
		const char* str_looping = editor_loop_start > 0 || editor_loop_end < MUSIC_DURATION - 0.6 ? "[L] " : "";

		int minutes = editor_time / 60;
		int seconds = int(editor_time) % 60;

		sprintf(buffer_min_secs, "%d:%02d", minutes, seconds);


		//editor_average_ms
    //sprintf(buffer, "%.2fs ------- %.2fms ------- %.2fps                 loop: %.2f - %.2f ", editor_time, editor_average_ms, 1000./editor_average_ms, editor_loop_start, editor_loop_end);

		//------- %.2fms ------- %.2fps                

		if(is_picking_loop_pos){
			sprintf(
				buffer, "%s %s %s %s %s %.2fs    L[%.2f - %.2f]  !! PICKING LOOP !! PICKING LOOP !! PICKING LOOP !! PICKING LOOP !! PICKING LOOP !! PICKING LOOP !! PICKING LOOP !!", 
				str_win_always_on_top,
				str_muted,
				str_looping,
				str_paused,
				str_failed_compile,
				editor_time, 
				editor_loop_start, 
				editor_loop_end, 
				editor_average_ms, 
				1000./editor_average_ms, 
				editor_mouse_x_ndc_shader, 
				editor_mouse_y_ndc_shader
			);

		} else {
			sprintf(
				buffer, "%s %s %s %s %s %s  ||   %.2fs   ||    L[%.2f - %.2f]   ||    %.2fms, %.2ffps                                                [%.2fx%.2f]             ", 
				str_win_always_on_top,
				str_muted,
				str_looping,
				str_paused,
				str_failed_compile,
				buffer_min_secs,
				editor_time, 
				editor_loop_start, 
				editor_loop_end, 
				editor_average_ms, 
				1000./editor_average_ms, 
				editor_mouse_x_ndc_shader, 
				editor_mouse_y_ndc_shader
			);
		}



		SetWindowText(hwnd, buffer);
		//SetWindowText(editor_hwnd_console, buffer);
		//char[2]
		const char* text_nothing = " ";
		SetWindowText(editor_hwnd_console, text_nothing);
	}


	// --- Draw Gui

	#if !DO_PERFECT_FFMPEG_CAPTURE
		//if(false)
	{
		oglUseProgram(0);

		if(editor_gui_toggled){
			float pos_y = -1;
			float pos_x = editor_time/float(MUSIC_DURATION)*2.0 - 1.;

			auto draw_point = [&](float pos_x, float pos_y, float point_size){
				glPointSize(point_size);
				glColor3f(1,1,1);
				glBegin(GL_POINTS);  
				glVertex2f(pos_x, pos_y);
				glEnd();
			};
			draw_point(
				editor_time/float(MUSIC_DURATION)*2.0 - 1., 
				-1.,
				30.0
			);

			float h = -0.98;
			float rad = 4.;

			draw_point(
				editor_loop_start/float(MUSIC_DURATION)*2.0 - 1., 
				h,
				rad
			);
			draw_point(
				editor_loop_end/float(MUSIC_DURATION)*2.0 - 1., 
				h,
				rad
			);



		}
		//glRects(-1, -1, 1, 1);

		if(shader_failed_compile){
			glColor3f(1,0.3,0.8);
			glPointSize(70.0f); // Set point size (in pixels)
			glBegin(GL_POINTS);  
			glVertex2f(-0.8, 0.8);  // Point at the center
			glEnd();
		}
	}
	#endif
	
	if(key_a_down){
    DWORD exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    BOOL isTopMost = (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;

    SetWindowPos(
        hwnd,
        isTopMost ? HWND_NOTOPMOST : HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
    );

    SetWindowPos(
        editor_hwnd_console,
        isTopMost ? HWND_NOTOPMOST : HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
    );

		editor_window_is_always_on_top = !editor_window_is_always_on_top;
	}

	// --- Seek
	{
		bool is_mouse_seeking = (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
		bool is_kbd_seeking = key_left_down || key_right_down;
		bool should_seek = is_kbd_seeking || is_mouse_seeking;

		const bool mouse_is_inside_window = 
			editor_mouse_x_ndc > -1 &&
			editor_mouse_x_ndc < 1 &&
			editor_mouse_y_ndc > -1 &&
			editor_mouse_y_ndc < 1;

		if(should_seek && editor_win_focused && (is_kbd_seeking || (is_mouse_seeking && mouse_is_inside_window))){
			double target_time;
			if(is_mouse_seeking){
				target_time = MUSIC_DURATION * (editor_mouse_x_ndc + 1) / 2;
			} else if (is_kbd_seeking){
				target_time = editor_time;
				float seek_amt_s = 5.;
				target_time += float(key_right_down) * seek_amt_s;
				target_time -= float(key_left_down) * seek_amt_s;
			}

			if(target_time < 0){
				target_time = 0;
			}
			if(editor_paused){
				editor_time = target_time;
			}
			music_editor_seek_and_play_buffer(target_time);
		}

		if( music_time > editor_loop_end - 0.04){
			music_editor_seek_and_play_buffer(editor_loop_start);
		}
	}

	// --- Re-render audio
	if(audio_shader_just_reloaded && !editor_just_started){
		music_render();
		if(!editor_paused && !editor_muted_music){
			music_editor_seek_and_play_buffer(music_time);
		}
		editor_print_to_console("\n");
	}


	// --- Save wav
	{
		if(key_s_down){
			music_save_wav();
		}
	}

	// --- Reset keys & vars
	{
		for(int i = 0; i < 20; i++){
			editor_keys_pressed[i] = false;
		}
		audio_shader_just_reloaded = false;
	}
	editor_just_started = false;
	editor_frame++;
#endif
	
}


