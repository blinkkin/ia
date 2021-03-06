#include "SdlWrapper.h"

#include "Engine.h"
#include <iostream>

#include <SDL_image.h>
#include <SDL_mixer.h>

#include "CommonData.h"
#include "Config.h"

using namespace std;

namespace SdlWrapper {

//------------------------------------------------------------------------LOCAL
namespace {

bool isInited = false;

}

//---------------------------------------------------------------------- GLOBAL
void init() {
  trace << "SdlWrapper::initSdl()..." << endl;

  isInited = true;

  if(SDL_Init(SDL_INIT_EVERYTHING) == -1) {
    trace << "[WARNING] Problem to init SDL, ";
    trace << "in SdlWrapper::initSdl()" << endl;
  }

  SDL_EnableUNICODE(1);

  if(IMG_Init(IMG_INIT_PNG) == -1) {
    trace << "[WARNING] Problem to init SDL_image";
    trace << ", in SdlWrapper::initSdl()" << endl;
  }

  const int     AUDIO_FREQ      = 44100;
  const Uint16  AUDIO_FORMAT    = MIX_DEFAULT_FORMAT;
  const int     AUDIO_CHANNELS  = 2;
  const int     AUDIO_BUFFERS   = 1024;

  if(Mix_OpenAudio(
        AUDIO_FREQ, AUDIO_FORMAT, AUDIO_CHANNELS, AUDIO_BUFFERS) == -1) {
    trace << "[WARNING] Problem to init SDL_mixer";
    trace << ", in SdlWrapper::initSdl()" << endl;
  }

  Mix_AllocateChannels(AUDIO_ALLOCATED_CHANNELS);

  trace << "SdlWrapper::initSdl() [DONE]" << endl;
}

void cleanup() {
  isInited = false;
  IMG_Quit();
  Mix_AllocateChannels(0);
  Mix_CloseAudio();
  SDL_Quit();
}

void sleep(const Uint32 DURATION) {
  if(isInited && Config::isBotPlaying() == false) {
    if(DURATION == 1) {
      SDL_Delay(DURATION);
    } else {
      const Uint32 WAIT_UNTIL = SDL_GetTicks() + DURATION;
      while(SDL_GetTicks() < WAIT_UNTIL) {SDL_PumpEvents();}
    }
  }
}

} //SdlWrapper
