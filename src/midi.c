/******************************************************************************/
/* midi.c (midi file import)                                                  */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "midi.h"

#define MIDI_TRACK_SIZE (64 * 1024)

static FILE* S_midi_import_fp;

static unsigned short S_midi_format;
static unsigned short S_midi_num_tracks;
static unsigned short S_midi_ppqn;

static unsigned char  S_midi_combined_data[MIDI_TRACK_SIZE];
static unsigned int   S_midi_combined_num_bytes;

static unsigned char  S_midi_track_data[MIDI_TRACK_SIZE];
static unsigned int   S_midi_track_num_bytes;

/******************************************************************************/
/* midi_parse_header()                                                        */
/******************************************************************************/
int midi_parse_header()
{
  unsigned char buf[5];
  unsigned int  header_size;

  /* chunk name "MThd" */
  if (fread(buf, sizeof(unsigned char), 4, S_midi_import_fp) < 4)
    return 1;

  if ((buf[0] != 0x4D) || (buf[1] != 0x54) || 
      (buf[2] != 0x68) || (buf[3] != 0x64))
  {
    return 1;
  }

  /* header size */
  if (fread(buf, sizeof(unsigned char), 4, S_midi_import_fp) < 4)
    return 1;

  header_size = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

  if (header_size != 6)
    return 1;

  /* format */
  if (fread(buf, sizeof(unsigned char), 2, S_midi_import_fp) < 2)
    return 1;

  S_midi_format = (buf[0] << 8) | buf[1];

  if (S_midi_format > 1)
    return 1;
  
  /* number of tracks */
  if (fread(buf, sizeof(unsigned char), 2, S_midi_import_fp) < 2)
    return 1;

  S_midi_num_tracks = (buf[0] << 8) | buf[1];

  if (S_midi_num_tracks > 16)
    return 1;

  /* parts per quarter note */
  if (fread(buf, sizeof(unsigned char), 2, S_midi_import_fp) < 2)
    return 1;

  S_midi_ppqn = (buf[0] << 8) | buf[1];

  if (S_midi_ppqn & 0x8000)
    return 1;

#if 0
  /* testing */
  printf("MIDI Header Size: %d\n", header_size);
  printf("MIDI Format: %d\n", S_midi_format);
  printf("MIDI Num Tracks: %d\n", S_midi_num_tracks);
  printf("MIDI PPQN: %d\n", S_midi_ppqn);
#endif

  return 0;
}

