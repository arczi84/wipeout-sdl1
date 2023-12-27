#include <SDL/SDL.h>
#include <sdl1_wrapper.h>

#include "platform.h"
#include "input.h"
#include "system.h"
#include "utils.h"
#include "mem.h"

#ifdef __AMIGA__
unsigned long __stack = 1024 * 1024;
static const char *version __attribute__((used)) = "$VER: wipEout 1.0.2 "__DATE__" port by Arczi";
#endif


int width, height;
bool fullscreen = false;

static uint64_t perf_freq = 0;
static bool wants_to_exit = false;
static SDL_AudioSpec audio_device;
static SDL_Joystick *gamepad;

static void (*audio_callback)(/*float*/int16_t *buffer, uint32_t len) = NULL;
static char *path_assets = "";
static char *path_userdata = "";
static char *temp_path = NULL;


uint8_t platform_sdl_gamepad_map[] = {
	[SDL_CONTROLLER_BUTTON_A] = INPUT_GAMEPAD_A,
	[SDL_CONTROLLER_BUTTON_B] = INPUT_GAMEPAD_B,
	[SDL_CONTROLLER_BUTTON_X] = INPUT_GAMEPAD_X,
	[SDL_CONTROLLER_BUTTON_Y] = INPUT_GAMEPAD_Y,
//	[SDL_CONTROLLER_BUTTON_BACK] = INPUT_GAMEPAD_SELECT,
	[SDL_CONTROLLER_BUTTON_START] = INPUT_GAMEPAD_START,
	[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = INPUT_GAMEPAD_L_SHOULDER,
	[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = INPUT_GAMEPAD_R_SHOULDER,
	[SDL_CONTROLLER_BUTTON_DPAD_UP] = INPUT_GAMEPAD_DPAD_UP,
	[SDL_CONTROLLER_BUTTON_DPAD_DOWN] = INPUT_GAMEPAD_DPAD_DOWN,
	[SDL_CONTROLLER_BUTTON_DPAD_LEFT] = INPUT_GAMEPAD_DPAD_LEFT,
	[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] = INPUT_GAMEPAD_DPAD_RIGHT,
	[SDL_CONTROLLER_BUTTON_MAX] = INPUT_INVALID
};

uint8_t platform_sdl_axis_map[] = {
	[SDL_CONTROLLER_AXIS_LEFTX] = INPUT_GAMEPAD_L_STICK_LEFT,
	[SDL_CONTROLLER_AXIS_LEFTY] = INPUT_GAMEPAD_L_STICK_UP,
	[SDL_CONTROLLER_AXIS_RIGHTX] = INPUT_GAMEPAD_R_STICK_LEFT,
	[SDL_CONTROLLER_AXIS_RIGHTY] = INPUT_GAMEPAD_R_STICK_UP,
	[SDL_CONTROLLER_AXIS_TRIGGERLEFT] = INPUT_GAMEPAD_L_TRIGGER,
	[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = INPUT_GAMEPAD_R_TRIGGER,
	[SDL_CONTROLLER_AXIS_MAX] = INPUT_INVALID
};


void platform_exit(void) {
	wants_to_exit = true;
}

SDL_GameController *platform_find_gamepad(void) {
	/*
	for (int i = 0; i < SDL_NumJoysticks(); i++) {
	if (SDL_IsGameController(i)) {
			return SDL_GameControllerOpen(i);
		}
	}*/

	return NULL;
}


void platform_pump_events(void) {
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		// Detect ALT+Enter press to toggle fullscreen
		if (
			ev.type == SDL_KEYDOWN && 
			keycode_to_scancode[ev.key.keysym.sym] == SDL_SCANCODE_RETURN &&
			(ev.key.keysym.mod & (KMOD_LALT | KMOD_RALT))
		) {
			platform_set_fullscreen(!platform_get_fullscreen());
		}

		// Input Keyboard
		else if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
			//printf("key sdl1 = %d \n", ev.key.keysym.sym);
			//printf("scancode sdl1 = %d \n", ev.key.keysym.scancode);
			int code = keycode_to_scancode[ev.key.keysym.sym]; //ev.key.keysym.scancode;
			float state = ev.type == SDL_KEYDOWN ? 1.0 : 0.0;

			if (code >= SDL_SCANCODE_LCTRL && code <= SDL_SCANCODE_RALT) {
				int code_internal = code - SDL_SCANCODE_LCTRL + INPUT_KEY_LCTRL;
				//printf("key_mod = %d \n", code_internal);
				input_set_button_state(code_internal, state);
			}
			else if (code > 0 && code < INPUT_KEY_MAX) {
            /* switch(code)
                {
				    case SDLK_UP: code = INPUT_KEY_UP; break;
                    case SDLK_DOWN: code = INPUT_KEY_DOWN; break;
                    case SDLK_LEFT: code = INPUT_KEY_LEFT; break;
                    case SDLK_RIGHT: code = INPUT_KEY_RIGHT; break;
					case SDLK_RETURN: code = INPUT_KEY_RETURN; break;
					case SDLK_a: code = INPUT_KEY_A; break;
					case SDLK_x: code = INPUT_KEY_X; break;
					case SDLK_z: code = INPUT_KEY_Z; break;
					case SDLK_s: code = INPUT_KEY_S; break;
					case SDLK_ESCAPE: SDL_Quit(); break;
					default: break;
				}*/
				//printf("key = %d \n", code);
				input_set_button_state(code, state);
			}
		}
#if 0
		else if (ev.type == SDL_TEXTINPUT) {
			input_textinput(ev.text.text[0]);
		}

		// Gamepads connect/disconnect
		else if (ev.type == SDL_CONTROLLERDEVICEADDED) {
			gamepad = SDL_GameControllerOpen(ev.cdevice.which);
		}
		else if (ev.type == SDL_CONTROLLERDEVICEREMOVED) {
			if (gamepad && ev.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad))) {
				SDL_GameControllerClose(gamepad);
				gamepad = platform_find_gamepad();
			}
		}

#endif				
		// Input Gamepad Buttons
		else if (
			ev.type == SDL_CONTROLLERBUTTONDOWN || 
			ev.type == SDL_CONTROLLERBUTTONUP
		) {
			if (ev.cbutton.button < SDL_CONTROLLER_BUTTON_MAX) {
				button_t button = platform_sdl_gamepad_map[ev.cbutton.button];
				printf("Button %d pressed\n", ev.jbutton.button);
				if (button != INPUT_INVALID) {
					float state = ev.type == SDL_CONTROLLERBUTTONDOWN ? 1.0 : 0.0;
					
					input_set_button_state(button, state);
				}
			}
		}
#if 1
		// Input Gamepad Axis
		else if (ev.type == SDL_CONTROLLERAXISMOTION) {
			float state = (float)ev.caxis.value  / 32767.0f;
			printf("Gamepad axis %d pressed\n", ev.caxis.axis);
			printf("Gamepad axis.value %d pressed\n", ev.caxis.value);
			printf("Gamepad state %f pressed\n", state);
			if (ev.caxis.axis < SDL_CONTROLLER_AXIS_MAX) {
				int code = platform_sdl_axis_map[ev.caxis.axis];
							printf("Gamepad key = %d \n", code);
				if (
					code == INPUT_GAMEPAD_L_TRIGGER || 
					code == INPUT_GAMEPAD_R_TRIGGER
				) {
					input_set_button_state(code, state);
				}
				else if (state > 0) {
					input_set_button_state(code, 0.0);
					input_set_button_state(code+1, state);
				}
				else {
					input_set_button_state(code, -state);
					input_set_button_state(code+1, 0.0);
				}
			}
		}
#else
		else if (event.type == SDL_JOYAXISMOTION)
                   /* case SDL_JOYBUTTONDOWN: {
                       joyIndex = joyGetIndex(ev.jbutton.which);
                       JoyKey key = joyCodeToJoyKey(ev.jbutton.button);
                       Input::setJoyDown(joyIndex, key, 1);
                       break;
                    }
                    case SDL_JOYBUTTONUP: {
                        joyIndex = joyGetIndex(ev.jbutton.which);
                        JoyKey key = joyCodeToJoyKey(ev.jbutton.button);
                        Input::setJoyDown(joyIndex, key, 0);
                        break;
                    }*/
                   /* case SDL_JOYAXISMOTION:*/ {
                        //joyIndex = joyGetIndex(ev.jaxis.which);
                        switch (ev.jaxis.axis)
                        {
                            // In the classic joystick interface we know what axis changed by it's number,
                            // they have no names like on the fancy GameController interface. 
                            case 0: joyL.x = joyAxisValue(ev.jaxis.value); break;
                            case 1: joyL.y = joyAxisValue(ev.jaxis.value); break;
                            case 2: joyR.x = joyAxisValue(ev.jaxis.value); break;
                            case 3: joyR.y = joyAxisValue(ev.jaxis.value); break;
                        }
                        //Input::setJoyPos(joyIndex, jkL, joyDir(joyL));
                        //Input::setJoyPos(joyIndex, jkR, joyDir(joyR));
                        break;
                    }
#endif					
#if 0
		// Mouse buttons
		else if (
			ev.type == SDL_MOUSEBUTTONDOWN ||
			ev.type == SDL_MOUSEBUTTONUP
		) {
			button_t button = INPUT_BUTTON_NONE;
			switch (ev.button.button) {
				case SDL_BUTTON_LEFT: button = INPUT_MOUSE_LEFT; break;
				case SDL_BUTTON_MIDDLE: button = INPUT_MOUSE_MIDDLE; break;
				case SDL_BUTTON_RIGHT: button = INPUT_MOUSE_RIGHT; break;
				default: break;
			}
			if (button != INPUT_BUTTON_NONE) {
				float state = ev.type == SDL_MOUSEBUTTONDOWN ? 1.0 : 0.0;
				input_set_button_state(button, state);
			}
		}

		// Mouse wheel
		else if (ev.type == SDL_MOUSEWHEEL) {
			button_t button = ev.wheel.y > 0 
				? INPUT_MOUSE_WHEEL_UP
				: INPUT_MOUSE_WHEEL_DOWN;
			input_set_button_state(button, 1.0);
			input_set_button_state(button, 0.0);
		}

		// Mouse move
		else if (ev.type == SDL_MOUSEMOTION) {
			input_set_mouse_pos(ev.motion.x, ev.motion.y);
		}
#endif
		// Window Events
		if (ev.type == SDL_QUIT) {
			wants_to_exit = true;
		}
		/*else if (
			ev.type == SDL_WINDOWEVENT &&
			(
				ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
				ev.window.event == SDL_WINDOWEVENT_RESIZED
			)
		) {
			system_resize(platform_screen_size());
		}*/
	}
}
//double == fast
double  platform_now() {
	uint64_t perf_counter = SDL_GetPerformanceCounter();
	return (double )perf_counter / (double )perf_freq;
}

