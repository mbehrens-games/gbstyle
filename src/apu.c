/******************************************************************************/
/* apu.c (faux sound chip)                                                    */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "apu.h"

#define PI 3.14159265358979323846f

/* note: the tables are generated in gnu octave */
/*       see the octave directory for .m files  */

/* clock, dividers, and timer */

/* the clock rate is a multiple of 1000 so that   */
/* there are an integer number of samples per ms. */
#define APU_CLOCK_RATE        96000
#define APU_CLOCKS_PER_SAMPLE (APU_CLOCK_RATE / APU_OUT_SAMPLING_RATE)

#define APU_ENV_CLOCK_DIVIDER    6  /* envelope clock is 16000  */
#define APU_LFO_CLOCK_DIVIDER   32  /* lfo clock is 3000        */
#define APU_SEQ_CLOCK_DIVIDER   16  /* sequencer clock is 6000  */

#define APU_TIMER_CLOCK_DIVIDER 48  /* lcm of the other dividers */

static unsigned short S_apu_timer;

/* db to linear table */
#define APU_DB_NUM_BLOCKS 16  /* blocks 0 to 15 */
#define APU_DB_TABLE_SIZE 256

#define APU_MAX_DB_INDEX  ((APU_DB_NUM_BLOCKS * APU_DB_TABLE_SIZE) - 1)

static unsigned short S_apu_db_table[APU_DB_TABLE_SIZE] = 
  { 32767, 32678, 32590, 32502, 32414, 32326, 32239, 32152,
    32065, 31978, 31892, 31805, 31719, 31634, 31548, 31463,
    31378, 31293, 31208, 31124, 31040, 30956, 30872, 30789,
    30705, 30622, 30540, 30457, 30375, 30293, 30211, 30129,
    30047, 29966, 29885, 29804, 29724, 29643, 29563, 29483,
    29404, 29324, 29245, 29166, 29087, 29008, 28930, 28852,
    28774, 28696, 28618, 28541, 28464, 28387, 28310, 28233,
    28157, 28081, 28005, 27929, 27854, 27778, 27703, 27628,
    27554, 27479, 27405, 27331, 27257, 27183, 27110, 27036,
    26963, 26890, 26818, 26745, 26673, 26601, 26529, 26457,
    26385, 26314, 26243, 26172, 26101, 26031, 25960, 25890,
    25820, 25750, 25681, 25611, 25542, 25473, 25404, 25335,
    25267, 25198, 25130, 25062, 24995, 24927, 24860, 24792,
    24725, 24659, 24592, 24525, 24459, 24393, 24327, 24261,
    24196, 24130, 24065, 24000, 23935, 23870, 23806, 23741,
    23677, 23613, 23549, 23486, 23422, 23359, 23296, 23233,
    23170, 23107, 23045, 22982, 22920, 22858, 22796, 22735,
    22673, 22612, 22551, 22490, 22429, 22368, 22308, 22248,
    22187, 22127, 22068, 22008, 21948, 21889, 21830, 21771,
    21712, 21653, 21595, 21536, 21478, 21420, 21362, 21304,
    21247, 21189, 21132, 21075, 21018, 20961, 20904, 20848,
    20791, 20735, 20679, 20623, 20568, 20512, 20456, 20401,
    20346, 20291, 20236, 20181, 20127, 20072, 20018, 19964,
    19910, 19856, 19802, 19749, 19696, 19642, 19589, 19536,
    19483, 19431, 19378, 19326, 19274, 19221, 19169, 19118,
    19066, 19014, 18963, 18912, 18861, 18810, 18759, 18708,
    18657, 18607, 18557, 18506, 18456, 18406, 18357, 18307,
    18258, 18208, 18159, 18110, 18061, 18012, 17963, 17915,
    17866, 17818, 17770, 17722, 17674, 17626, 17578, 17531,
    17483, 17436, 17389, 17342, 17295, 17248, 17202, 17155,
    17109, 17063, 17016, 16970, 16925, 16879, 16833, 16788,
    16742, 16697, 16652, 16607, 16562, 16517, 16472, 16428
  };

