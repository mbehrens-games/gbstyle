/******************************************************************************/
/* apu.c (faux sound chip)                                                    */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "apu.h"

/* note: the tables are generated in gnu octave */
/*       see the octave directory for .m files  */

/**********/
/* CLOCKS */
/**********/

/* the clock rate is a multiple of 1000 so that   */
/* there are an integer number of samples per ms. */
#define APU_CLOCK_RATE        96000
#define APU_CLOCKS_PER_SAMPLE (APU_CLOCK_RATE / APU_OUT_SAMPLING_RATE)

#define APU_SEQ_CLOCK_DIVIDER    24  /* sequencer clock is 4000  */
#define APU_LFO_CLOCK_DIVIDER   256  /* lfo clock is 375         */
#define APU_ENV_CLOCK_DIVIDER    64  /* envelope clock is 1500   */

#define APU_TIMER_CLOCK_DIVIDER 768  /* lcm of the other dividers */

static unsigned short S_apu_timer;

/* tuning */
#define APU_NOTE_LOWEST   ( 0 * 12 +  9) /* A-0 */
#define APU_NOTE_HIGHEST  ( 8 * 12 +  0) /* C-8 */
#define APU_NOTE_MIDDLE_C ( 4 * 12 +  0) /* C-4 */

/********************/
/* VOLUME & PANNING */
/********************/

/* volume db to linear table */
#define APU_VOL_LIN_NUM_BLOCKS 16  /* blocks 0 to 15 */
#define APU_VOL_LIN_TABLE_SIZE 256

#define APU_VOL_LIN_MAX_INDEX  4095 /* num_blocks * table_size - 1 */

static unsigned short S_apu_vol_lin_table[APU_VOL_LIN_TABLE_SIZE] = 
  { 32768, 32679, 32591, 32503, 32415, 32327, 32240, 32153,
    32066, 31979, 31893, 31806, 31720, 31635, 31549, 31464,
    31379, 31294, 31209, 31125, 31041, 30957, 30873, 30790,
    30706, 30623, 30541, 30458, 30376, 30293, 30212, 30130,
    30048, 29967, 29886, 29805, 29725, 29644, 29564, 29484,
    29405, 29325, 29246, 29167, 29088, 29009, 28931, 28852,
    28774, 28697, 28619, 28542, 28464, 28388, 28311, 28234,
    28158, 28082, 28006, 27930, 27855, 27779, 27704, 27629,
    27554, 27480, 27406, 27332, 27258, 27184, 27110, 27037,
    26964, 26891, 26818, 26746, 26674, 26601, 26530, 26458,
    26386, 26315, 26244, 26173, 26102, 26031, 25961, 25891,
    25821, 25751, 25681, 25612, 25543, 25474, 25405, 25336,
    25268, 25199, 25131, 25063, 24995, 24928, 24860, 24793,
    24726, 24659, 24593, 24526, 24460, 24394, 24328, 24262,
    24196, 24131, 24066, 24001, 23936, 23871, 23806, 23742,
    23678, 23614, 23550, 23486, 23423, 23359, 23296, 23233,
    23170, 23108, 23045, 22983, 22921, 22859, 22797, 22735,
    22674, 22613, 22552, 22491, 22430, 22369, 22309, 22248,
    22188, 22128, 22068, 22009, 21949, 21890, 21831, 21772,
    21713, 21654, 21595, 21537, 21479, 21421, 21363, 21305,
    21247, 21190, 21133, 21076, 21019, 20962, 20905, 20849,
    20792, 20736, 20680, 20624, 20568, 20513, 20457, 20402,
    20347, 20292, 20237, 20182, 20127, 20073, 20019, 19965,
    19911, 19857, 19803, 19750, 19696, 19643, 19590, 19537,
    19484, 19431, 19379, 19326, 19274, 19222, 19170, 19118,
    19066, 19015, 18963, 18912, 18861, 18810, 18759, 18708,
    18658, 18607, 18557, 18507, 18457, 18407, 18357, 18308,
    18258, 18209, 18160, 18110, 18061, 18013, 17964, 17915,
    17867, 17819, 17770, 17722, 17674, 17627, 17579, 17531,
    17484, 17437, 17390, 17343, 17296, 17249, 17202, 17156,
    17109, 17063, 17017, 16971, 16925, 16879, 16834, 16788,
    16743, 16697, 16652, 16607, 16562, 16518, 16473, 16428
  };

