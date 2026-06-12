/******************************************************************************/
/* audio.h (sdl audio code)                                                   */
/******************************************************************************/

#ifndef AUDIO_H
#define AUDIO_H

extern short          G_audio_frame_buffer[];
extern unsigned long  G_audio_frame_num_samples;

/* function declarations */
int audio_init();
int audio_deinit();

int audio_update_frame(unsigned short milliseconds);

#endif