/* tuning */
#define APU_NOTE_LOWEST   ( 0 * 12 +  9) /* A-0 */
#define APU_NOTE_HIGHEST  ( 8 * 12 +  0) /* C-8 */
#define APU_NOTE_MIDDLE_C ( 4 * 12 +  0) /* C-4 */

/* phase registers */
#define APU_PHASE_REG_NUM_BITS  20
#define APU_PHASE_REG_SIZE      (1 << APU_PHASE_REG_NUM_BITS)
#define APU_PHASE_REG_MASK      (APU_PHASE_REG_SIZE - 1)

#define APU_PHASE_REG_MANTISSA_BITS 15

/* oscillator phase increment table */
#define APU_OSC_SEMITONE_STEPS        64
#define APU_OSC_NUM_OCTAVES            9 /* octaves 0 to 8 */
#define APU_OSC_PHASE_INC_TABLE_SIZE (12 * APU_OSC_SEMITONE_STEPS)

#define APU_OSC_PHASE_INC_MAX_INDEX  ((APU_OSC_NUM_OCTAVES * APU_OSC_PHASE_INC_TABLE_SIZE) - 1)

static unsigned int S_apu_osc_phase_inc_table[APU_OSC_PHASE_INC_TABLE_SIZE] = 
  { 45722,  45764,  45805,  45846,  45888,  45929,  45971,  46012,
    46054,  46095,  46137,  46179,  46220,  46262,  46304,  46346,
    46387,  46429,  46471,  46513,  46555,  46597,  46639,  46681,
    46724,  46766,  46808,  46850,  46893,  46935,  46977,  47020,
    47062,  47105,  47147,  47190,  47232,  47275,  47318,  47360,
    47403,  47446,  47489,  47532,  47575,  47618,  47661,  47704,
    47747,  47790,  47833,  47876,  47919,  47963,  48006,  48049,
    48093,  48136,  48180,  48223,  48267,  48310,  48354,  48397,
    48441,  48485,  48529,  48573,  48616,  48660,  48704,  48748,
    48792,  48836,  48880,  48924,  48969,  49013,  49057,  49101,
    49146,  49190,  49235,  49279,  49324,  49368,  49413,  49457,
    49502,  49547,  49591,  49636,  49681,  49726,  49771,  49816,
    49861,  49906,  49951,  49996,  50041,  50086,  50131,  50177,
    50222,  50267,  50313,  50358,  50404,  50449,  50495,  50540,
    50586,  50632,  50677,  50723,  50769,  50815,  50861,  50906,
    50952,  50998,  51044,  51091,  51137,  51183,  51229,  51275,
    51322,  51368,  51414,  51461,  51507,  51554,  51600,  51647,
    51694,  51740,  51787,  51834,  51880,  51927,  51974,  52021,
    52068,  52115,  52162,  52209,  52256,  52304,  52351,  52398,
    52445,  52493,  52540,  52588,  52635,  52683,  52730,  52778,
    52825,  52873,  52921,  52969,  53017,  53064,  53112,  53160,
    53208,  53256,  53304,  53353,  53401,  53449,  53497,  53545,
    53594,  53642,  53691,  53739,  53788,  53836,  53885,  53934,
    53982,  54031,  54080,  54129,  54177,  54226,  54275,  54324,
    54373,  54422,  54472,  54521,  54570,  54619,  54669,  54718,
    54767,  54817,  54866,  54916,  54965,  55015,  55065,  55114,
    55164,  55214,  55264,  55314,  55364,  55414,  55464,  55514,
    55564,  55614,  55664,  55715,  55765,  55815,  55866,  55916,
    55967,  56017,  56068,  56118,  56169,  56220,  56271,  56321,
    56372,  56423,  56474,  56525,  56576,  56627,  56678,  56729,
    56781,  56832,  56883,  56935,  56986,  57038,  57089,  57141,
    57192,  57244,  57295,  57347,  57399,  57451,  57503,  57555,
    57607,  57659,  57711,  57763,  57815,  57867,  57919,  57972,
    58024,  58076,  58129,  58181,  58234,  58286,  58339,  58392,
    58444,  58497,  58550,  58603,  58656,  58709,  58762,  58815,
    58868,  58921,  58974,  59028,  59081,  59134,  59188,  59241,
    59295,  59348,  59402,  59455,  59509,  59563,  59617,  59670,
    59724,  59778,  59832,  59886,  59940,  59994,  60049,  60103,
    60157,  60211,  60266,  60320,  60375,  60429,  60484,  60538,
    60593,  60648,  60702,  60757,  60812,  60867,  60922,  60977,
    61032,  61087,  61142,  61198,  61253,  61308,  61363,  61419,
    61474,  61530,  61585,  61641,  61697,  61752,  61808,  61864,
    61920,  61976,  62032,  62088,  62144,  62200,  62256,  62312,
    62368,  62425,  62481,  62538,  62594,  62651,  62707,  62764,
    62820,  62877,  62934,  62991,  63048,  63105,  63162,  63219,
    63276,  63333,  63390,  63447,  63505,  63562,  63619,  63677,
    63734,  63792,  63849,  63907,  63965,  64022,  64080,  64138,
    64196,  64254,  64312,  64370,  64428,  64486,  64545,  64603,
    64661,  64720,  64778,  64837,  64895,  64954,  65012,  65071,
    65130,  65189,  65247,  65306,  65365,  65424,  65483,  65543,
    65602,  65661,  65720,  65780,  65839,  65898,  65958,  66017,
    66077,  66137,  66196,  66256,  66316,  66376,  66436,  66496,
    66556,  66616,  66676,  66736,  66797,  66857,  66917,  66978,
    67038,  67099,  67159,  67220,  67281,  67341,  67402,  67463,
    67524,  67585,  67646,  67707,  67768,  67829,  67891,  67952,
    68013,  68075,  68136,  68198,  68259,  68321,  68383,  68444,
    68506,  68568,  68630,  68692,  68754,  68816,  68878,  68940,
    69003,  69065,  69127,  69190,  69252,  69315,  69377,  69440,
    69503,  69565,  69628,  69691,  69754,  69817,  69880,  69943,
    70006,  70069,  70133,  70196,  70259,  70323,  70386,  70450,
    70514,  70577,  70641,  70705,  70769,  70832,  70896,  70960,
    71025,  71089,  71153,  71217,  71281,  71346,  71410,  71475,
    71539,  71604,  71668,  71733,  71798,  71863,  71928,  71993,
    72058,  72123,  72188,  72253,  72318,  72384,  72449,  72514,
    72580,  72645,  72711,  72777,  72842,  72908,  72974,  73040,
    73106,  73172,  73238,  73304,  73370,  73436,  73503,  73569,
    73635,  73702,  73768,  73835,  73902,  73968,  74035,  74102,
    74169,  74236,  74303,  74370,  74437,  74505,  74572,  74639,
    74707,  74774,  74841,  74909,  74977,  75044,  75112,  75180,
    75248,  75316,  75384,  75452,  75520,  75588,  75656,  75725,
    75793,  75862,  75930,  75999,  76067,  76136,  76205,  76273,
    76342,  76411,  76480,  76549,  76618,  76688,  76757,  76826,
    76896,  76965,  77035,  77104,  77174,  77243,  77313,  77383,
    77453,  77523,  77593,  77663,  77733,  77803,  77873,  77944,
    78014,  78084,  78155,  78226,  78296,  78367,  78438,  78508,
    78579,  78650,  78721,  78792,  78864,  78935,  79006,  79077,
    79149,  79220,  79292,  79363,  79435,  79507,  79579,  79650,
    79722,  79794,  79866,  79938,  80011,  80083,  80155,  80228,
    80300,  80373,  80445,  80518,  80590,  80663,  80736,  80809,
    80882,  80955,  81028,  81101,  81174,  81248,  81321,  81395,
    81468,  81542,  81615,  81689,  81763,  81836,  81910,  81984,
    82058,  82132,  82207,  82281,  82355,  82430,  82504,  82578,
    82653,  82728,  82802,  82877,  82952,  83027,  83102,  83177,
    83252,  83327,  83402,  83478,  83553,  83628,  83704,  83780,
    83855,  83931,  84007,  84083,  84158,  84234,  84311,  84387,
    84463,  84539,  84615,  84692,  84768,  84845,  84921,  84998,
    85075,  85152,  85229,  85306,  85383,  85460,  85537,  85614,
    85691,  85769,  85846,  85924,  86001,  86079,  86157,  86234,
    86312,  86390,  86468,  86546,  86625,  86703,  86781,  86859,
    86938,  87016,  87095,  87174,  87252,  87331,  87410,  87489,
    87568,  87647,  87726,  87805,  87885,  87964,  88043,  88123,
    88202,  88282,  88362,  88441,  88521,  88601,  88681,  88761,
    88842,  88922,  89002,  89082,  89163,  89243,  89324,  89405,
    89485,  89566,  89647,  89728,  89809,  89890,  89971,  90052,
    90134,  90215,  90297,  90378,  90460,  90541,  90623,  90705,
    90787,  90869,  90951,  91033,  91115,  91198,  91280,  91362 
  };