/*******/
/* DCO */
/*******/
#define APU_DCO_PHASE_NUM_BLOCKS 9
#define APU_DCO_PHASE_TABLE_SIZE (12 * 32)

#define APU_DCO_PHASE_MAX_INDEX  3455 /* num_blocks * table_size - 1 */

/* dco phase tables */
static unsigned short S_apu_dco_phase_shifts_table[APU_DCO_PHASE_NUM_BLOCKS] = 
  { 6, 5, 4, 3, 2, 1, 0, 0, 0 };

static unsigned short S_apu_dco_phase_steps_table[APU_DCO_PHASE_NUM_BLOCKS] = 
  { 1, 1, 1, 1, 1, 1, 1, 2, 4 };

static unsigned short S_apu_dco_phase_incs_table[APU_DCO_PHASE_TABLE_SIZE] = 
  { 22861, 22902, 22944, 22985, 23027, 23068, 23110, 23152,
    23194, 23236, 23278, 23320, 23362, 23404, 23446, 23489,
    23531, 23574, 23616, 23659, 23702, 23744, 23787, 23830,
    23873, 23916, 23960, 24003, 24046, 24090, 24133, 24177,
    24221, 24264, 24308, 24352, 24396, 24440, 24484, 24529,
    24573, 24617, 24662, 24706, 24751, 24796, 24840, 24885,
    24930, 24975, 25020, 25066, 25111, 25156, 25202, 25247,
    25293, 25339, 25384, 25430, 25476, 25522, 25568, 25615,
    25661, 25707, 25754, 25800, 25847, 25893, 25940, 25987,
    26034, 26081, 26128, 26175, 26223, 26270, 26318, 26365,
    26413, 26460, 26508, 26556, 26604, 26652, 26700, 26749,
    26797, 26845, 26894, 26942, 26991, 27040, 27089, 27138,
    27187, 27236, 27285, 27334, 27384, 27433, 27483, 27532,
    27582, 27632, 27682, 27732, 27782, 27832, 27882, 27933,
    27983, 28034, 28085, 28135, 28186, 28237, 28288, 28339,
    28390, 28442, 28493, 28545, 28596, 28648, 28699, 28751,
    28803, 28855, 28907, 28960, 29012, 29064, 29117, 29170,
    29222, 29275, 29328, 29381, 29434, 29487, 29540, 29594,
    29647, 29701, 29755, 29808, 29862, 29916, 29970, 30024,
    30079, 30133, 30187, 30242, 30296, 30351, 30406, 30461,
    30516, 30571, 30626, 30682, 30737, 30793, 30848, 30904,
    30960, 31016, 31072, 31128, 31184, 31241, 31297, 31354,
    31410, 31467, 31524, 31581, 31638, 31695, 31752, 31810,
    31867, 31925, 31982, 32040, 32098, 32156, 32214, 32272,
    32331, 32389, 32448, 32506, 32565, 32624, 32683, 32742,
    32801, 32860, 32919, 32979, 33039, 33098, 33158, 33218,
    33278, 33338, 33398, 33459, 33519, 33580, 33640, 33701,
    33762, 33823, 33884, 33945, 34007, 34068, 34130, 34191,
    34253, 34315, 34377, 34439, 34501, 34564, 34626, 34689,
    34751, 34814, 34877, 34940, 35003, 35066, 35130, 35193,
    35257, 35320, 35384, 35448, 35512, 35576, 35641, 35705,
    35770, 35834, 35899, 35964, 36029, 36094, 36159, 36224,
    36290, 36355, 36421, 36487, 36553, 36619, 36685, 36751,
    36818, 36884, 36951, 37018, 37085, 37152, 37219, 37286,
    37353, 37421, 37488, 37556, 37624, 37692, 37760, 37828,
    37897, 37965, 38034, 38102, 38171, 38240, 38309, 38378,
    38448, 38517, 38587, 38657, 38726, 38796, 38866, 38937,
    39007, 39077, 39148, 39219, 39290, 39361, 39432, 39503,
    39574, 39646, 39718, 39789, 39861, 39933, 40005, 40078,
    40150, 40223, 40295, 40368, 40441, 40514, 40587, 40661,
    40734, 40808, 40881, 40955, 41029, 41103, 41178, 41252,
    41327, 41401, 41476, 41551, 41626, 41701, 41777, 41852,
    41928, 42003, 42079, 42155, 42231, 42308, 42384, 42461,
    42537, 42614, 42691, 42768, 42846, 42923, 43001, 43078,
    43156, 43234, 43312, 43391, 43469, 43547, 43626, 43705,
    43784, 43863, 43942, 44022, 44101, 44181, 44261, 44341,
    44421, 44501, 44581, 44662, 44743, 44823, 44904, 44986,
    45067, 45148, 45230, 45312, 45393, 45475, 45558, 45640
  };

