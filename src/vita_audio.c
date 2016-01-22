/** PSP helper library ***************************************/
/**                                                         **/
/**                          audio.c                        **/
/**                                                         **/
/** This file contains the audio rendering library. It is   **/
/** based almost entirely on the pspaudio library by Adresd **/
/** and Marcus R. Brown, 2005.                              **/
/**                                                         **/
/** Akop Karapetyan 2007                                    **/
/*************************************************************/
#include "vita_audio.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/audioout.h>

#define AUDIO_CHANNELS       1
#define DEFAULT_SAMPLE_COUNT 512

static int AudioReady;
static volatile int StopAudio;
static int SampleCount;
static int Stereo;

typedef struct {
  int ThreadHandle;
  int Handle;
  int LeftVolume;
  int RightVolume;
  pspAudioCallback Callback;
  void *Userdata;
} ChannelInfo;

static int DirectHandle;
static ChannelInfo AudioStatus[AUDIO_CHANNELS];
static short *AudioBuffer[AUDIO_CHANNELS][2];
static unsigned int AudioBufferSamples[AUDIO_CHANNELS][2];

static int AudioChannelThread(int args, void *argp);
static void FreeBuffers();
static int OutputBlocking(unsigned int channel,
  unsigned int vol1, unsigned int vol2, void *buf, int length);

int pspAudioInit(int sample_count, int stereo)
{
  int i, j, failed;

  Stereo = stereo;
  StopAudio = 0;
  AudioReady = 0;
  DirectHandle = -1;

  /* Check sample count */
  if (sample_count < 0) sample_count = DEFAULT_SAMPLE_COUNT;
  sample_count = PSP_AUDIO_SAMPLE_ALIGN(sample_count);

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    AudioStatus[i].Handle = -1;
    AudioStatus[i].ThreadHandle = -1;
    AudioStatus[i].LeftVolume = PSP_AUDIO_MAX_VOLUME;
    AudioStatus[i].RightVolume = PSP_AUDIO_MAX_VOLUME;
    AudioStatus[i].Callback = NULL;
    AudioStatus[i].Userdata = NULL;

    for (j = 0; j < 2; j++)
    {
      AudioBuffer[i][j] = NULL;
      AudioBufferSamples[i][j] = 0;
    }
  }

  DirectHandle = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, DEFAULT_SAMPLE_COUNT,
    48000, (Stereo) ? SCE_AUDIO_OUT_MODE_STEREO : SCE_AUDIO_OUT_MODE_MONO);

  /* No callback */
  if ((SampleCount = sample_count) == 0)
    return 1;

  SampleCount = sample_count;

  /* Initialize buffers */
  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    for (j = 0; j < 2; j++)
    {
      if (!(AudioBuffer[i][j] = (short*)malloc(SampleCount
        * (Stereo ? sizeof(PspStereoSample) : sizeof(PspMonoSample)))))
      {
        FreeBuffers();
        return 0;
      }

      AudioBufferSamples[i][j] = SampleCount;
    }
  }

  /* Initialize channels */
  for (i = 0, failed = 0; i < AUDIO_CHANNELS; i++)
  {
    AudioStatus[i].Handle = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, SampleCount,
      48000, (Stereo) ? SCE_AUDIO_OUT_MODE_STEREO : SCE_AUDIO_OUT_MODE_MONO);

    if (AudioStatus[i].Handle < 0)
    {
      failed = 1;
      break;
    }
  }

  if (failed)
  {
    for (i = 0; i < AUDIO_CHANNELS; i++)
    {
      if (AudioStatus[i].Handle != -1)
      {
        sceAudioOutReleasePort(AudioStatus[i].Handle);
        AudioStatus[i].Handle = -1;
      }
    }

    if (DirectHandle != -1)
      sceAudioOutReleasePort(DirectHandle);

    FreeBuffers();
    return 0;
  }

  AudioReady = 1;

  char label[16];
  strcpy(label, "audiotX");

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    label[6] = '0' + i;
    AudioStatus[i].ThreadHandle =
      sceKernelCreateThread(label, (void*)&AudioChannelThread, 0x10000100, 0x10000,
        0, 0, NULL);

    if (AudioStatus[i].ThreadHandle < 0)
    {
      AudioStatus[i].ThreadHandle = -1;
      failed = 1;
      break;
    }

    if (sceKernelStartThread(AudioStatus[i].ThreadHandle, sizeof(i), &i) != 0)
    {
      failed = 1;
      break;
    }
  }

  if (failed)
  {
    StopAudio = 1;
    for (i = 0; i < AUDIO_CHANNELS; i++)
    {
      if (AudioStatus[i].ThreadHandle != -1)
      {
        //sceKernelWaitThreadEnd(AudioStatus[i].threadhandle,NULL);
        sceKernelDeleteThread(AudioStatus[i].ThreadHandle);
      }

      AudioStatus[i].ThreadHandle = -1;
    }

    AudioReady = 0;
    FreeBuffers();
    return 0;
  }

  return SampleCount;
}

