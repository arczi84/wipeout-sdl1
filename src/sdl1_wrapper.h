#define SDL_Renderer SDL_Surface
#define SDL_Texture SDL_Surface
#define SDL_Window SDL_Surface
#define SDL_RenderPresent SDL_Flip
#define SDL_GameController SDL_Joystick
#define SDL_GameControllerClose SDL_JoystickClose
#define SDL_AudioDeviceID SDL_AudioSpec
#define SDL_UnlockTexture SDL_UnlockSurface

//keys
#define SDL_SCANCODE_RALT SDLK_RALT
#define SDL_SCANCODE_RETURN SDLK_RETURN
#define SDL_SCANCODE_LCTRL SDLK_LCTRL
//gamepad
#define SDL_CONTROLLERBUTTONDOWN SDL_JOYBUTTONDOWN
#define SDL_CONTROLLERBUTTONUP SDL_JOYBUTTONUP
#define SDL_CONTROLLERAXISMOTION SDL_JOYAXISMOTION
#define SDL_CONTROLLER_BUTTON_MAX 7
#define SDL_CONTROLLER_AXIS_MAX 2
#define cbutton button
#define caxis jaxis
#define JOY_DEAD_ZONE 256

typedef enum
{
    SDL_CONTROLLER_BUTTON_INVALID = -1,
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT//,
    //SDL_CONTROLLER_BUTTON_MAX
} SDL_GameControllerButton;

float joyAxisValue(int value) {
    if (value > -JOY_DEAD_ZONE && value < JOY_DEAD_ZONE)
        return 0.0f;
    return value / 32767.0f;
}

static int keycode_to_scancode[] = {
    0, 
	1, 
	2, 
	3, 
	4, 
	5, 
	6, 
	42, 
	42,
	43, 
	10, 
	11, 
	12, 
	40, //ENTER
	8,  //BACKSPACE
	15, 
	16, 
	17, 
	18, 
	19, 
	20, 
	21, 
	22, 
	23, 
	24, 
	25, 
	26, 
	41, //ESC
	28, 
	29, 
	30, 
	31, 
	44, 
	33, 
	34, 
	35, 
	36, 
	37, 
	38, 
	39, 
	40, 
	41, 
	42, 
	43, 
	54, 
	120,//45, 
	55, 
	56, 
	39, 
	30, 
	31, 
	32, 
	33, 
	34, 
	35, 
	36, 
	37, 
	38, 
	58, 
	59, 
	60, 
	61, 
	62, 
	63, 
	64, 
	65, 
	66, 
	67, 
	68, 
	69, 
	70, 
	71, 
	72, 
	73, 
	74, 
	75, 
	76, 
	77, 
	78, 
	79, 
	80, 
	81, 
	82, 
	83, 
	84, 
	85, 
	86, 
	87, 
	88, 
	89, 
	90, 
	91, 
	92, 
	93, 
	94, 
	95, 
	96, 
	4, 
	5, 
	6, 
	7, 
	8, 
	9, 
	10, 
	11, 
	12, 
	13, 
	14, 
	15, 
	16, 
	17, 
	18, 
	19, 
	20, 
	21, 
	22, 
	23, 
	24, 
	25, 
	26, 
	27, //esc
	28, 
	29, 
	123, 
	124, 
	125, 
	126, 
	127, 
	128, 
	129, 
	130, 
	131, 
	132, 
	133, 
	134, 
	135, 
	136, 
	137, 
	138, 
	139, 
	140, 
	141, 
	142, 
	143, 
	144, 
	145, 
	146, 
	147, 
	148, 
	149, 
	150, 
	151, 
	152, 
	153, 
	154, 
	155, 
	156, 
	157, 
	158, 
	159, 
	160, 
	161, 
	162, 
	163, 
	164, 
	165, 
	166, 
	167, 
	168, 
	169, 
	170, 
	171, 
	172, 
	173, 
	174, 
	175, 
	176, 
	177, 
	178, 
	179, 
	180, 
	181, 
	182, 
	183, 
	184, 
	185, 
	186, 
	187, 
	188, 
	189, 
	190, 
	191, 
	192, 
	193, 
	194, 
	195, 
	196, 
	197, 
	198, 
	199, 
	200, 
	201, 
	202, 
	203, 
	204, 
	205, 
	206, 
	207, 
	208, 
	209, 
	210, 
	211, 
	212, 
	213, 
	214, 
	215, 
	216, 
	217, 
	218, 
	219, 
	220, 
	221, 
	222, 
	223, 
	224, 
	225, 
	226, 
	227, 
	228, 
	229, 
	230, 
	231, 
	232, 
	233, 
	234, 
	235, 
	236, 
	237, 
	238, 
	239, 
	240, 
	241, 
	242, 
	243, 
	244, 
	245, 
	246, 
	247, 
	248, 
	249, 
	250, 
	251, 
	252, 
	253, 
	254, 
	255, 
	256, 
	257, 
	258, 
	259, 
	260, 
	261, 
	262, 
	263, 
	264, 
	265, 
	266, 
	267, 
	268, 
	269, 
	270, 
	271, 
	272, 
	82,//273,
	81,//274, 
	79,//275, 
	80,//276, 
	277, 
	278, 
	279, 
	280, 
	281, 
	282, 
	283, 
	284, 
	285, 
	286, 
	287, 
	288, 
	289, 
	290, 
	291, 
	292, 
	293
};