/* dco index to db table (4 bits) */
static unsigned short S_apu_dco_index_to_db_table[8] = 
  { 1000, 594, 406, 281, 189, 115, 53, 0 };

/*************/
/* ENVELOPES */
/*************/
#define APU_ENV_PHASE_NUM_BLOCKS 13
#define APU_ENV_PHASE_TABLE_SIZE  8

#define APU_ENV_PHASE_MAX_INDEX 103 /* num_blocks * table_size - 1 */

/* envelope phase tables */
static unsigned short S_apu_env_phase_shifts_table[APU_ENV_PHASE_NUM_BLOCKS] = 
  { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0 };

static unsigned short S_apu_env_phase_steps_table[APU_ENV_PHASE_NUM_BLOCKS] = 
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 8 };

static unsigned short S_apu_env_phase_incs_table[APU_ENV_PHASE_TABLE_SIZE] = 
  { 32768, 36864, 40960, 45056, 49152, 53248, 57344, 61440 };

/* envelope index to db tables (6 bits) */
static unsigned short S_apu_env_index_to_db_rise_table[64] = 
  { 4095, 3583, 3135, 2743, 2400, 2100, 1838, 1608,
    1407, 1231, 1077,  943,  825,  722,  631,  553,
     483,  423,  370,  324,  283,  248,  217,  190,
     166,  145,  127,  111,   97,   85,   75,   65,
      57,   50,   44,   38,   33,   29,   26,   22,
      20,   17,   15,   13,   11,   10,    9,    8,
       7,    6,    5,    5,    4,    3,    3,    3,
       2,    2,    2,    2,    1,    1,    1,    1
  };

static unsigned short S_apu_env_index_to_db_fall_table[64] = 
  { 4095, 4030, 3965, 3900, 3835, 3770, 3705, 3640,
    3575, 3510, 3445, 3380, 3315, 3250, 3185, 3120,
    3055, 2990, 2925, 2860, 2795, 2730, 2665, 2600,
    2535, 2470, 2405, 2340, 2275, 2210, 2145, 2080,
    2015, 1950, 1885, 1820, 1755, 1690, 1625, 1560,
    1495, 1430, 1365, 1300, 1235, 1170, 1105, 1040,
     975,  910,  845,  780,  715,  650,  585,  520,
     455,  390,  325,  260,  195,  130,   65,    0
  };

/* patch param envelope rise/fall rate tables */
static unsigned short S_apu_param_env_speed_rise_table[32] = 
  {  16,  19,  22,  24,  27,  30,  33,  35,
     38,  41,  44,  46,  49,  52,  55,  57,
     60,  63,  66,  68,  71,  74,  77,  79,
     82,  85,  88,  90,  93,  96,  99, 101
  };

static unsigned short S_apu_param_env_speed_fall_table[32] = 
  {  0,   3,   6,   8,  11,  14,  17,  19,
    22,  25,  28,  30,  33,  36,  39,  41,
    44,  47,  50,  52,  55,  58,  61,  63,
    66,  69,  72,  74,  77,  80,  83,  85,
  };

