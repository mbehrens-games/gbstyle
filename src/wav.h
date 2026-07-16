/******************************************************************************/
/* wav.h (wave file import and export)                                        */
/******************************************************************************/

#ifndef WAV_H
#define WAV_H

/* function declarations */
int wav_export_open_file(char* filename);
int wav_export_close_file();
int wav_export_write_header();
int wav_export_write_block(short* sample_buf, unsigned int num_samples);

#endif

