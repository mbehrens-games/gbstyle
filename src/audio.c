/******************************************************************************/
/* audio.c (sdl audio code)                                                   */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "audio.h"

#include "apu.h"

#define AUDIO_FB_MAX_MS 50
#define AUDIO_FB_SIZE   (AUDIO_FB_MAX_MS * APU_OUT_SAMPLES_PER_MS)

/* audio frame buffer */
short         G_audio_frame_buffer[AUDIO_FB_SIZE];
unsigned int  G_audio_frame_num_samples;

/* we can switch these to static once we're not writing to a wave file... */
#if 0
static short        S_audio_frame_buffer[AUDIO_FB_SIZE];
static unsigned int S_audio_frame_num_samples;
#endif

/******************************************************************************/
/* audio_init()                                                               */
/******************************************************************************/
int audio_init()
{
  int k;

  for (k = 0; k < AUDIO_FB_SIZE; k++)
    G_audio_frame_buffer[k] = 0;

  G_audio_frame_num_samples = 0;

  return 0;
}

/******************************************************************************/
/* audio_deinit()                                                             */
/******************************************************************************/
int audio_deinit()
{
  

  return 0;
}

/******************************************************************************/
/* audio_update_frame()                                                       */
/******************************************************************************/
int audio_update_frame(unsigned short milliseconds)
{
  int k;

  if (milliseconds > AUDIO_FB_MAX_MS)
    milliseconds = AUDIO_FB_MAX_MS;

  G_audio_frame_num_samples = 0;

  for (k = 0; k < milliseconds * APU_OUT_SAMPLES_PER_MS; k++)
  {
    apu_update_out();

    G_audio_frame_buffer[k] = G_apu_out_left;
    G_audio_frame_num_samples += 1;
  }

  return 0;
}