/* envelope keyscaling */
#define APU_ENV_KEYSCALING_DIVISOR  256
#define APU_ENV_TIME_KS_BREAKPOINT  (APU_NOTE_MIDDLE_C - 4 * 12 + 0)  /* C-0 */
#define APU_ENV_LEVEL_KS_BREAKPOINT (APU_NOTE_MIDDLE_C - 2 * 12 + 9)  /* A-2 */

/* wavetables */
#define APU_WAVE_STEPS 32
#define APU_WAVE_BYTES (APU_WAVE_STEPS / 2)

#define APU_DCO_WAVETABLE_ENTRIES 16
#define APU_DCO_WAVETABLE_SIZE    (APU_DCO_WAVETABLE_ENTRIES * APU_WAVE_BYTES)

static unsigned char S_apu_dco_waves[APU_DCO_WAVETABLE_SIZE];

/*******/
/* LFO */
/*******/

#define APU_LFO_WAVETABLE_ENTRIES 8
#define APU_LFO_WAVETABLE_SIZE    (APU_LFO_WAVETABLE_ENTRIES * APU_WAVE_BYTES)

static unsigned char S_apu_lfo_waves[APU_LFO_WAVETABLE_SIZE] = 
  { 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98,
    0x76, 0x54, 0x32, 0x10, 0x01, 0x23, 0x45, 0x67,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
    0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
    0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
  };

/*******/
/* PCM */
/*******/

/* pcm index to db table (8 bits) */
static unsigned short S_apu_pcm_index_to_db_table[128] = 
  { 2047, 1641, 1452, 1328, 1235, 1161, 1099, 1046,
    1000,  959,  922,  889,  858,  829,  803,  778,
     755,  733,  713,  693,  675,  657,  641,  625,
     609,  594,  580,  567,  553,  541,  528,  516,
     505,  494,  483,  472,  462,  452,  442,  433,
     424,  415,  406,  397,  389,  381,  373,  365,
     357,  349,  342,  335,  328,  321,  314,  307,
     301,  294,  288,  281,  275,  269,  263,  257,
     252,  246,  240,  235,  229,  224,  219,  214,
     208,  203,  198,  194,  189,  184,  179,  174,
     170,  165,  161,  156,  152,  148,  143,  139,
     135,  131,  127,  123,  119,  115,  111,  107,
     103,   99,   95,   92,   88,   84,   81,   77,
      73,   70,   66,   63,   60,   56,   53,   50,
      46,   43,   40,   37,   33,   30,   27,   24,
      21,   18,   15,   12,    9,    6,    3,    0
  };

/***********/
/* FILTERS */
/***********/

/* highpass filters */
#define APU_HP_MULT_A0  32768
#define APU_HP_MULT_A1 -32709

#define APU_HP_MULT_B0  32739
#define APU_HP_MULT_B1 -32739

static short S_apu_hp_L_in[2];
static short S_apu_hp_L_out[2];

static short S_apu_hp_R_in[2];
static short S_apu_hp_R_out[2];

/* lowpass filters */
#define APU_LP_M 64

#define APU_LP_KERNEL_SIZE ((APU_LP_M / 2) + 1)
#define APU_LP_BUFFER_SIZE (APU_LP_M + 1)

static short S_apu_lp_kernel[APU_LP_KERNEL_SIZE] = 
  {    -3,   -18,   -30,   -35,   -28,    -6,    29,    67,
       93,    89,    43,   -42,  -143,  -222,  -236,  -156,
       17,   242,   442,   527,   425,   116,  -342,  -818,
    -1130, -1096,  -588,   415,  1805,  3356,  4774,  5768,
     6126
  };

static short S_apu_lp_L_in[APU_LP_BUFFER_SIZE];
static short S_apu_lp_R_in[APU_LP_BUFFER_SIZE];

static short S_apu_lp_buf_pos;

/**********/
/* VOICES */
/**********/