/* envelope phase increment table */
#define APU_ENV_PHASE_INC_TABLE_SIZE (12 * 15) /* 12 semitones, 15 levels */
#define APU_ENV_NUM_OCTAVES           13

#define APU_ENV_PHASE_INC_MAX_INDEX  ((APU_ENV_NUM_OCTAVES * APU_ENV_PHASE_INC_TABLE_SIZE) - 1)

static unsigned int S_apu_phase_incs_env_table[APU_ENV_PHASE_INC_TABLE_SIZE] = 
  {  5461,   5786,   6130,   6495,   6881,   7290,   7723,   8183,   8669,   9185,   9731,  10310,
    10923,  11572,  12260,  12989,  13762,  14580,  15447,  16366,  17339,  18370,  19462,  20619,
    16384,  17358,  18390,  19484,  20643,  21870,  23170,  24548,  26008,  27554,  29193,  30929,
    21845,  23144,  24521,  25979,  27523,  29160,  30894,  32731,  34677,  36739,  38924,  41238,
    27307,  28930,  30651,  32473,  34404,  36450,  38617,  40914,  43347,  45924,  48655,  51548,
    32768,  34716,  36781,  38968,  41285,  43740,  46341,  49097,  52016,  55109,  58386,  61858,
    38229,  40503,  42911,  45463,  48166,  51030,  54064,  57279,  60685,  64294,  68117,  72167,
    43691,  46289,  49041,  51957,  55047,  58320,  61788,  65462,  69355,  73479,  77848,  82477,
    49152,  52075,  55171,  58452,  61928,  65610,  69511,  73645,  78024,  82663,  87579,  92787,
    54613,  57861,  61301,  64947,  68808,  72900,  77235,  81828,  86693,  91848,  97310, 103096,
    60075,  63647,  67432,  71441,  75689,  80190,  84958,  90010,  95363, 101033, 107041, 113406,
    65536,  69433,  73562,  77936,  82570,  87480,  92682,  98193, 104032, 110218, 116772, 123715,
    70997,  75219,  79692,  84431,  89451,  94770, 100405, 106376, 112701, 119403, 126503, 134025,
    76459,  81005,  85822,  90925,  96332, 102060, 108129, 114559, 121371, 128588, 136234, 144335,
    81920,  86791,  91952,  97420, 103213, 109350, 115852, 122741, 130040, 137772, 145965, 154644
  };