bool platform_get_fullscreen(void) {
	//return SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
}

void platform_set_fullscreen(bool fullscreen) {
	if (fullscreen) {
		/*int32_t display = SDL_GetWindowDisplayIndex(window);
		
		SDL_DisplayMode mode;
		SDL_GetDesktopDisplayMode(display, &mode);
		SDL_SetWindowDisplayMode(window, &mode);
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);*/
		SDL_ShowCursor(SDL_DISABLE);
	}
	else {
		//SDL_SetWindowFullscreen(window, 0);
		SDL_ShowCursor(SDL_ENABLE);
	}
}

void platform_audio_callback(void* userdata, uint8_t* stream, int len) {
	if (audio_callback) {
		audio_callback(/*(float *)*/(int16_t*)stream, len/sizeof(int16_t));
	}
	else {
		memset(stream, 0, len);
	}
}

void platform_set_audio_mix_cb(void (*cb)(int16_t *buffer, uint32_t len)) {
	audio_callback = cb;
	SDL_PauseAudioDevice(audio_device, 0);
}


uint8_t *platform_load_asset(const char *name, uint32_t *bytes_read) {
	char *path = strcat(strcpy(temp_path, path_assets), name);
	return file_load(path, bytes_read);
}

uint8_t *platform_load_userdata(const char *name, uint32_t *bytes_read) {
	char *path = strcat(strcpy(temp_path, path_userdata), name);
	if (!file_exists(path)) {
		*bytes_read = 0;
		return NULL;
	}
	return file_load(path, bytes_read);
}

