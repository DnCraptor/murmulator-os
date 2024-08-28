#ifndef MOSSOUND_H_
#define MOSSOUND_H_

//#include "SDL.h"
//#include "SDL_thread.h" 

class AbstractAudio;

class MOSAudio : public AbstractAudio
{
public:
	MOSAudio(AYSongInfo *info);
	virtual ~MOSAudio();
	virtual bool Start();
	virtual void Stop();
private:
//	static void Play(void *udata, Uint8 *stream, int len);
//	SDL_AudioSpec fmt_out;
//	SDL_Thread *stopping_thread;
//	static int StoppingThread(void *arg);
};

#endif /*MOSSOUND_H_*/