/* osc level to db table (4 bits) */
static unsigned short S_apu_steps_osc_table[8] = 
  { 1000,  594,  406,  281,  189,  115,   53,    0 };

/* pcm level to db table (8 bits) */
static unsigned short S_apu_steps_pcm_table[128] = 
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

/* envelope level to db tables (4 bits) */
static unsigned short S_apu_steps_env_lin_table[16] = 
  { 4095, 1000,  744,  594,  488,  406,  338,  281,
     232,  189,  150,  115,   82,   53,   25,    0
  };

static unsigned short S_apu_steps_env_exp_table[16] = 
  { 960,  896,  832,  768,  704,  640,  576,  512,
    448,  384,  320,  256,  192,  128,   64,    0
  };

/* envelope keyscaling */
#define APU_ENV_KEYSCALING_DIVISOR  256
#define APU_ENV_TIME_KS_BREAKPOINT  (APU_NOTE_MIDDLE_C - 4 * 12 + 0)  /* C-0 */
#define APU_ENV_LEVEL_KS_BREAKPOINT (APU_NOTE_MIDDLE_C - 2 * 12 + 9)  /* A-2 */

/* wavetables */
#define APU_WAVETABLE_STEPS 32
#define APU_WAVETABLE_BYTES (APU_WAVETABLE_STEPS / 2)

