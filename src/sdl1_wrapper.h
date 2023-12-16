#define SDL_Renderer SDL_Surface
#define SDL_Texture SDL_Surface
#define SDL_RenderPresent SDL_Flip
#define SDL_GameController SDL_Joystick
#define SDL_GameControllerClose SDL_JoystickClose
#define SDL_AudioDeviceID SDL_AudioSpec
#define SDL_UnlockTexture SDL_UnlockSurface

#define SDL_SCANCODE_RALT SDLK_RALT
#define SDL_SCANCODE_RETURN SDLK_RETURN
#define SDL_SCANCODE_LCTRL SDLK_LCTRL

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
