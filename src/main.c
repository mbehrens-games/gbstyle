/******************************************************************************/
/* gbstyle (prototype code for Felisynth) - No Shinobi Knows Me 2026          */
/******************************************************************************/

/******************************************************************************/
/* main.c                                                                     */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "apu.h"
#include "audio.h"
#include "fileio.h"

/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char *argv[])
{
  int k;

  audio_init();
  apu_reset();

  /* just try writing out some stuff */
  export_wav_open_file("test_01.wav");
  export_wav_write_header();

  for (k = 0; k < 60; k++)
  {
    if (k % 3 == 0)
      audio_update_frame(16);
    else
      audio_update_frame(17);

    export_wav_write_block(&G_audio_frame_buffer[0], G_audio_frame_num_samples);
  }

  export_wav_close_file();

  return 0;
}