/* voices */
#define APU_NUM_DCO_VOICES 10
#define APU_NUM_PCM_VOICES  6

#define APU_ENV_STAGE_A 0
#define APU_ENV_STAGE_D 1
#define APU_ENV_STAGE_S 2
#define APU_ENV_STAGE_R 3

static unsigned char  S_apu_param_env_ar[APU_NUM_DCO_VOICES];
static unsigned char  S_apu_param_env_dr[APU_NUM_DCO_VOICES];
static unsigned char  S_apu_param_env_sr[APU_NUM_DCO_VOICES];
static unsigned char  S_apu_param_env_rr[APU_NUM_DCO_VOICES];
static unsigned char  S_apu_param_env_sl[APU_NUM_DCO_VOICES];

static unsigned char  S_apu_env_stage[APU_NUM_DCO_VOICES];
static unsigned short S_apu_env_speed[APU_NUM_DCO_VOICES];
static unsigned short S_apu_env_phase[APU_NUM_DCO_VOICES];
static unsigned short S_apu_env_index[APU_NUM_DCO_VOICES];

static unsigned char  S_apu_dco_note[APU_NUM_DCO_VOICES];
static unsigned short S_apu_dco_phase[APU_NUM_DCO_VOICES];
static unsigned short S_apu_dco_index[APU_NUM_DCO_VOICES];

static unsigned short S_apu_dco_level_db[APU_NUM_DCO_VOICES];
static unsigned char  S_apu_dco_level_sign[APU_NUM_DCO_VOICES];

/* output levels */
short G_apu_out_L;
short G_apu_out_R;

/******************************************************************************/
/* apu_reset()                                                                */
/******************************************************************************/
int apu_reset()
{
  int m;
  int n;

  S_apu_timer = 0;

  /* reset dco wavetables */
  for (m = 0; m < APU_DCO_WAVETABLE_ENTRIES; m++)
  {
    for (n = 0; n < (APU_WAVE_BYTES / 2); n++)
      S_apu_dco_waves[m * APU_WAVE_BYTES + n] = 0xFF;

    for (n = (APU_WAVE_BYTES / 2); n < APU_WAVE_BYTES; n++)
      S_apu_dco_waves[m * APU_WAVE_BYTES + n] = 0x00;
  }

  /* reset patch params */
  for (m = 0; m < APU_NUM_DCO_VOICES; m++)
  {
    S_apu_param_env_ar[m] = 0;
    S_apu_param_env_dr[m] = 0;
    S_apu_param_env_sr[m] = 0;
    S_apu_param_env_rr[m] = 0;
    S_apu_param_env_sl[m] = 0;
  }

  /* reset dco variables */
  for (m = 0; m < APU_NUM_DCO_VOICES; m++)
  {
    S_apu_env_stage[m] = APU_ENV_STAGE_R;
    S_apu_env_speed[m] = 0;
    S_apu_env_phase[m] = 0;
    S_apu_env_index[m] = 0;

    S_apu_dco_note[m] = APU_NOTE_MIDDLE_C;
    S_apu_dco_phase[m] = 0;
    S_apu_dco_index[m] = 0;
    S_apu_dco_level_db[m] = APU_VOL_LIN_MAX_INDEX;
    S_apu_dco_level_sign[m] = 0;
  }

  /* reset filters */
  for (m = 0; m < 2; m++)
  {
    S_apu_hp_L_in[m] = 0;
    S_apu_hp_L_out[m] = 0;

    S_apu_hp_R_in[m] = 0;
    S_apu_hp_R_out[m] = 0;
  }

  for (m = 0; m < APU_LP_BUFFER_SIZE; m++)
  {
    S_apu_lp_L_in[m] = 0;
    S_apu_lp_R_in[m] = 0;
  }

  S_apu_lp_buf_pos = 0;

  /* reset output */
  G_apu_out_L = 0;
  G_apu_out_R = 0;

  /* testing: setup the 1st oscillator envelope */
  S_apu_param_env_ar[0] = 24;
  S_apu_param_env_dr[0] = 16;
  S_apu_param_env_sr[0] = 8;
  S_apu_param_env_rr[0] = 16;
  S_apu_param_env_sl[0] = 8;

  S_apu_env_stage[0] = APU_ENV_STAGE_A;
  S_apu_env_speed[0] = S_apu_param_env_speed_rise_table[S_apu_param_env_ar[0]];
  S_apu_env_phase[0] = 0;
  S_apu_env_index[0] = 0;

  return 0;
}