/******************************************************************************/
/* midi_parse_track()                                                         */
/******************************************************************************/
int midi_parse_track()
{
  unsigned int k;

  unsigned char buf[5];
  unsigned int  chunk_size;

  unsigned int  delta_time;
  unsigned char status_byte;

  unsigned char meta_code;
  unsigned int  event_size;

  /* chunk name "MTrk" */
  if (fread(buf, sizeof(unsigned char), 4, S_midi_import_fp) < 4)
    return 1;

  if ((buf[0] != 0x4D) || (buf[1] != 0x54) || 
      (buf[2] != 0x72) || (buf[3] != 0x6B))
  {
    return 1;
  }

  /* chunk size */
  if (fread(buf, sizeof(unsigned char), 4, S_midi_import_fp) < 4)
    return 1;

  chunk_size = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

  printf("Chunk Size: %d\n", chunk_size);

  /* reset running status */
  status_byte = 0x00;

  /* read track events */
  while (1)
  {
    /* read delta time */
    for (k = 0; k < 4; k++)
    {
      if (fread(buf, sizeof(unsigned char), 1, S_midi_import_fp) < 1)
        return 1;

      if (k == 0)
        delta_time = buf[0] & 0x7F;
      else
        delta_time = (delta_time << 7) | (buf[0] & 0x7F);

      if (!(buf[0] & 0x80))
        break;
    }

    /* output "delay" sequencer commands if necessary */
    while (delta_time > 0)
    {
      if (delta_time <= 255)
      {
        S_midi_track_data[S_midi_track_num_bytes + 0] = 0x10;
        S_midi_track_data[S_midi_track_num_bytes + 1] = delta_time & 0xFF;
        S_midi_track_num_bytes += 2;
        delta_time = 0;
      }
      else if (delta_time <= 65535)
      {
        S_midi_track_data[S_midi_track_num_bytes + 0] = 0x20;
        S_midi_track_data[S_midi_track_num_bytes + 1] = delta_time & 0xFF;
        S_midi_track_data[S_midi_track_num_bytes + 2] = (delta_time >> 8) & 0xFF;
        S_midi_track_num_bytes += 3;
        delta_time = 0;
      }
      else
      {
        S_midi_track_data[S_midi_track_num_bytes + 0] = 0x20;
        S_midi_track_data[S_midi_track_num_bytes + 1] = 0xFF;
        S_midi_track_data[S_midi_track_num_bytes + 2] = 0xFF;
        S_midi_track_num_bytes += 3;
        delta_time -= 65535;
      }
    }

    /* read status byte or 1st data byte */
    if (fread(buf, sizeof(unsigned char), 1, S_midi_import_fp) < 1)
      return 1;

    /* check for running status */
    /* if a data byte is encountered here, assume that the (implicit) */
    /* status byte is the same as it was for the previous midi event  */
    if (buf[0] & 0x80)
    {
      status_byte = buf[0];

      /* read data byte(s) */
      if (((status_byte & 0xF0) == 0x80) || 
          ((status_byte & 0xF0) == 0x90) || 
          ((status_byte & 0xF0) == 0xA0) || 
          ((status_byte & 0xF0) == 0xB0) || 
          ((status_byte & 0xF0) == 0xE0))
      {
        if (fread(&buf[0], sizeof(unsigned char), 2, S_midi_import_fp) < 2)
          return 1;
      }
      else if ( ((status_byte & 0xF0) == 0xC0) || 
                ((status_byte & 0xF0) == 0xD0))
      {
        if (fread(&buf[0], sizeof(unsigned char), 1, S_midi_import_fp) < 1)
          return 1;

        buf[1] = 0x00;
      }
    }
    else
    {
      /* read remaining data byte(s) */
      if (((status_byte & 0xF0) == 0x80) || 
          ((status_byte & 0xF0) == 0x90) || 
          ((status_byte & 0xF0) == 0xA0) || 
          ((status_byte & 0xF0) == 0xB0) || 
          ((status_byte & 0xF0) == 0xE0))
      {
        if (fread(&buf[1], sizeof(unsigned char), 1, S_midi_import_fp) < 1)
          return 1;
      }
      else if ( ((status_byte & 0xF0) == 0xC0) || 
                ((status_byte & 0xF0) == 0xD0))
      {
        buf[1] = 0x00;
      }
    }

    /* note off */
    if ((status_byte & 0xF0) == 0x80)
    {
      S_midi_track_data[S_midi_track_num_bytes + 0] = 0x50;
      S_midi_track_data[S_midi_track_num_bytes + 1] = buf[0];
      S_midi_track_num_bytes += 2;
    }
    /* note on */
    else if ((status_byte & 0xF0) == 0x90)
    {
      S_midi_track_data[S_midi_track_num_bytes + 0] = 0x40;
      S_midi_track_data[S_midi_track_num_bytes + 1] = buf[0];
      S_midi_track_data[S_midi_track_num_bytes + 2] = buf[1];
      S_midi_track_num_bytes += 3;
    }
    /* aftertouch */
    else if ((status_byte & 0xF0) == 0xA0)
    {
      S_midi_track_data[S_midi_track_num_bytes + 0] = 0x70;
      S_midi_track_data[S_midi_track_num_bytes + 1] = buf[1];
      S_midi_track_num_bytes += 2;
    }
    /* controller */
    else if ((status_byte & 0xF0) == 0xB0)
    {
      printf("Controller %d Change: %d\n", buf[0], buf[1]);

      if (buf[0] == 1)
        S_midi_track_data[S_midi_track_num_bytes + 0] = 0x90;
      else if (buf[0] == 4)
        S_midi_track_data[S_midi_track_num_bytes + 0] = 0xA0;
      else
        S_midi_track_data[S_midi_track_num_bytes + 0] = 0x90;

      S_midi_track_data[S_midi_track_num_bytes + 1] = buf[1];
      S_midi_track_num_bytes += 2;
    }
    /* program change */
    else if ((status_byte & 0xF0) == 0xC0)
    {
      S_midi_track_data[S_midi_track_num_bytes + 0] = 0xD0;
      S_midi_track_data[S_midi_track_num_bytes + 1] = buf[0];
      S_midi_track_num_bytes += 2;
    }
    /* channel pressure */
    else if ((status_byte & 0xF0) == 0xD0)
    {
      S_midi_track_data[S_midi_track_num_bytes + 0] = 0x70;
      S_midi_track_data[S_midi_track_num_bytes + 1] = buf[0];
      S_midi_track_num_bytes += 2;
    }
    /* pitch bend */
    else if ((status_byte & 0xF0) == 0xE0)
    {
      S_midi_track_data[S_midi_track_num_bytes + 0] = 0x60;
      S_midi_track_data[S_midi_track_num_bytes + 1] = buf[1];
      S_midi_track_num_bytes += 2;
    }
    /* meta event */
    else if (status_byte == 0xFF)
    {
      /* read meta code */
      if (fread(&meta_code, sizeof(unsigned char), 1, S_midi_import_fp) < 1)
        return 1;

      /* read event size */
      for (k = 0; k < 4; k++)
      {
        if (fread(buf, sizeof(unsigned char), 1, S_midi_import_fp) < 1)
          return 1;

        if (k == 0)
          event_size = buf[0] & 0x7F;
        else
          event_size = (delta_time << 7) | (buf[0] & 0x7F);

        if (!(buf[0] & 0x80))
          break;
      }

      /* end of track */
      if (meta_code == 0x2F)
      {
        if (event_size != 0)
          return 1;

        printf("Success!\n");
        return 0;
      }
      /* skip other meta events */
      else
      {
        for (k = 0; k < event_size; k++)
        {
          if (fread(buf, sizeof(unsigned char), 1, S_midi_import_fp) < 1)
            return 1;
        }
      }

      /* reset running status */
      status_byte = 0x00;
    }
    /* sysex event */
    else if ((status_byte == 0xF0) || (status_byte == 0xF7))
    {
      /* read event size */
      for (k = 0; k < 4; k++)
      {
        if (fread(buf, sizeof(unsigned char), 1, S_midi_import_fp) < 1)
          return 1;

        if (k == 0)
          event_size = buf[0] & 0x7F;
        else
          event_size = (delta_time << 7) | (buf[0] & 0x7F);

        if (!(buf[0] & 0x80))
          break;
      }

      /* skip sysex event */
      for (k = 0; k < event_size; k++)
      {
        if (fread(buf, sizeof(unsigned char), 1, S_midi_import_fp) < 1)
          return 1;
      }

      /* reset running status */
      status_byte = 0x00;
    }
    else
    {
      printf("Unknown MIDI Message.\n");
      return 1;
    }
  }

  return 0;
}

