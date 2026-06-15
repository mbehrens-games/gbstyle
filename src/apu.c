/******************************************************************************/
/* apu.c (faux sound chip)                                                    */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "apu.h"

#define PI 3.14159265358979323846f

/* clock, dividers, and timer */

/* the clock rate is a multiple of 1000 so that   */
/* there are an integer number of samples per ms. */
#define APU_CLOCK_RATE        96000
#define APU_CLOCKS_PER_SAMPLE (APU_CLOCK_RATE / APU_OUT_SAMPLING_RATE)

#define APU_ENV_CLOCK_DIVIDER  6  /* envelope clock is 16000  */
#define APU_LFO_CLOCK_DIVIDER 32  /* lfo clock is 3000        */
#define APU_SEQ_CLOCK_DIVIDER 16  /* sequencer clock is 6000  */

#define APU_TIMER_CLOCK_DIVIDER 48  /* lcm of the other dividers */

static unsigned short S_apu_timer;

/* tuning */
#define APU_NOTE_LOWEST   ( 0 * 12 +  9) /* A-0 */
#define APU_NOTE_HIGHEST  ( 8 * 12 +  0) /* C-8 */
#define APU_NOTE_MIDDLE_C ( 4 * 12 +  0) /* C-4 */

/* linear to db and db to linear tables */
#define APU_DB_STEP 0.01171875f

#define APU_DB_VAL_NUM_BITS  12
#define APU_DB_VAL_SIZE      (1 << APU_DB_VAL_NUM_BITS)
#define APU_DB_VAL_MASK      (APU_DB_VAL_SIZE - 1)

#define APU_MAX_VOL_DB  0
#define APU_MAX_ATT_DB  (APU_DB_VAL_SIZE - 1)

#define APU_MAX_VOL_LIN 32767
#define APU_MAX_ATT_LIN 0

static short S_apu_db_to_linear_table[APU_DB_VAL_SIZE];

static unsigned short S_apu_env_level_to_db_table[16];
static unsigned short S_apu_wave_wt_level_to_db_table[16];
static unsigned short S_apu_pcm_level_to_db_table[128];

/* pitch table */
#define APU_WAVE_SEMITONE_STEPS   64
#define APU_WAVE_NUM_OCTAVES       9 /* octaves 0 to 8 */
#define APU_WAVE_PITCH_TABLE_SIZE (12 * APU_WAVE_SEMITONE_STEPS)

#define APU_WAVE_MAX_PITCH_INDEX  ((APU_WAVE_NUM_OCTAVES * APU_WAVE_PITCH_TABLE_SIZE) - 1)

static unsigned int S_apu_wave_pitch_table[APU_WAVE_PITCH_TABLE_SIZE];

/* envelope time table */
#define APU_ENV_TIME_TABLE_SIZE (12 * 16) /* 16 steps per semitone */
#define APU_ENV_NUM_OCTAVES     11

#define APU_ENV_MAX_TIME_INDEX  ((APU_ENV_NUM_OCTAVES * APU_ENV_TIME_TABLE_SIZE) - 1)

static unsigned int S_apu_env_time_table[APU_ENV_TIME_TABLE_SIZE];

/* envelope keyscaling */
#define APU_ENV_KEYSCALING_DIVISOR  256
#define APU_ENV_TIME_KS_BREAKPOINT  (APU_NOTE_MIDDLE_C - 4 * 12 + 0)  /* C-0 */
#define APU_ENV_LEVEL_KS_BREAKPOINT (APU_NOTE_MIDDLE_C - 2 * 12 + 9)  /* A-2 */

/* phase registers */
#define APU_PHASE_REG_NUM_BITS  21
#define APU_PHASE_REG_SIZE      (1 << APU_PHASE_REG_NUM_BITS)
#define APU_PHASE_REG_MASK      (APU_PHASE_REG_SIZE - 1)

/* wavetables */
#define APU_WT_STEPS      32
#define APU_WT_WAVE_BYTES ((5 * APU_WT_STEPS) / 8)
#define APU_WT_ENV_BYTES  ((4 * APU_WT_STEPS) / 8)