/******************************************************************************/
/* apu_advance_sequencer()                                                    */
/******************************************************************************/
int apu_advance_sequencer()
{
  return 0;
}

/******************************************************************************/
/* apu_advance_lfos()                                                         */
/******************************************************************************/
int apu_advance_lfos()
{
  return 0;
}

/******************************************************************************/
/* apu_advance_envelopes()                                                    */
/******************************************************************************/
int apu_advance_envelopes()
{
  int m;

  unsigned short block;
  unsigned short entry;
  unsigned short increment;

  for (m = 0; m < APU_NUM_DCO_VOICES; m++)
  {
    /* testing: just update 1st voice for now */
    if (m > 0)
      continue;

    /* lookup phase increment */
    block = S_apu_env_speed[m] / APU_ENV_PHASE_TABLE_SIZE;
    entry = S_apu_env_speed[m] % APU_ENV_PHASE_TABLE_SIZE;

    increment = S_apu_env_phase_incs_table[entry];

    if (S_apu_env_phase_shifts_table[block] > 0)
      increment = increment >> S_apu_env_phase_shifts_table[block];

    /* update phase, check for wraparound */
    S_apu_env_phase[m] = (S_apu_env_phase[m] + increment) & 0xFFFF; 

    if (S_apu_env_phase[m] < increment)
    {
      /* attack */
      if (S_apu_env_stage[m] == APU_ENV_STAGE_A)
      {
        if (S_apu_env_index[m] <= (63 - S_apu_env_phase_steps_table[block]))
          S_apu_env_index[m] += S_apu_env_phase_steps_table[block];
        else
          S_apu_env_index[m] = 63;

        if (S_apu_env_index[m] == 63)
        {
          S_apu_env_stage[m] = APU_ENV_STAGE_D;

          S_apu_env_speed[m] = 
            S_apu_param_env_speed_fall_table[S_apu_param_env_dr[m]];
        }
      }
      /* decay */
      else if (S_apu_env_stage[m] == APU_ENV_STAGE_D)
      {
        if (S_apu_env_index[m] >= 0 + S_apu_env_phase_steps_table[block])
          S_apu_env_index[m] -= S_apu_env_phase_steps_table[block];
        else
          S_apu_env_index[m] = 0;

        if (S_apu_env_index[m] <= (48 + S_apu_param_env_sl[m]))
        {
          S_apu_env_stage[m] = APU_ENV_STAGE_S;

          S_apu_env_speed[m] = 
            S_apu_param_env_speed_fall_table[S_apu_param_env_sr[m]];
        }
      }
      /* sustain & release */
      else
      {
        if (S_apu_env_index[m] >= 0 + S_apu_env_phase_steps_table[block])
          S_apu_env_index[m] -= S_apu_env_phase_steps_table[block];
        else
          S_apu_env_index[m] = 0;
      }
    }
  }

  return 0;
}