void pspAudioShutdown()
{
  int i;
  AudioReady = 0;
  StopAudio = 1;

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    if (AudioStatus[i].ThreadHandle != -1)
    {
      //sceKernelWaitThreadEnd(AudioStatus[i].threadhandle,NULL);
      sceKernelDeleteThread(AudioStatus[i].ThreadHandle);
    }

    AudioStatus[i].ThreadHandle = -1;
  }

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    if (AudioStatus[i].Handle != -1)
    {
      sceAudioOutReleasePort(AudioStatus[i].Handle);
      AudioStatus[i].Handle = -1;
    }
  }

  if (DirectHandle != -1)
  {
    sceAudioOutReleasePort(DirectHandle);
    DirectHandle = -1;
  }

  FreeBuffers();
}

void pspAudioSetVolume(int channel, int left, int right)
{
  AudioStatus[channel].LeftVolume = left;
  AudioStatus[channel].RightVolume = right;
}

static int AudioChannelThread(int args, void *argp)
{
  volatile int bufidx = 0;
  int channel = *(int*)argp;
  int i;
  unsigned short *ptr_m;
  unsigned int *ptr_s;
  void *bufptr;
  unsigned int samples;
  pspAudioCallback callback;

  for (i = 0; i < 2; i++)
    memset(AudioBuffer[channel][bufidx], 0, SampleCount
      * ((Stereo) ? sizeof(PspStereoSample) : sizeof(PspMonoSample)));

  while (!StopAudio)
  {
    callback = AudioStatus[channel].Callback;
    bufptr = AudioBuffer[channel][bufidx];
    samples = AudioBufferSamples[channel][bufidx];

    if (callback) callback(bufptr, &samples, AudioStatus[channel].Userdata);
    else
    {
      if (Stereo) for (i = 0, ptr_s = bufptr; i < samples; i++) *(ptr_s++) = 0;
      else for (i = 0, ptr_m = bufptr; i < samples; i++) *(ptr_m++) = 0;
    }

	  OutputBlocking(channel, AudioStatus[channel].LeftVolume,
      AudioStatus[channel].RightVolume, bufptr, samples);

    bufidx = (bufidx ? 0 : 1);
  }

  sceKernelExitThread(0);
  return 0;
}

int pspAudioOutputBlocking(void *buf, unsigned int length)
{
  if (DirectHandle < 0) return -1;

  int vols[2]={PSP_AUDIO_MAX_VOLUME,PSP_AUDIO_MAX_VOLUME};
  sceAudioOutSetConfig(DirectHandle, length, -1, -1);
  sceAudioOutSetVolume(DirectHandle,SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH,vols);
  return sceAudioOutOutput(DirectHandle, buf);
}

int pspAudioOutput(void *buf, unsigned int length)
{
  if (DirectHandle < 0) return -1;

  int vols[2]={PSP_AUDIO_MAX_VOLUME,PSP_AUDIO_MAX_VOLUME};
  sceAudioOutSetConfig(DirectHandle, length, -1, -1);
  sceAudioOutSetVolume(DirectHandle,SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH,vols);
  return sceAudioOutOutput(DirectHandle, buf);
}

static int OutputBlocking(unsigned int channel,
  unsigned int vol1, unsigned int vol2, void *buf, int length)
{
  if (!AudioReady) return -1;
  if (channel >= AUDIO_CHANNELS) return -1;
  if (vol1 > PSP_AUDIO_MAX_VOLUME) vol1 = PSP_AUDIO_MAX_VOLUME;
  if (vol2 > PSP_AUDIO_MAX_VOLUME) vol2 = PSP_AUDIO_MAX_VOLUME;

  int vols[2]={vol1,vol2};
  sceAudioOutSetConfig(AudioStatus[channel].Handle, length, -1, -1);
  sceAudioOutSetVolume(AudioStatus[channel].Handle,SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH,vols);
  return sceAudioOutOutput(AudioStatus[channel].Handle,buf);
}

static void FreeBuffers()
{
  int i, j;

  for (i = 0; i < AUDIO_CHANNELS; i++)
  {
    for (j = 0; j < 2; j++)
    {
      if (AudioBuffer[i][j])
      {
        free(AudioBuffer[i][j]);
        AudioBuffer[i][j] = NULL;
      }
    }
  }
}

void pspAudioSetChannelCallback(int channel, pspAudioCallback callback, void *userdata)
{
  volatile ChannelInfo *pci = &AudioStatus[channel];
  pci->Callback = NULL;
  pci->Userdata = userdata;
  pci->Callback = callback;
}

int pspAudioGetSampleCount()
{
  return SampleCount;
}
