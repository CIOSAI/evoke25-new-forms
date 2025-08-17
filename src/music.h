#pragma once



using MUSIC_SAMPLE_TYPE = float;
static constexpr int MUSIC_SAMPLE_RATE = 44100;
static constexpr int MUSIC_MAX_SAMPLES = MUSIC_SAMPLE_RATE*MUSIC_DURATION;
static constexpr int MUSIC_BUFFER_SIZE = MUSIC_MAX_SAMPLES * 2 + 100;

#include <mmsystem.h>
#include <mmreg.h>
#if EDITOR
	#include <dbt.h>
#endif 


#pragma data_seg(".musicout")
	static float music_lpSoundBuffer[MUSIC_BUFFER_SIZE];
	#if EDITOR
		static float music_lpSoundBufferEmpty[MUSIC_BUFFER_SIZE] = {};
	#endif

#pragma data_seg(".waveout")
static HWAVEOUT music_hWaveOut;

#pragma data_seg(".wavefmt")
	static WAVEFORMATEX music_WaveFMT = {
		WAVE_FORMAT_IEEE_FLOAT,
		2,                                   // channels
		MUSIC_SAMPLE_RATE,                         // samples per sec
		MUSIC_SAMPLE_RATE*sizeof(MUSIC_SAMPLE_TYPE) * 2, // bytes per sec
		sizeof(MUSIC_SAMPLE_TYPE) * 2,             // block alignment;
		sizeof(MUSIC_SAMPLE_TYPE) * 8,             // bits per sample
		0                                    // extension not needed
	};

#pragma data_seg(".wavehdr")
static WAVEHDR music_WaveHDR = {
	(LPSTR)music_lpSoundBuffer, MUSIC_MAX_SAMPLES * sizeof(MUSIC_SAMPLE_TYPE) * 2, 0, 0, 0, 0, 0, 0
};

#pragma data_seg(".mmtime")
static MMTIME music_MMTime = {
	TIME_SAMPLES, 0
};


#define MUSIC_OFFSET
#if EDITOR
	#define MUSIC_OFFSET + music_offset_samples
	static int music_offset_samples = 0;
#endif


static __forceinline int music_get_time_samples() {
	waveOutGetPosition(music_hWaveOut, &music_MMTime, sizeof(MMTIME));
	return music_MMTime.u.sample MUSIC_OFFSET;
}

static __forceinline double music_get_time_seconds() {
	return double(music_get_time_samples()) / double(44100);
}



void editor_start_timer();
void editor_end_timer(const char* label);
void editor_print_to_console(const char* message);

static __forceinline void music_render() { 
	editor_print_to_console("               --------------------------- \n");
	editor_print_to_console("               ------ AUDIO RERENDER ----- \n");
	editor_print_to_console("               --------------------------- \n");

	editor_start_timer();
	oglUseProgram(PROG_MUSIC);
	constexpr int samples_cnt = MUSIC_SAMPLE_RATE * MUSIC_DURATION; 
	constexpr int bytes_per_samp = 4 * 2; // two f32

	constexpr int local_thread_cnt = 256;

	constexpr int total_group_dispatches = samples_cnt / local_thread_cnt + 1; 
	//constexpr int gpu_buff_start_offs_bytes = 4000000*bytes_per_samp;
	constexpr int gpu_buff_start_offs_bytes = 0;

	constexpr int total_byte_count  = samples_cnt * bytes_per_samp;

	// 20727000 samples
	// 80965 group dispatches

#if ANTI_TDR
		constexpr int group_disp_batch_cnt = 1024;
		constexpr int bytes_per_disp = group_disp_batch_cnt * local_thread_cnt * bytes_per_samp;

		for(int group_disp_idx = 0; group_disp_idx <= total_group_dispatches; group_disp_idx += group_disp_batch_cnt){
			oglUniform1i(0, local_thread_cnt * group_disp_idx);
			oglDispatchCompute(group_disp_batch_cnt, 1, 1);
			//float* cpu_write_ptr = &music_lpSoundBuffer[0];
			//cpu_write_ptr += group_disp_idx * local_thread_cnt * 2;
			//glFlush();
			glFinish();
		}
		//oglGetNamedBufferSubData(ssbo, gpu_buff_start_offs_bytes, total_byte_count, &music_lpSoundBuffer);
#else
		oglDispatchCompute(total_group_dispatches, 1, 1);
#endif
	oglGetNamedBufferSubData(ssbo, gpu_buff_start_offs_bytes, total_byte_count, &music_lpSoundBuffer);

	editor_end_timer("\n");
}