/******************************************************************************/
/* apu_advance_dcos()                                                         */
/******************************************************************************/
int apu_advance_dcos()
{
  int m;

  unsigned short block;
  unsigned short entry;
  unsigned short increment;

  unsigned short addr;
  unsigned char  code;

  int val;

  for (m = 0; m < APU_NUM_DCO_VOICES; m++)
  {
    /* testing: just update 1st voice for now */
    if (m > 0)
      continue;

    /* lookup phase increment */
    val = S_apu_dco_note[m] * 32;

    if (val > APU_DCO_PHASE_MAX_INDEX)
      val = APU_DCO_PHASE_MAX_INDEX;
    else if (val < 0)
      val = 0;

    block = val / APU_DCO_PHASE_TABLE_SIZE;
    entry = val % APU_DCO_PHASE_TABLE_SIZE;

    increment = S_apu_dco_phase_incs_table[entry];

    if (S_apu_dco_phase_shifts_table[block] > 0)
      increment = increment >> S_apu_dco_phase_shifts_table[block];

    /* update phase, check for wraparound */
    S_apu_dco_phase[m] = (S_apu_dco_phase[m] + increment) & 0xFFFF; 

    if (S_apu_dco_phase[m] < increment)
    {
      S_apu_dco_index[m] += S_apu_dco_phase_steps_table[block];
      S_apu_dco_index[m] &= 0x1F;
    }

    /* wavetable lookup */
    addr = 0 * APU_WAVE_BYTES;

    if ((S_apu_dco_index[m] % 2) == 0)
      code = (S_apu_dco_waves[addr + (S_apu_dco_index[m] / 2)] >> 4) & 0x0F;
    else
      code = S_apu_dco_waves[addr + (S_apu_dco_index[m] / 2)] & 0x0F;

    /* determine wave level */
    if (code >= 8)
      val = S_apu_dco_index_to_db_table[code - 8];
    else
      val = S_apu_dco_index_to_db_table[7 - code];

#if 0
    /* testing: set level to 1/2 for now */
    if (m == 0)
      S_apu_dco_level_db[m] += 256;
#endif

    /* apply envelope */
    if (S_apu_env_stage[m] == APU_ENV_STAGE_A)
      val += S_apu_env_index_to_db_rise_table[S_apu_env_index[m]];
    else
      val += S_apu_env_index_to_db_fall_table[S_apu_env_index[m]];

    if (val > APU_VOL_LIN_MAX_INDEX)
      val = APU_VOL_LIN_MAX_INDEX;
    else if (val < 0)
      val = 0;

    /* set final level and sign */
    S_apu_dco_level_db[m] = val;

    if (code >= 8)
      S_apu_dco_level_sign[m] = 0;
    else
      S_apu_dco_level_sign[m] = 1;
  }

  return 0;
}

/******************************************************************************/
/* apu_advance_sample()                                                       */
/******************************************************************************/
int apu_advance_sample()
{
  int m;

  int samp_L;
  int samp_R;

  unsigned short index;
  unsigned short block;
  unsigned short shift;

  unsigned int val;

  /* compute current samples (left & right) */
  samp_L = 0;
  samp_R = 0;

  for (m = 0; m < APU_NUM_DCO_VOICES; m++)
  {
    index = S_apu_dco_level_db[m] % APU_VOL_LIN_TABLE_SIZE;
    block = S_apu_dco_level_db[m] / APU_VOL_LIN_TABLE_SIZE;

    shift = block;
    val = S_apu_vol_lin_table[index];

    if (shift > 0)
      val = val >> shift;

    if (S_apu_dco_level_sign[m] == 0)
      samp_L += val;
    else
      samp_L -= val;

    if (S_apu_dco_level_sign[m] == 0)
      samp_R += val;
    else
      samp_R -= val;
  }

  if (samp_L > 32767)
    samp_L = 32767;
  else if (samp_L < -32768)
    samp_L = -32768;

  if (samp_R > 32767)
    samp_R = 32767;
  else if (samp_R < -32768)
    samp_R = -32768;

  /* apply highpass filters (left & right) */
  S_apu_hp_L_in[1]  = S_apu_hp_L_in[0];
  S_apu_hp_L_out[1] = S_apu_hp_L_out[0];

  S_apu_hp_L_in[0] = samp_L;
  samp_L =  ((APU_HP_MULT_B0 * S_apu_hp_L_in[0]) / 32768) + 
            ((APU_HP_MULT_B1 * S_apu_hp_L_in[1]) / 32768) - 
            ((APU_HP_MULT_A1 * S_apu_hp_L_out[1]) / 32768);

  S_apu_hp_R_in[1]  = S_apu_hp_R_in[0];
  S_apu_hp_R_out[1] = S_apu_hp_R_out[0];

  S_apu_hp_R_in[0] = samp_R;
  samp_R =  ((APU_HP_MULT_B0 * S_apu_hp_R_in[0]) / 32768) + 
            ((APU_HP_MULT_B1 * S_apu_hp_R_in[1]) / 32768) - 
            ((APU_HP_MULT_A1 * S_apu_hp_R_out[1]) / 32768);

  if (samp_L > 32767)
    samp_L = 32767;
  else if (samp_L < -32768)
    samp_L = -32768;

  if (samp_R > 32767)
    samp_R = 32767;
  else if (samp_R < -32768)
    samp_R = -32768;

  S_apu_hp_L_out[0] = samp_L;
  S_apu_hp_R_out[0] = samp_R;

  /* update lowpass filter input buffers (left & right) */
  S_apu_lp_L_in[S_apu_lp_buf_pos] = S_apu_hp_L_out[0];
  S_apu_lp_R_in[S_apu_lp_buf_pos] = S_apu_hp_R_out[0];

  S_apu_lp_buf_pos = (S_apu_lp_buf_pos + 1) % APU_LP_BUFFER_SIZE;

  return 0;
}