uint32_t platform_store_userdata(const char *name, void *bytes, int32_t len) {
	char *path = strcat(strcpy(temp_path, path_userdata), name);
	return file_store(path, bytes, len);
}

#if defined(RENDERER_GL_LEGACY) // ----------------------------------------------------
	#define PLATFORM_WINDOW_FLAGS SDL_OPENGL

	void platform_video_init(void) {
		#if defined(USE_GLES2)
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		#endif

		static SDL_Surface *renderer;
		const int flags = SDL_OPENGL| (fullscreen ? SDL_FULLSCREEN : 0);
		SDL_WM_SetCaption("WipEout", "WipEout");

		renderer = SDL_SetVideoMode(window_width, window_height, 16, flags);
	}

	void platform_prepare_frame(void) {
		// nothing
	}

	void platform_video_cleanup(void) {
	}

	void platform_end_frame(void) {
		SDL_GL_SwapBuffers();
#ifdef AMIGA		
		WaitTOF();
#endif
	}

	vec2i_t platform_screen_size(void) {
		return vec2i(window_width, window_height);
	}


#elif defined(RENDERER_SOFTWARE) // ----------------------------------------------
	#define PLATFORM_WINDOW_FLAGS 0
	static SDL_Surface *renderer;
	static SDL_Surface *screenbuffer = NULL;
	static void *screenbuffer_pixels = NULL;
	static int screenbuffer_pitch;
