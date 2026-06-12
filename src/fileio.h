/******************************************************************************/
/* fileio.h (wave file import and export)                                     */
/******************************************************************************/

#ifndef FILEIO_H
#define FILEIO_H

/* function declarations */
int export_wav_open_file(char* filename);
int export_wav_close_file();
int export_wav_write_header();
int export_wav_write_block(short* sample_buf, unsigned long num_samples);

#endif

