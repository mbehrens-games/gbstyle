/******************************************************************************/
/* fileio.c (wave file import and export)                                     */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "fileio.h"

#define WAV_AUDIO_FORMAT     1
#define WAV_NUM_CHANNELS     1
#define WAV_BIT_RESOLUTION  16
#define WAV_SAMPLE_SIZE     (WAV_NUM_CHANNELS * (WAV_BIT_RESOLUTION / 8))

static FILE* S_export_fp;

/******************************************************************************/
/* export_wav_open_file()                                                     */
/******************************************************************************/
int export_wav_open_file(char* filename)
{
  /* make sure filename is valid */
  if (filename == NULL)
    return 1;

  /* open file */
  S_export_fp = fopen(filename, "w+b");

  if (S_export_fp == NULL)
    return 1;

  return 0;
}

/******************************************************************************/
/* export_wav_close_file()                                                    */
/******************************************************************************/
int export_wav_close_file()
{
  /* close file */
  fclose(S_export_fp);

  return 0;
}

/******************************************************************************/
/* export_wav_write_header()                                                  */
/******************************************************************************/
int export_wav_write_header()
{
  char id_field[4];

  unsigned short audio_format;
  unsigned short num_channels;
  unsigned int  sampling_rate;
  unsigned int  byte_rate;
  unsigned short sample_size;
  unsigned short bit_resolution;

  unsigned int  chunk_size;
  unsigned int  header_subchunk_size;
  unsigned int  data_subchunk_size;

  /* set and compute values */
  audio_format = WAV_AUDIO_FORMAT;
  num_channels = WAV_NUM_CHANNELS;
  sampling_rate = 24000; /* this is set in apu.h */
  bit_resolution = WAV_BIT_RESOLUTION;
  sample_size = WAV_SAMPLE_SIZE;
  byte_rate = sampling_rate * WAV_SAMPLE_SIZE;

  header_subchunk_size = 16;
  data_subchunk_size = 0;
  chunk_size = 4 + (8 + header_subchunk_size) + (8 + data_subchunk_size);

  /* write 'RIFF' */
  id_field[0] = 'R';
  id_field[1] = 'I';
  id_field[2] = 'F';
  id_field[3] = 'F';

  if (fwrite(id_field, 1, 4, S_export_fp) < 4)
    return 1;

  /* write chunk size */
  if (fwrite(&chunk_size, 4, 1, S_export_fp) < 1)
    return 1;

  /* write 'WAVE' */
  id_field[0] = 'W';
  id_field[1] = 'A';
  id_field[2] = 'V';
  id_field[3] = 'E';

  if (fwrite(id_field, 1, 4, S_export_fp) < 4)
    return 1;

  /* write 'fmt ' */
  id_field[0] = 'f';
  id_field[1] = 'm';
  id_field[2] = 't';
  id_field[3] = ' ';

  if (fwrite(id_field, 1, 4, S_export_fp) < 4)
    return 1;

  /* write header subchunk size */
  if (fwrite(&header_subchunk_size, 4, 1, S_export_fp) < 1)
    return 1;

  /* write header subchunk */
  if (fwrite(&audio_format, 2, 1, S_export_fp) < 1)
    return 1;

  if (fwrite(&num_channels, 2, 1, S_export_fp) < 1)
    return 1;

  if (fwrite(&sampling_rate, 4, 1, S_export_fp) < 1)
    return 1;

  if (fwrite(&byte_rate, 4, 1, S_export_fp) < 1)
    return 1;

  if (fwrite(&sample_size, 2, 1, S_export_fp) < 1)
    return 1;

  if (fwrite(&bit_resolution, 2, 1, S_export_fp) < 1)
    return 1;

  /* write 'data' */
  id_field[0] = 'd';
  id_field[1] = 'a';
  id_field[2] = 't';
  id_field[3] = 'a';

  if (fwrite(id_field, 1, 4, S_export_fp) < 4)
    return 1;

  /* write data subchunk size */
  if (fwrite(&data_subchunk_size, 4, 1, S_export_fp) < 1)
    return 1;

  return 0;
}

/******************************************************************************/
/* export_wav_write_block()                                                   */
/******************************************************************************/
int export_wav_write_block(short* sample_buf, unsigned int num_samples)
{
  unsigned int chunk_size;
  unsigned int data_subchunk_size;

  /* check input parameters */
  if (sample_buf == NULL)
    return 1;

  if (num_samples == 0)
    return 1;

  /* update chunk size */
  fseek(S_export_fp, 4, SEEK_SET);

  if (fread(&chunk_size, 4, 1, S_export_fp) < 1)
    return 1;

  chunk_size += num_samples * WAV_SAMPLE_SIZE;

  fseek(S_export_fp, 4, SEEK_SET);

  if (fwrite(&chunk_size, 4, 1, S_export_fp) < 1)
    return 1;

  /* update data subchunk size */
  fseek(S_export_fp, 40, SEEK_SET);

  if (fread(&data_subchunk_size, 4, 1, S_export_fp) < 1)
    return 1;

  data_subchunk_size += num_samples * WAV_SAMPLE_SIZE;

  fseek(S_export_fp, 40, SEEK_SET);

  if (fwrite(&data_subchunk_size, 4, 1, S_export_fp) < 1)
    return 1;

  /* write samples */
  fseek(S_export_fp, 0, SEEK_END);

  if (fwrite(sample_buf, 2, num_samples, S_export_fp) < num_samples)
    return 1;

  return 0;
}