#define APU_WT_NUM_WAVES      64
#define APU_WT_WAVE_BANK_SIZE (APU_WT_NUM_WAVES * APU_WT_WAVE_BYTES)

#define APU_WT_NUM_ENVS       (6 * 16)
#define APU_WT_ENV_BANK_SIZE  (APU_WT_NUM_ENVS * APU_WT_ENV_BYTES)

static unsigned char S_apu_wave_wavetable_bank[APU_WT_WAVE_BANK_SIZE];
static unsigned char S_apu_env_wavetable_bank[APU_WT_ENV_BANK_SIZE];

/* voices */
#define APU_NUM_WAVE_VOICES 12
#define APU_NUM_PCM_VOICES   4

static unsigned char  S_apu_wave_note[APU_NUM_WAVE_VOICES];
static unsigned short S_apu_wave_phase[APU_NUM_WAVE_VOICES];
static unsigned short S_apu_wave_wt_pos[APU_NUM_WAVE_VOICES];
static unsigned short S_apu_wave_level_db[APU_NUM_WAVE_VOICES];
static unsigned char  S_apu_wave_level_sign[APU_NUM_WAVE_VOICES];

/* highpass filters */

/* the coefficient is: 1 / (2 * PI * DELTA_T * CUTOFF_FC + 1)   */
/* where DELTA_T is 1 / CLOCK_RATE and CUTOFF_FC is 27.5 (A-0)  */
#define APU_HP_COEF 0.998203362923f

/* the multiplier is the coefficient expressed as a fraction over 32768 */
#define APU_HP_MULT 32710

static short S_apu_hp_left_in[2];
static short S_apu_hp_left_out[2];

static short S_apu_hp_right_in[2];
static short S_apu_hp_right_out[2];

/* lowpass filters */
#define APU_LP_M 64

#define APU_LP_KERNEL_SIZE (APU_LP_M + 1)

static short S_apu_lp_kernel[APU_LP_KERNEL_SIZE];

static short S_apu_lp_left_in[APU_LP_KERNEL_SIZE];
static short S_apu_lp_right_in[APU_LP_KERNEL_SIZE];

static short S_apu_lp_buf_pos;

/* output levels */
short G_apu_out_left;
short G_apu_out_right;

/******************************************************************************/
/* apu_reset()                                                                */
/******************************************************************************/
int apu_reset()
{
  int m;

  S_apu_timer = 0;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    S_apu_wave_note[m] = APU_NOTE_MIDDLE_C;
    S_apu_wave_phase[m] = 0;
    S_apu_wave_wt_pos[m] = 0;
    S_apu_wave_level_db[m] = APU_MAX_ATT_DB;
    S_apu_wave_level_sign[m] = 0;
  }

  for (m = 0; m < 2; m++)
  {
    S_apu_hp_left_in[m] = 0;
    S_apu_hp_left_out[m] = 0;

    S_apu_hp_right_in[m] = 0;
    S_apu_hp_right_out[m] = 0;
  }

  for (m = 0; m < APU_LP_KERNEL_SIZE; m++)
  {
    S_apu_lp_left_in[m] = 0;
    S_apu_lp_right_in[m] = 0;
  }

  S_apu_lp_buf_pos = 0;

  G_apu_out_left = 0;
  G_apu_out_right = 0;

  return 0;
}

