#pragma once


#define post_shader_prog programs[0]
#define sh_sandbox programs[1]
#define sh_song programs[2]
#define sh_jizz programs[3]
#define sh_i2f programs[4]


const float BPM=90.0;
const float BEAT_DUR=60./BPM;


static unsigned int framebuffers[20];
static unsigned int textures[20];

#define fb_main framebuffers[3]
#define fb_scnd framebuffers[5]
#define fb_back framebuffers[4]
#define fb_petridish framebuffers[6]
#define fb_r framebuffers[0]
#define fb_g framebuffers[1]
#define fb_b framebuffers[2]

#define tex_main textures[3]
#define tex_scnd textures[5]
#define tex_back textures[4]
#define tex_petridish textures[6]
#define tex_r textures[0]
#define tex_g textures[1]
#define tex_b textures[2]



static void compClear() {
	float cols[4] = {0.0f, 0.0f ,0.0f ,0.0f};
	for(int i = 4; i <= 6; i++){
		//oglClearTexSubImage( textures[i], 0, 0, 0, 0, XRES, YRES, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &cols[0] );
	}
}

static void __forceinline init_shaders() {
		post_shader_prog = add_program(GL_FRAGMENT_SHADER, post_frag, "src/shaders/post.frag");
		sh_sandbox = add_program(GL_FRAGMENT_SHADER, scene_sandbox_frag, "src/shaders/scene_sandbox.frag");
		sh_song = add_program(GL_COMPUTE_SHADER, song_comp, "src/shaders/song.comp");
		sh_jizz = add_program(GL_COMPUTE_SHADER, scene_jizz_comp, "src/shaders/scene_jizz.comp");
		sh_i2f = add_program(GL_FRAGMENT_SHADER, scene_i2f_frag, "src/shaders/scene_i2f.frag");
		int _programs[] = {programs[0], programs[1], programs[2], programs[3], programs[4]};
		dbg_check_shader_compilation(_programs);
		dbg_try_validate_shaders();
}

static void __forceinline init_resources() {
	for(int i = 0; i <= 10; i++){
		textures[i] = i;
		glBindTexture(GL_TEXTURE_2D, i);
		if(i <= 2){
			oglTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, XRES, YRES);
			oglBindImageTexture(i, i, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
			oglBindTextureUnit(i,i);
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			
			oglCreateFramebuffers(1, &framebuffers[i]);
			oglNamedFramebufferTexture(
				framebuffers[i],
				GL_COLOR_ATTACHMENT0,
				textures[i],
				0
			);
		}
	}
	oglCreateBuffers(1, &ssbo);
	oglNamedBufferStorage(ssbo, 292144000,0,0); // 
	oglBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
}

static void __forceinline pre_loop() { }

static unsigned int framee = 0;
static void __forceinline main_loop() {
	editor_do_fps_counter();
	editor_try_reload();
	#if EDITOR
		music_time = editor_time;
	#else
		music_time = music_get_time_seconds();
	#endif
    static int rseed = 1339;
	framee++;



	{
			oglBindTextureUnit(0, tex_r);
			oglBindTextureUnit(1, tex_g);
			oglBindTextureUnit(2, tex_b);
			compClear();
			oglUseProgram(sh_jizz);
			oglUniform1f(oglGetUniformLocation(sh_jizz, "u_t"), music_time);
			oglUniform1i(oglGetUniformLocation(sh_jizz, "F"), framee);
			oglDispatchCompute(XRES/256, YRES, 1);
			oglMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	{
		oglBindFramebuffer(GL_FRAMEBUFFER, fb_main);
		oglUseProgram(sh_i2f);
		glRects(-1, -1, 1, 1);
	}

	{
		oglBindTextureUnit(3, tex_main);
		oglBindTextureUnit(4, tex_back);
		oglBindFramebuffer(GL_FRAMEBUFFER, fb_petridish);

		oglUseProgram(sh_sandbox);
		oglUniform1f(oglGetUniformLocation(sh_sandbox, "u_t"), music_time);
		oglUniform1i(oglGetUniformLocation(sh_sandbox, "reaction_diffusion"), true);
		glRects(-1, -1, 1, 1);

		
		oglBlitNamedFramebuffer(
			fb_petridish, fb_back, 0, 0, XRES, YRES, 0, 0, XRES, YRES, GL_COLOR_BUFFER_BIT, GL_NEAREST
		);
	}

	{
		oglBindFramebuffer(GL_FRAMEBUFFER, fb_scnd);

		oglUseProgram(sh_sandbox);
		oglUniform1i(oglGetUniformLocation(sh_sandbox, "reaction_diffusion"), false);
		glRects(-1, -1, 1, 1);
	}
		
	{
		oglBindTextureUnit(5, tex_scnd);
		oglBindFramebuffer(GL_FRAMEBUFFER, 0);

		oglUseProgram(post_shader_prog);

		glRects(-1, -1, 1, 1);
	}


		//oglUseProgram(PROG_RENDER);
		//oglUniform1f(1, music_time);
		////oglUniform2f(2, xres, yres);
		//#if EDITOR
		//	oglUniform2f(3, editor_mouse_x_ndc_shader, editor_mouse_y_ndc_shader);
		//#endif
		//oglDispatchCompute(XRES/16, YRES/16, 1);
		////frame += 1;

		//oglUseProgram(PROG_POST);
		////oglUniform1f(1, music_time);
		////oglUniform2f(2, xres, yres);
		////	oglUniform1i(0, frame); // unused
		////OglUniform1f(1, music_time);
		//glRects(-1, -1, 1, 1);
}
