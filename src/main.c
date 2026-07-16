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
#include "midi.h"
#include "wav.h"

/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char *argv[])
{
  int k;

  audio_init();
  apu_reset();

  /* load midi file */
  midi_import_file("touhou_6_apparitions.mid");

  /* just try writing out some stuff */
  wav_export_open_file("test_01.wav");
  wav_export_write_header();

  apu_play_note(0, 4 * 12 + 0);

  for (k = 0; k < 60; k++)
  {
    if (k % 3 == 0)
      audio_update_frame(16);
    else
      audio_update_frame(17);

    wav_export_write_block(&G_audio_frame_buffer[0], G_audio_frame_num_samples);
  }

  wav_export_close_file();

  return 0;
}