#ifdef __AMIGA__
	static vec2i_t screenbuffer_size; //gcc4
	static vec2i_t screen_size;
#else
	static vec2i_t screenbuffer_size = vec2i(0, 0); 
	static vec2i_t screen_size = vec2i(0, 0);
#endif

	void platform_video_init(void) {
		const int flags = SDL_SWSURFACE/*|SDL_DOUBLEBUF*/ | (fullscreen ? SDL_FULLSCREEN : 0);
		SDL_WM_SetCaption("WipEout", "WipEout");

		renderer = SDL_SetVideoMode(window_width, window_height, 32, flags);
	}

	void platform_video_cleanup(void) {
		if (screenbuffer) {
			SDL_DestroyTexture(screenbuffer);
		}
		SDL_DestroyRenderer(renderer);
	}

	void platform_prepare_frame(void) {
		if (screen_size.x != screenbuffer_size.x || screen_size.y != screenbuffer_size.y) {
			if (screenbuffer) {
				SDL_DestroyTexture(screenbuffer);
			}

			static uint32_t rmask, gmask, bmask, amask;
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				rmask = 0xff000000;
				gmask = 0x00ff0000;
				bmask = 0x0000ff00;
				amask = 0x000000ff;
			#else
				rmask = 0xff;
				bmask = 0xFF00;
				gmask = 0xFF0000;
				amask = 0xFF000000;
			#endif

			screenbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, screen_size.x, screen_size.y, 32, bmask, gmask, rmask, amask);
			
			screenbuffer_size = screen_size;
		}
		SDL_LockSurface(screenbuffer);
		screenbuffer_pitch = screenbuffer->pitch;
		screenbuffer_pixels = screenbuffer->pixels;
	}

	void platform_end_frame(void) {
		screenbuffer_pixels = NULL;
		SDL_UnlockTexture(screenbuffer);
		SDL_RenderCopy(renderer, screenbuffer, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	rgba_t *platform_get_screenbuffer(int32_t *pitch) {
		*pitch = screenbuffer_pitch;
		return screenbuffer_pixels;
	}

	vec2i_t platform_screen_size(void) {

		screen_size = vec2i(renderer->w, renderer->h);
		return screen_size;
	}

#else
	#error "Unsupported renderer for platform SDL"
#endif

#ifndef AMIGA// GCC_VERSION >= 4
int WinMain
#else
int main
#endif
(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO| SDL_INIT_JOYSTICK);

	window_width = 640;
	window_height = 360;

    // Sprawdzenie liczby argumentów
    if (argc >= 4) {
        // Ustawienie fullscreen
        if (strcmp(argv[1], "-fullscreen") == 0) {
            fullscreen = true;
        }

        // Ustawienie szerokości
        window_width = atoi(argv[2]);

        // Ustawienie wysokości
        window_height = atoi(argv[3]);
    } else {
        // Informacja o poprawnym użyciu programu
        printf("Usage: %s [-fullscreen] [width] [height]\n", argv[0]);
        //return 1; // Zakończ program z kodem błędu
    }

    // Tutaj możesz użyć wartości fullscreen, width i height do konfiguracji programu
    printf("Fullscreen: %s\n", (fullscreen ? "Yes" : "No"));
    printf("Width: %d\n", window_width);
    printf("Height: %d\n", window_height);

	// Figure out the absolute asset and userdata paths. These may either be
	// supplied at build time through -DPATH_ASSETS=.. and -DPATH_USERDATA=..
	// or received at runtime from SDL. Note that SDL may return NULL for these.
	// We fall back to the current directory (i.e. just "") in this case.

	char *sdl_path_assets = NULL;
	#ifdef PATH_ASSETS
		path_assets = TOSTRING(PATH_ASSETS);
	#else
		sdl_path_assets = SDL_GetBasePath();
		if (sdl_path_assets) {
			path_assets = sdl_path_assets;
		}
	#endif

	char *sdl_path_userdata = NULL;
	#ifdef PATH_USERDATA
		path_userdata = TOSTRING(PATH_USERDATA);
	#else
		sdl_path_userdata = SDL_GetPrefPath("phoboslab", "data");
		if (sdl_path_userdata) {
			path_userdata = sdl_path_userdata;
		}
	#endif

	// Reserve some space for concatenating the asset and userdata paths with
	// local filenames.
	temp_path = mem_bump(max(strlen(path_assets), strlen(path_userdata)) + 64);

#if 0
	// Load gamecontrollerdb.txt if present.
	// FIXME: Should this load from userdata instead?
	char *gcdb_path = strcat(strcpy(temp_path, path_assets), "gamecontrollerdb.txt");
	int gcdb_res = SDL_GameControllerAddMappingsFromFile(gcdb_path);
	if (gcdb_res < 0) {
		printf("Failed to load gamecontrollerdb.txt\n");
	}
	else {
		printf("load gamecontrollerdb.txt\n");
	}
#endif

	//gamepad = platform_find_gamepad();
	gamepad = SDL_JoystickOpen(0);

	if (gamepad) {
		fprintf(stdout, "Using joystick '%s'\n", SDL_JoystickName(0));
		fprintf(stdout, "Number of Buttons: %d\n", SDL_JoystickNumButtons(gamepad));
	}
	perf_freq = SDL_GetPerformanceFrequency();
	SDL_AudioSpec audio_device;
	memset(&audio_device, 0, sizeof(audio_device));
	audio_device.freq = 44100;
	audio_device.format = AUDIO_S16SYS;
	audio_device.channels = 2;
	audio_device.samples = 2048;
	audio_device.callback = platform_audio_callback;
	audio_device.userdata = NULL;
	if (SDL_OpenAudio(&audio_device, 0) == 0) {
		SDL_PauseAudio(0);
	}

	platform_video_init();
	system_init();

	while (!wants_to_exit) {
		platform_pump_events();
		platform_prepare_frame();
		system_update();
		platform_end_frame();
	}
	system_cleanup();
	platform_video_cleanup();


	if (gamepad) {
		SDL_GameControllerClose(gamepad);
	}

	if (sdl_path_assets) {
		//SDL_free(sdl_path_assets); //crash
	}
	if (sdl_path_userdata) {
		//SDL_free(sdl_path_userdata); //crash
	}

	SDL_CloseAudioDevice(audio_device);
	SDL_Quit();
	return 0;
}