/******************************************************************************/
/* apu_update_waves()                                                         */
/******************************************************************************/
int apu_update_waves()
{
  int m;

  int   pitch_index;
  short octave;

  unsigned int increment;

  unsigned short bank_index;
  unsigned char  code;
  unsigned char  bit;
  unsigned char  mask;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    /* testing: just update 1st voice for now */
    if (m > 0)
      continue;

    /* lookup phase increment */
    pitch_index = (S_apu_wave_note[m] % 12) * APU_WAVE_SEMITONE_STEPS;

    if (pitch_index > APU_WAVE_MAX_PITCH_INDEX)
      pitch_index = APU_WAVE_MAX_PITCH_INDEX;
    else if (pitch_index < 0)
      pitch_index = 0;

    increment = S_apu_wave_pitch_table[pitch_index];

    octave = (S_apu_wave_note[m] / 12);

    if (octave < (APU_WAVE_NUM_OCTAVES - 1))
      increment = increment >> (APU_WAVE_NUM_OCTAVES - 1 - octave);

    /* add phase increment to register, check for overflow */
    S_apu_wave_phase[m] = (S_apu_wave_phase[m] + increment) % 65536;

    /* update wavetable position and level if necessary */
    if (S_apu_wave_phase[m] < increment)
    {
      S_apu_wave_wt_pos[m] = (S_apu_wave_wt_pos[m] + 1) % APU_WT_STEPS;

      /* get level at this wavetable position */
      bank_index = 0 + ((5 * S_apu_wave_wt_pos[m]) / 8);

      if (((5 * S_apu_wave_wt_pos[m]) % 8) == 0)
        mask = 0x01;
      else
        mask = 0x01 << ((5 * S_apu_wave_wt_pos[m]) % 8);

      code = 0;

      for (bit = 0; bit < 5; bit++)
      {
        if (S_apu_wave_wavetable_bank[bank_index] & mask)
        {
          if (bit == 0)
            code |= 0x01;
          else
            code |= 0x01 << bit;
        }
        else
        {
          if (bit == 0)
            code &= ~0x01;
          else
            code &= ~(0x01 << bit);
        }

        if (mask == 0x80)
        {
          bank_index += 1;
          mask = 0x01;
        }
        else
          mask = mask << 1;
      }

      code = code & 0x1F;

      /* determine wave level */
      if (code >= 16)
      {
        S_apu_wave_level_db[m] = S_apu_wave_wt_level_to_db_table[code - 16];
        S_apu_wave_level_sign[m] = 0;
      }
      else
      {
        S_apu_wave_level_db[m] = S_apu_wave_wt_level_to_db_table[15 - code];
        S_apu_wave_level_sign[m] = 1;
      }

      /* testing: set level to 1/2 for now */
      S_apu_wave_level_db[m] += 256;

      if (S_apu_wave_level_db[m] > APU_MAX_ATT_DB)
        S_apu_wave_level_db[m] = APU_MAX_ATT_DB;
    }
  }

  return 0;
}

/******************************************************************************/
/* apu_update_out()                                                           */
/******************************************************************************/
int apu_update_out()
{
  int m;

  int samp_left;
  int samp_right;

  int adj_pos;

  for (m = 0; m < APU_CLOCKS_PER_SAMPLE; m++)
  {
    /* update sequencer */
    if ((S_apu_timer % APU_SEQ_CLOCK_DIVIDER) == 0)
    {

    }

    /* update lfos */
    if ((S_apu_timer % APU_LFO_CLOCK_DIVIDER) == 0)
    {

    }

    /* update envelopes */
    if ((S_apu_timer % APU_ENV_CLOCK_DIVIDER) == 0)
    {

    }

    /* update waves */
    apu_update_waves();

    /* compute current samples (left & right) */
    samp_left = 0;
    samp_right = 0;

    for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
    {
      if (S_apu_wave_level_sign[m] == 0)
      {
        samp_left  += S_apu_db_to_linear_table[S_apu_wave_level_db[m]];
        samp_right += S_apu_db_to_linear_table[S_apu_wave_level_db[m]];
      }
      else
      {
        samp_left  -= S_apu_db_to_linear_table[S_apu_wave_level_db[m]];
        samp_right -= S_apu_db_to_linear_table[S_apu_wave_level_db[m]];
      }
    }

    if (samp_left > 32767)
      samp_left = 32767;
    else if (samp_left < -32768)
      samp_left = -32768;

    if (samp_right > 32767)
      samp_right = 32767;
    else if (samp_right < -32768)
      samp_right = -32768;

    /* apply highpass filters (left & right) */
    S_apu_hp_left_in[1] = S_apu_hp_left_in[0];
    S_apu_hp_left_in[0] = samp_left;

    S_apu_hp_left_out[1] = S_apu_hp_left_out[0];
    S_apu_hp_left_out[0] = (APU_HP_MULT * ( S_apu_hp_left_out[1] + 
                                            S_apu_hp_left_in[0] - 
                                            S_apu_hp_left_in[1])) / 32768;

    S_apu_hp_right_in[1] = S_apu_hp_right_in[0];
    S_apu_hp_right_in[0] = samp_left;

    S_apu_hp_right_out[1] = S_apu_hp_right_out[0];
    S_apu_hp_right_out[0] = (APU_HP_MULT * (S_apu_hp_right_out[1] + 
                                            S_apu_hp_right_in[0] - 
                                            S_apu_hp_right_in[1])) / 32768;

    /* update lowpass filter input buffers (left & right) */
    S_apu_lp_left_in[S_apu_lp_buf_pos] = S_apu_hp_left_out[0];
    S_apu_lp_right_in[S_apu_lp_buf_pos] = S_apu_hp_right_out[0];

    S_apu_lp_buf_pos = (S_apu_lp_buf_pos + 1) % APU_LP_KERNEL_SIZE;

    /* update timer */
    S_apu_timer += 1;

    if ((S_apu_timer % APU_TIMER_CLOCK_DIVIDER) == 0)
      S_apu_timer = 0;
  }

  /* apply lowpass filters (left & right) */
  samp_left = 0;
  samp_right = 0;

  for (m = 0; m < APU_LP_KERNEL_SIZE; m++)
  {
    adj_pos = (S_apu_lp_buf_pos + m) % APU_LP_KERNEL_SIZE;

    samp_left += (S_apu_lp_kernel[m] * S_apu_lp_left_in[adj_pos]) / 32768;
    samp_right += (S_apu_lp_kernel[m] * S_apu_lp_right_in[adj_pos]) / 32768;
  }

  if (samp_left > 32767)
    samp_left = 32767;
  else if (samp_left < -32768)
    samp_left = -32768;

  if (samp_right > 32767)
    samp_right = 32767;
  else if (samp_right < -32768)
    samp_right = -32768;

  G_apu_out_left = samp_left;
  G_apu_out_right = samp_right;

  return 0;
}

