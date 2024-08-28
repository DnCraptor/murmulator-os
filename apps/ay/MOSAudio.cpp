#include "ayfly.h"

MOSAudio::MOSAudio(AYSongInfo *info) :
    AbstractAudio(info)
{
    songinfo = info;
//    stopping_thread = 0;
}

MOSAudio::~MOSAudio()
{
//    if(stopping_thread)
//    {
//        SDL_KillThread(stopping_thread);
//        stopping_thread = 0;
//    }
//    Stop();
}

bool MOSAudio::Start()
{
    if(!started)
    {/*
        SDL_AudioSpec fmt;
        SDL_memset(&fmt, 0, sizeof(SDL_AudioSpec));
        fmt.callback = MOSAudio::Play;
        fmt.channels = 2;
        fmt.format = AUDIO_S16;
        fmt.samples = 1024 * 2 * 2;
        fmt.freq = songinfo->sr;
        fmt.userdata = this;
        if(SDL_OpenAudio(&fmt, &fmt_out) < 0)
        {
            return false;
        }
        SDL_PauseAudio(0);*/
        started = true;
    }
    return started;

}

void MOSAudio::Stop()
{
    if(started)
    {/*
        started = false;
        SDL_PauseAudio(1);
        SDL_CloseAudio();*/
    }

}
/*
void MOSAudio::Play(void *udata, Uint8 *stream, int len)
{
    MOSAudio *audio = (MOSAudio *)udata;
    if(audio->songinfo->stopping)
    {
        audio->songinfo->stopping = false;
        audio->stopping_thread = SDL_CreateThread(MOSAudio::StoppingThread, audio);
        memset(stream, 0, len);
        return;
    }
    ay_rendersongbuffer(audio->songinfo, stream, len);
    //audio->songinfo->ay8910 [0].ayProcess(stream, len);
}

int MOSAudio::StoppingThread(void *arg)
{
    MOSAudio *audio = (MOSAudio *)arg;
    audio->stopping_thread = 0;
    audio->Stop();
    if(audio->songinfo->s_callback)
        audio->songinfo->s_callback(audio->songinfo->s_callback_arg);
    return 0;
}
*/