/******************************************************************************/
/* midi_import_file()                                                         */
/******************************************************************************/
int midi_import_file(char* filename)
{
  unsigned int k;

  /* make sure filename is valid */
  if (filename == NULL)
    return 1;

  /* open file */
  S_midi_import_fp = fopen(filename, "rb");

  if (S_midi_import_fp == NULL)
    return 1;

  /* initialize combined data (all tracks) */
  for (k = 0; k < MIDI_TRACK_SIZE; k++)
    S_midi_combined_data[k] = 0x00;

  S_midi_combined_num_bytes = 0;

  /* start parsing the file */
  if (midi_parse_header())
    goto nope;

  for (k = 0; k < S_midi_num_tracks; k++)
  {
    /* initialize track data */
    for (k = 0; k < MIDI_TRACK_SIZE; k++)
      S_midi_track_data[k] = 0x00;

    S_midi_track_num_bytes = 0;

    /* parse track */
    if (midi_parse_track())
      goto nope;

    /* consolidate tracks */
    /* ... */

    /* testing */
    printf("Track Size: %d\n", S_midi_track_num_bytes);
  }

  /* close file */
  fclose(S_midi_import_fp);

  goto ok;

nope:
  printf("Error parsing MIDI file...\n");
  fclose(S_midi_import_fp);
  return 1;

ok:
  return 0;
}