#define APU_WAVETABLE_BANK_NUM_WAVES  16
#define APU_WAVETABLE_BANK_SIZE       (APU_WAVETABLE_BANK_NUM_WAVES * APU_WAVETABLE_BYTES)

static unsigned char S_apu_wavetables[APU_WAVETABLE_BANK_SIZE] = 
  { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10, 0x01, 0x23, 0x45, 0x67,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88
  };

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

/* voices */
#define APU_NUM_WAVE_VOICES 12
#define APU_NUM_PCM_VOICES   4

static unsigned char  S_apu_osc_note[APU_NUM_WAVE_VOICES];
static unsigned int   S_apu_osc_phase[APU_NUM_WAVE_VOICES];
static unsigned short S_apu_osc_level_db[APU_NUM_WAVE_VOICES];
static unsigned char  S_apu_osc_level_sign[APU_NUM_WAVE_VOICES];

/* output levels */
short G_apu_out_L;
short G_apu_out_R;

/******************************************************************************/
/* apu_reset()                                                                */
/******************************************************************************/
int apu_reset()
{
  int m;

  S_apu_timer = 0;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    S_apu_osc_note[m] = APU_NOTE_MIDDLE_C;
    S_apu_osc_phase[m] = 0;
    S_apu_osc_level_db[m] = APU_MAX_DB_INDEX;
    S_apu_osc_level_sign[m] = 0;
  }

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

  G_apu_out_L = 0;
  G_apu_out_R = 0;

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
  return 0;
}