/******************************************************************************/
/* apu_advance_output()                                                       */
/******************************************************************************/
int apu_advance_output()
{
  int m;

  int samp_L;
  int samp_R;

  int adj_pos;
  int inv_pos;

  short mult;

  /* apply lowpass filters (left & right) */
  adj_pos = (S_apu_lp_buf_pos + (APU_LP_M / 2)) % APU_LP_BUFFER_SIZE;

  mult = S_apu_lp_kernel[APU_LP_M / 2];

  samp_L = (mult * S_apu_lp_L_in[adj_pos]) / 32768;
  samp_R = (mult * S_apu_lp_R_in[adj_pos]) / 32768;

  for (m = 0; m < (APU_LP_M / 2); m++)
  {
    adj_pos = (S_apu_lp_buf_pos + m) % APU_LP_BUFFER_SIZE;
    inv_pos = (S_apu_lp_buf_pos + APU_LP_M - m) % APU_LP_BUFFER_SIZE;

    mult = S_apu_lp_kernel[m];

    samp_L += 
      (mult * (S_apu_lp_L_in[adj_pos] + S_apu_lp_L_in[inv_pos])) / 32768;

    samp_R += 
      (mult * (S_apu_lp_R_in[adj_pos] + S_apu_lp_R_in[inv_pos])) / 32768;
  }

#if 0
  samp_L = 0;
  samp_R = 0;

  for (m = 0; m < APU_LP_KERNEL_SIZE; m++)
  {
    adj_pos = (S_apu_lp_buf_pos + m) % APU_LP_KERNEL_SIZE;

    samp_L += (S_apu_lp_kernel[m] * S_apu_lp_L_in[adj_pos]) / 32768;
    samp_R += (S_apu_lp_kernel[m] * S_apu_lp_R_in[adj_pos]) / 32768;
  }
#endif

  if (samp_L > 32767)
    samp_L = 32767;
  else if (samp_L < -32768)
    samp_L = -32768;

  if (samp_R > 32767)
    samp_R = 32767;
  else if (samp_R < -32768)
    samp_R = -32768;

  G_apu_out_L = samp_L;
  G_apu_out_R = samp_R;

  return 0;
}

/******************************************************************************/
/* apu_update()                                                               */
/******************************************************************************/
int apu_update()
{
  int m;

  for (m = 0; m < APU_CLOCKS_PER_SAMPLE; m++)
  {
    if ((S_apu_timer % APU_SEQ_CLOCK_DIVIDER) == 0)
      apu_advance_sequencer();

    if ((S_apu_timer % APU_LFO_CLOCK_DIVIDER) == 0)
      apu_advance_lfos();

    if ((S_apu_timer % APU_ENV_CLOCK_DIVIDER) == 0)
      apu_advance_envelopes();

    apu_advance_dcos();
    apu_advance_sample();

    S_apu_timer += 1;

    if ((S_apu_timer % APU_TIMER_CLOCK_DIVIDER) == 0)
      S_apu_timer = 0;
  }

  apu_advance_output();

  return 0;
}