static void __forceinline music_kill() { 
#if EDITOR
	waveOutReset(music_hWaveOut);
	waveOutClose(music_hWaveOut);
#endif
}


static void __forceinline music_init() { 
	#if !EDITOR_FORCE_RELOAD_FROM_DISK
		music_render();
	#endif
	waveOutOpen(&music_hWaveOut, WAVE_MAPPER, &music_WaveFMT, NULL, 0, CALLBACK_NULL);
	waveOutPrepareHeader(music_hWaveOut, &music_WaveHDR, sizeof(music_WaveHDR));
	waveOutWrite(music_hWaveOut, &music_WaveHDR, sizeof(music_WaveHDR));
	
	#if EDITOR
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = { 0 };
    NotificationFilter.dbcc_size = sizeof(NotificationFilter);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

		RegisterDeviceNotification(hwnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
	#endif
}

static void __forceinline music_editor_seek_and_play_buffer(double pos_secs) { 
#if EDITOR
	waveOutReset(music_hWaveOut);
	waveOutUnprepareHeader(music_hWaveOut,&music_WaveHDR,sizeof(WAVEHDR));
	if(pos_secs >= MUSIC_DURATION - 0.5){
		pos_secs = MUSIC_DURATION;
	}

	MUSIC_SAMPLE_TYPE* buf = music_lpSoundBuffer;
	if(editor_paused || editor_muted_music){
		buf = music_lpSoundBufferEmpty;
	}

	music_get_time_samples();
	int new_time_samples = (pos_secs) * 44100;
	buf += new_time_samples*2; // seek
	music_offset_samples = new_time_samples;
	music_WaveHDR.lpData=(LPSTR)(buf);
	music_WaveHDR.dwBufferLength=(MUSIC_MAX_SAMPLES - new_time_samples) * sizeof(MUSIC_SAMPLE_TYPE) * 2;

	waveOutPrepareHeader(music_hWaveOut,&music_WaveHDR,sizeof(WAVEHDR));
	waveOutWrite(music_hWaveOut,&music_WaveHDR,sizeof(WAVEHDR));
#endif
}

#if EDITOR
	void editor_print_to_console(const char* message);
#endif


void music_save_wav() {
#if EDITOR
    FILE* file = fopen("song.wav", "wb");
    if (!file) {
        MessageBox(NULL, "Error opening file!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Define WAV header
    struct {
        char chunkID[4];      // "RIFF"
        DWORD chunkSize;      // File size - 8
        char format[4];       // "WAVE"
        char subchunk1ID[4];  // "fmt "
        DWORD subchunk1Size;  // 16 (PCM)
        WORD audioFormat;     // 3 (IEEE Float PCM)
        WORD numChannels;     // 2 (Stereo)
        DWORD sampleRate;     // 44100 Hz
        DWORD byteRate;       // SampleRate * NumChannels * BitsPerSample / 8
        WORD blockAlign;      // NumChannels * BitsPerSample / 8
        WORD bitsPerSample;   // 32-bit float
        char subchunk2ID[4];  // "data"
        DWORD subchunk2Size;  // NumSamples * NumChannels * BitsPerSample / 8
    } wavHeader;

    // Fill WAV header
    memcpy(wavHeader.chunkID, "RIFF", 4);
    memcpy(wavHeader.format, "WAVE", 4);
    memcpy(wavHeader.subchunk1ID, "fmt ", 4);
    memcpy(wavHeader.subchunk2ID, "data", 4);

    wavHeader.subchunk1Size = 16;
    wavHeader.audioFormat = 3;  // IEEE Float PCM
    wavHeader.numChannels = 2;
    wavHeader.sampleRate = MUSIC_SAMPLE_RATE;
    wavHeader.bitsPerSample = 32;
    wavHeader.blockAlign = (wavHeader.bitsPerSample / 8) * wavHeader.numChannels;
    wavHeader.byteRate = wavHeader.sampleRate * wavHeader.blockAlign;
    wavHeader.subchunk2Size = MUSIC_MAX_SAMPLES * wavHeader.blockAlign;
    wavHeader.chunkSize = 36 + wavHeader.subchunk2Size;

    // Write WAV header
    fwrite(&wavHeader, sizeof(wavHeader), 1, file);

    // Write the sound buffer
    fwrite(music_lpSoundBuffer, sizeof(MUSIC_SAMPLE_TYPE), MUSIC_MAX_SAMPLES * 2, file);

    fclose(file);

		editor_print_to_console("----- WAV file created succesfully -----\n \n \n");
    //MessageBox(NULL, "WAV file saved successfully!", "Success", MB_OK);
#endif
}