/******************************************************************************/
/* apu_advance_oscillators()                                                  */
/******************************************************************************/
int apu_advance_oscillators()
{
  int m;

  int total_pitch;

  unsigned short pitch_index;
  unsigned short pitch_octave;

  unsigned int   increment;
  unsigned short step;

  unsigned short index;
  unsigned char  code;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    /* testing: just update 1st voice for now */
    if (m > 0)
      continue;

    /* lookup phase increment */
    total_pitch = S_apu_osc_note[m] * APU_OSC_SEMITONE_STEPS;

    if (total_pitch > APU_OSC_PHASE_INC_MAX_INDEX)
      total_pitch = APU_OSC_PHASE_INC_MAX_INDEX;
    else if (total_pitch < 0)
      total_pitch = 0;

    pitch_index = total_pitch % APU_OSC_PHASE_INC_TABLE_SIZE;
    pitch_octave = total_pitch / APU_OSC_PHASE_INC_TABLE_SIZE;

    increment = S_apu_osc_phase_inc_table[pitch_index];

    if (pitch_octave < (APU_OSC_NUM_OCTAVES - 1))
      increment = increment >> (APU_OSC_NUM_OCTAVES - 1 - pitch_octave);

    /* update phase and wavetable step */
    S_apu_osc_phase[m] += increment;
    S_apu_osc_phase[m] &= APU_PHASE_REG_MASK;

    step = (S_apu_osc_phase[m] >> APU_PHASE_REG_MANTISSA_BITS) & 0x1F;

    index = 4 * APU_WAVETABLE_BYTES;

    if ((step % 2) == 0)
      code = (S_apu_wavetables[index + (step / 2)] >> 4) & 0x0F;
    else
      code = S_apu_wavetables[index + (step / 2)] & 0x0F;

    /* determine wave level */
    if (code >= 8)
    {
      S_apu_osc_level_db[m] = S_apu_steps_osc_table[code - 8];
      S_apu_osc_level_sign[m] = 0;
    }
    else
    {
      S_apu_osc_level_db[m] = S_apu_steps_osc_table[7 - code];
      S_apu_osc_level_sign[m] = 1;
    }

    /* testing: set level to 1/2 for now */
    S_apu_osc_level_db[m] += 256;

    if (S_apu_osc_level_db[m] > APU_MAX_DB_INDEX)
      S_apu_osc_level_db[m] = APU_MAX_DB_INDEX;
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

  short db_index;
  short db_block;

  /* compute current samples (left & right) */
  samp_L = 0;
  samp_R = 0;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    db_index = S_apu_osc_level_db[m] % APU_DB_TABLE_SIZE;
    db_block = S_apu_osc_level_db[m] / APU_DB_TABLE_SIZE;

    if (S_apu_osc_level_sign[m] == 0)
    {
      if (db_block == 0)
        samp_L += S_apu_db_table[db_index];
      else
        samp_L += S_apu_db_table[db_index] >> db_block;

      if (db_block == 0)
        samp_R += S_apu_db_table[db_index];
      else
        samp_R += S_apu_db_table[db_index] >> db_block;
    }
    else
    {
      if (db_block == 0)
        samp_L -= S_apu_db_table[db_index];
      else
        samp_L -= S_apu_db_table[db_index] >> db_block;

      if (db_block == 0)
        samp_R -= S_apu_db_table[db_index];
      else
        samp_R -= S_apu_db_table[db_index] >> db_block;
    }
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
  S_apu_hp_L_in[1] = S_apu_hp_L_in[0];
  S_apu_hp_L_in[0] = samp_L;

  S_apu_hp_L_out[1] = S_apu_hp_L_out[0];
  S_apu_hp_L_out[0] = ((APU_HP_MULT_B0 * S_apu_hp_L_in[0]) / 32768) + 
                      ((APU_HP_MULT_B1 * S_apu_hp_L_in[1]) / 32768) - 
                      ((APU_HP_MULT_A1 * S_apu_hp_L_out[1]) / 32768);

  S_apu_hp_R_in[1] = S_apu_hp_R_in[0];
  S_apu_hp_R_in[0] = samp_L;

  S_apu_hp_R_out[1] = S_apu_hp_R_out[0];
  S_apu_hp_R_out[0] = ((APU_HP_MULT_B0 * S_apu_hp_R_in[0]) / 32768) + 
                      ((APU_HP_MULT_B1 * S_apu_hp_R_in[1]) / 32768) - 
                      ((APU_HP_MULT_A1 * S_apu_hp_R_out[1]) / 32768);

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

    apu_advance_oscillators();
    apu_advance_sample();

    S_apu_timer += 1;

    if ((S_apu_timer % APU_TIMER_CLOCK_DIVIDER) == 0)
      S_apu_timer = 0;
  }

  apu_advance_output();

  return 0;
}