char* SDL_GetBasePath(void) 
{
#ifdef AMIGA
    return "PROGDIR:";
#else
    return "";
#endif
}

char* SDL_GetPrefPath(const char* org, const char* app) 
{ 
    return (char*)org; 
}

void
SDL_GetWindowSize(SDL_Window * window, int *w, int *h)
{
    if (w) {
        *w = window->w;
    }
    if (h) {
        *h = window->h;
    }
}

int SDL_RenderCopy(SDL_Renderer*   renderer,
                   SDL_Texture*    texture,
                   const SDL_Rect* srcrect,
                   const SDL_Rect* dstrect)
{
   //Blit the surface
    return SDL_BlitSurface( texture, NULL, renderer, NULL );
}

void SDL_DestroyWindow(SDL_Surface* Surface)
{
	SDL_FreeSurface(Surface);
}
void SDL_DestroyTexture(SDL_Surface* Surface)
{
	SDL_FreeSurface(Surface);
}
void SDL_DestroyRenderer(SDL_Surface* Surface)
{
	SDL_FreeSurface(Surface);
}
/*
SDL_AudioDeviceID SDL_OpenAudioDevice(const char* device, int iscapture,
                                      const SDL_AudioSpec* desired,
                                      SDL_AudioSpec*       obtained,
                                      int                  allowed_changes)
{

  return SDL_OpenAudio(const_cast<SDL_AudioSpec*>(desired), const_cast<SDL_AudioSpec*>(obtained));

}
*/

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, Uint32 format, int access, int w, int h)
{
    return SDL_CreateRGBSurface(renderer->flags, w, h, 32, renderer->format->Rmask, renderer->format->Gmask, renderer->format->Bmask, renderer->format->Amask);
}

void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on)
{
    SDL_PauseAudio(pause_on);
}

void SDL_CloseAudioDevice(SDL_AudioDeviceID dev)
{
    SDL_CloseAudio();
}

void SDL_SetWindowTitle(SDL_Surface* Surface, const char* title)
{
    SDL_WM_SetCaption(title, title);
}

#include <time.h>

static SDL_bool ticks_started = SDL_FALSE;
static struct timeval start_tv;

void
SDL_TicksInit(void)
{
    if (ticks_started) {
        return;
    }
    ticks_started = SDL_TRUE;

    /* Set first ticks value */
    {
        gettimeofday(&start_tv, NULL);
    }
}

void
SDL_TicksQuit(void)
{
    ticks_started = SDL_FALSE;
}

Uint64
SDL_GetPerformanceCounter(void)
{
    Uint64 ticks;
    if (!ticks_started) {
        SDL_TicksInit();
    }

    struct timeval now;

    gettimeofday(&now, NULL);
    ticks = now.tv_sec;
    ticks *= 1000000;
    ticks += now.tv_usec;

    return (ticks);
} 

Uint64
SDL_GetPerformanceFrequency(void)
{
    if (!ticks_started) {
        SDL_TicksInit();
    }
     
    return 1000000;
}   