/******************************************************************************/
/* apu_compute_tables()                                                       */
/******************************************************************************/
int apu_compute_tables()
{
  int k;
  int m;
  int n;

  double val;
  double normal;

  unsigned char wavetable[APU_WT_STEPS];
  unsigned char level;

  int index;

  unsigned char bit;
  unsigned char mask;

  /* 12 bit envelope & waveform values for each bit, in db:                 */
  /* 3(8), 3(4), 3(2), 3(1), 3/2, 3/4, 3/8, 3/16, 3/32, 3/64, 3/128, 3/256  */

  /* db to linear table */
  S_apu_db_to_linear_table[0] = APU_MAX_VOL_LIN;
  S_apu_db_to_linear_table[APU_DB_VAL_SIZE - 1] = APU_MAX_ATT_LIN;

  for (m = 1; m < APU_DB_VAL_SIZE - 1; m++)
  {
    val = exp(-log(10) * (APU_DB_STEP / 10) * m);

    S_apu_db_to_linear_table[m] = (short) ((APU_MAX_VOL_LIN * val) + 0.5f);
  }

  /* envelope level to db table (4 bits, 0 to 1) */
  S_apu_env_level_to_db_table[0]  = APU_MAX_ATT_DB;
  S_apu_env_level_to_db_table[15] = APU_MAX_VOL_DB;

  for (m = 1; m < 15; m++)
  {
    val = m / 15.0f;

    S_apu_env_level_to_db_table[m] = 
      (short) ((10 * (-log(val) / log(10)) / APU_DB_STEP) + 0.5f);
  }

  /* wave level to db table (5 bits, -1 to 1) */
  S_apu_wave_wt_level_to_db_table[15] = APU_MAX_VOL_DB;

  for (m = 0; m < 15; m++)
  {
    val = (2 * m + 1) / 31.0f;

    S_apu_wave_wt_level_to_db_table[m] = 
      (short) ((10 * (-log(val) / log(10)) / APU_DB_STEP) + 0.5f);
  }

  /* pcm level to db table (8 bits, -1 to 1) */
  S_apu_pcm_level_to_db_table[127] = APU_MAX_VOL_DB;

  for (m = 0; m < 127; m++)
  {
    val = (2 * m + 1) / 255.0f;

    S_apu_pcm_level_to_db_table[m] = 
      (short) ((10 * (-log(val) / log(10)) / APU_DB_STEP) + 0.5f);
  }

  /* wave pitch table */
  /* this contains the phase increments for the highest octave, C8 - B8 */
  for (m = 0; m < APU_WAVE_PITCH_TABLE_SIZE; m++)
  {
    val = 440.0f * exp(-log(2) * (9.0f / 12.0f));
    val *= 16;
    val *= exp(log(2) * ((double) m / APU_WAVE_PITCH_TABLE_SIZE));

    val *= (double) APU_PHASE_REG_SIZE;
    val /= (double) APU_CLOCK_RATE;

    S_apu_wave_pitch_table[m] = (unsigned int) (val + 0.5f);
  }

  /* envelope time table */
  /* for the attack stage, the fastest rate should have a rise time of 6 ms   */
  /* for the decay stage, the fastest rate should have a fall time of 24 ms   */
  /* (which is 2 octaves down). these occur over 32 envelope wavetable steps. */
  for (m = 0; m < APU_ENV_TIME_TABLE_SIZE; m++)
  {
    val = 32 / 0.006f;
    val *= 1 / 2.0f;
    val *= exp(log(2) * ((double) m / APU_ENV_TIME_TABLE_SIZE));

    val *= (double) APU_PHASE_REG_SIZE;
    val /= (double) (APU_CLOCK_RATE / APU_ENV_CLOCK_DIVIDER);

    S_apu_env_time_table[m] = (unsigned int) (val + 0.5f);
  }

  /* wave wavetables (default square, saw) */
  for (m = 0; m < 2; m++)
  {
    index = m * APU_WT_STEPS;
    mask = 0x01;

    for (n = 0; n < APU_WT_STEPS; n++)
    {
      if (m == 0)
      {
        if (n < (APU_WT_STEPS / 2))
          wavetable[n] = 31;
        else
          wavetable[n] = 0;
      }
      else if (m == 1)
        wavetable[n] = 31 - n;
      else
        wavetable[n] = 8;
    }

    for (n = 0; n < APU_WT_STEPS; n++)
    {
      level = wavetable[n];

      for (bit = 0; bit < 5; bit++)
      {
        if (level & 0x01)
          S_apu_wave_wavetable_bank[index] |= mask;
        else
          S_apu_wave_wavetable_bank[index] &= ~mask;

        level = level >> 1;

        if (mask == 0x80)
        {
          index += 1;
          mask = 0x01;
        }
        else
          mask = mask << 1;
      }
    }
  }

  /* envelope wavetables (linear) */
  for (k = 0; k < 3; k++)
  {
    for (m = 0; m < 16; m++)
    {
      index = (k * 16 + m) * APU_WT_STEPS;
      mask = 0x01;

      /* compute attack wavetable */
      for (n = 0; n < APU_WT_STEPS; n++)
      {
        val = ((15 - m) / 31.0f) * n + m;

        wavetable[n] = (unsigned char) (val + 0.5f);
      }

      /* write out wavetable after transformation */
      for (n = 0; n < APU_WT_STEPS; n++)
      {
        if (k == 0)
          level = wavetable[n];
        else if (k == 1)
          level = wavetable[APU_WT_STEPS - 1 - n];
        else if (k == 2)
          level = wavetable[APU_WT_STEPS - 1 - n] - m;
        else
          level = 0;

        for (bit = 0; bit < 4; bit++)
        {
          if (level & 0x01)
            S_apu_env_wavetable_bank[index] |= mask;
          else
            S_apu_env_wavetable_bank[index] &= ~mask;

          level = level >> 1;

          if (mask == 0x80)
          {
            index += 1;
            mask = 0x01;
          }
          else
            mask = mask << 1;
        }
      }
    }
  }

  /* lowpass fir filter kernel */

  /* The equations are from Steven W. Smith's The Scientist and         */
  /* Engineer's Guide to Digital Signal Processing, page 290 (Ch. 16)   */

  /* the cutoff frequency is defined as a fraction of the clock rate    */
  /* with a clock rate of 96 khz and a sampling rate of 24 khz, the     */
  /* output nyquist frequency is 12 khz, or 0.125. since the transition */
  /* band is approximately 4 / M, with M = 64, that gives us 0.0625.    */
  /* thus the cutoff is 0.09375 (center of transition band)             */

  /* compute normalization factor (sum of all kernel values) */
  normal = 0.0f;

  for (m = 0; m < APU_LP_KERNEL_SIZE; m++)
  {
    if (m == (APU_LP_M / 2))
      val = 2 * PI * 0.09375f;
    else
    {
      /* windowed sinc with hamming window */
      val = sin(2 * PI * 0.09375f * (m - APU_LP_M / 2)) / (m - APU_LP_M / 2);
      val *= 0.54f - 0.46f * cos(2 * PI * (m / (double) APU_LP_M));
    }

    normal += val;
  }

  /* compute final kernel */
  for (m = 0; m < APU_LP_KERNEL_SIZE; m++)
  {
    if (m == (APU_LP_M / 2))
      val = 2 * PI * 0.09375f;
    else
    {
      /* windowed sinc with hamming window */
      val = sin(2 * PI * 0.09375f * (m - APU_LP_M / 2)) / (m - APU_LP_M / 2);
      val *= 0.54f - 0.46f * cos(2 * PI * (m / (double) APU_LP_M));
    }

    val /= normal;
    val *= 32767;

    S_apu_lp_kernel[m] = (unsigned short) (val + 0.5f);
  }

  /* testing */
#if 0
  for (m = 0; m < 16; m++)
    printf("Envelope Level to DB Index %d: %d\n", m, S_apu_env_level_to_db_table[m]);

  printf("\n");

  for (m = 0; m < 16; m++)
    printf("Wave Level to DB Level Index %d: %d\n", m, S_apu_wave_wt_level_to_db_table[m]);

  printf("\n");

  for (m = 0; m < 128; m++)
    printf("PCM Level to DB Index %d: %d\n", m, S_apu_pcm_level_to_db_table[m]);
#endif

#if 0
  for (m = 0; m < APU_WAVE_PITCH_TABLE_SIZE; m += 4)
    printf("Wave Pitch Table Index %d: %d\n", m, S_apu_wave_pitch_table[m]);
#endif

#if 0
  for (m = 0; m < APU_ENV_TIME_TABLE_SIZE; m += 4)
    printf("Env Time Table Index %d: %d\n", m, S_apu_env_time_table[m]);
#endif

#if 0
  printf("Wave Wavetable:\n");

  index = 1 * APU_WT_STEPS;
  mask = 0x01;

  for (n = 0; n < APU_WT_STEPS; n++)
  {
    level = 0;

    for (bit = 0; bit < 5; bit++)
    {
      if (S_apu_wave_wavetable_bank[index] & mask)
      {
        if (bit == 0)
          level |= 0x01;
        else
          level |= 0x01 << bit;
      }
      else
      {
        if (bit == 0)
          level &= ~0x01;
        else
          level &= ~(0x01 << bit);
      }

      if (mask == 0x80)
      {
        index += 1;
        mask = 0x01;
      }
      else
        mask = mask << 1;
    }

    printf("%d: %d\n", n, level);
  }
#endif

#if 0
  for (k = 0; k < 3; k++)
  {
    printf("Env Stage %d:\n\n", k);

    for (m = 0; m < 16; m++)
    {
      printf("Variant %d:\n", m);

      index = (k * 16 + m) * APU_WT_STEPS;
      mask = 0x01;

      for (n = 0; n < APU_WT_STEPS; n++)
      {
        level = 0;

        for (bit = 0; bit < 4; bit++)
        {
          if (S_apu_env_wavetable_bank[index] & mask)
          {
            if (bit == 0)
              level |= 0x01;
            else
              level |= 0x01 << bit;
          }
          else
          {
            if (bit == 0)
              level &= ~0x01;
            else
              level &= ~(0x01 << bit);
          }

          if (mask == 0x80)
          {
            index += 1;
            mask = 0x01;
          }
          else
            mask = mask << 1;
        }

        printf("%d ", level);
      }

      printf("\n\n");
    }
  }
#endif

  return 0;
}

