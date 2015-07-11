/** PSP helper library ***************************************/
/**                                                         **/
/**                          audio.h                        **/
/**                                                         **/
/** This file contains definitions for the audio rendering  **/
/** library. It is based on the pspaudio library by Adresd  **/
/** and Marcus R. Brown, 2005.                              **/
/**                                                         **/
/** Akop Karapetyan 2007                                    **/
/*************************************************************/
#ifndef _PSP_AUDIO_H
#define _PSP_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define PSP_AUDIO_SAMPLE_ALIGN(s) (((s) + 63) & ~63)
#define PSP_AUDIO_SAMPLE_TRUNCATE(s) ((s) & ~63)
#define PSP_AUDIO_MAX_VOLUME      0x8000

typedef struct
{
  short Left;
  short Right;
} PspStereoSample;

typedef struct
{
  short Channel;
} PspMonoSample;

typedef void (*pspAudioCallback)(void *buffer, unsigned int *sample_count, void *userdata);

int  pspAudioInit(int sample_count, int stereo);
void pspAudioSetVolume(int channel, int left, int right);
void pspAudioSetChannelCallback(int channel, pspAudioCallback callback, void *userdata);
void pspAudioShutdown();
int  pspAudioOutputBlocking(void *buf, unsigned int length);
int  pspAudioOutput(void *buf, unsigned int length);
int  pspAudioGetSampleCount();

#ifdef __cplusplus
}
#endif

#endif // _PSP_AUDIO_H
