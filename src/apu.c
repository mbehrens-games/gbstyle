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

/* tuning */
#define APU_NOTE_LOWEST   ( 0 * 12 +  9) /* A-0 */
#define APU_NOTE_HIGHEST  ( 8 * 12 +  0) /* C-8 */
#define APU_NOTE_MIDDLE_C ( 4 * 12 +  0) /* C-4 */

/* phase registers */
#define APU_PHASE_REG_NUM_BITS  20
#define APU_PHASE_REG_SIZE      (1 << APU_PHASE_REG_NUM_BITS)
#define APU_PHASE_REG_MASK      (APU_PHASE_REG_SIZE - 1)

#define APU_PHASE_REG_MANTISSA  15

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

/* dco step to db table (4 bits) */
static unsigned short S_apu_dco_step_db_table[8] = 
  { 1000,  594,  406,  281,  189,  115,   53,    0 };
     
/* pcm step to db table (8 bits) */
static unsigned short S_apu_pcm_step_db_table[128] = 
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

/* envelope step to db tables (6 bits) */
static unsigned short S_apu_env_step_db_rise_table[64] = 
  { 4095, 3583, 3135, 2743, 2400, 2100, 1838, 1608,
    1407, 1231, 1077,  943,  825,  722,  631,  553,
     483,  423,  370,  324,  283,  248,  217,  190,
     166,  145,  127,  111,   97,   85,   75,   65,
      57,   50,   44,   38,   33,   29,   26,   22,
      20,   17,   15,   13,   11,   10,    9,    8,
       7,    6,    5,    5,    4,    3,    3,    3,
       2,    2,    2,    2,    1,    1,    1,    1
  };

static unsigned short S_apu_env_step_db_fall_table[64] = 
  { 4095, 4030, 3965, 3900, 3835, 3770, 3705, 3640,
    3575, 3510, 3445, 3380, 3315, 3250, 3185, 3120,
    3055, 2990, 2925, 2860, 2795, 2730, 2665, 2600,
    2535, 2470, 2405, 2340, 2275, 2210, 2145, 2080,
    2015, 1950, 1885, 1820, 1755, 1690, 1625, 1560,
    1495, 1430, 1365, 1300, 1235, 1170, 1105, 1040,
     975,  910,  845,  780,  715,  650,  585,  520,
     455,  390,  325,  260,  195,  130,   65,    0
  };

/* dco phase increment table */
#define APU_DCO_PHASE_INC_SORTA_CENTS 64
#define APU_DCO_PHASE_INC_NUM_BLOCKS   9 /* octaves 0 to 8 */
#define APU_DCO_PHASE_INC_TABLE_SIZE  (12 * APU_DCO_PHASE_INC_SORTA_CENTS)

#define APU_DCO_PHASE_INC_MAX_INDEX   6911 /* num_blocks * table_size - 1 */

static unsigned int S_apu_dco_phase_inc_table[APU_DCO_PHASE_INC_TABLE_SIZE] = 
  {  45722,  45764,  45805,  45846,  45888,  45929,  45971,  46012,
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
#define APU_ENV_PHASE_INC_SORTA_CENTS 16
#define APU_ENV_PHASE_INC_NUM_BLOCKS  13
#define APU_ENV_PHASE_INC_TABLE_SIZE  (12 * APU_ENV_PHASE_INC_SORTA_CENTS)

#define APU_ENV_PHASE_INC_MAX_INDEX   2495 /* num_blocks * table_size - 1 */

static unsigned int S_apu_env_phase_inc_table[APU_ENV_PHASE_INC_TABLE_SIZE] = 
  { 344064, 345308, 346557, 347811, 349069, 350331, 351598, 352870,
    354146, 355427, 356712, 358002, 359297, 360596, 361901, 363210,
    364523, 365841, 367165, 368493, 369825, 371163, 372505, 373852,
    375204, 376561, 377923, 379290, 380662, 382039, 383420, 384807,
    386199, 387596, 388997, 390404, 391816, 393233, 394655, 396083,
    397515, 398953, 400396, 401844, 403297, 404756, 406220, 407689,
    409163, 410643, 412128, 413619, 415115, 416616, 418123, 419635,
    421153, 422676, 424205, 425739, 427279, 428824, 430375, 431931,
    433493, 435061, 436635, 438214, 439799, 441389, 442986, 444588,
    446196, 447810, 449429, 451055, 452686, 454323, 455966, 457615,
    459270, 460931, 462598, 464271, 465951, 467636, 469327, 471024,
    472728, 474438, 476154, 477876, 479604, 481339, 483079, 484827,
    486580, 488340, 490106, 491878, 493657, 495443, 497235, 499033,
    500838, 502649, 504467, 506292, 508123, 509960, 511805, 513656,
    515514, 517378, 519249, 521127, 523012, 524903, 526802, 528707,
    530619, 532538, 534464, 536397, 538337, 540284, 542238, 544199,
    546168, 548143, 550125, 552115, 554112, 556116, 558127, 560146,
    562171, 564205, 566245, 568293, 570348, 572411, 574481, 576559,
    578644, 580737, 582837, 584945, 587061, 589184, 591315, 593454,
    595600, 597754, 599916, 602086, 604263, 606449, 608642, 610843,
    613052, 615270, 617495, 619728, 621969, 624219, 626476, 628742,
    631016, 633298, 635589, 637887, 640194, 642510, 644834, 647166,
    649506, 651855, 654213, 656579, 658954, 661337, 663729, 666129,
    668538, 670956, 673383, 675818, 678262, 680715, 683177, 685648
  };

/* patch param envelope rise/fall rate tables */
static unsigned short S_apu_env_rise_rate_table[32] = 
  {  384,  448,  512,  576,  640,  704,  768,  832,
     896,  960, 1024, 1088, 1152, 1216, 1280, 1344,
    1408, 1472, 1536, 1600, 1664, 1728, 1792, 1856,
    1920, 1984, 2048, 2112, 2176, 2240, 2304, 2368
  };

static unsigned short S_apu_env_fall_rate_table[32] = 
  {    0,   64,  128,  192,  256,  320,  384,  448,
     512,  576,  640,  704,  768,  832,  896,  960,
    1024, 1088, 1152, 1216, 1280, 1344, 1408, 1472,
    1536, 1600, 1664, 1728, 1792, 1856, 1920, 1984
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
static unsigned int   S_apu_env_phase[APU_NUM_DCO_VOICES];
static unsigned short S_apu_env_step[APU_NUM_DCO_VOICES];

static unsigned char  S_apu_dco_note[APU_NUM_DCO_VOICES];
static unsigned int   S_apu_dco_phase[APU_NUM_DCO_VOICES];
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
    S_apu_env_phase[m] = 0;
    S_apu_env_step[m] = 0;

    S_apu_dco_note[m] = APU_NOTE_MIDDLE_C;
    S_apu_dco_phase[m] = 0;
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
  S_apu_param_env_sr[0] = 16;
  S_apu_param_env_rr[0] = 16;
  S_apu_param_env_sl[0] = 0;

  S_apu_env_stage[0] = APU_ENV_STAGE_A;

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

  unsigned short index;
  unsigned short block;
  unsigned short shift;

  unsigned int   increment;
  unsigned short step;

  int val;

  for (m = 0; m < APU_NUM_DCO_VOICES; m++)
  {
    /* testing: just update 1st voice for now */
    if (m > 0)
      continue;

    /* determine phase increment */
    if (S_apu_env_stage[m] == APU_ENV_STAGE_A)
      val = S_apu_env_rise_rate_table[S_apu_param_env_ar[m] & 0x1F];
    else if (S_apu_env_stage[m] == APU_ENV_STAGE_D)
      val = S_apu_env_fall_rate_table[S_apu_param_env_dr[m] & 0x1F];
    else if (S_apu_env_stage[m] == APU_ENV_STAGE_S)
      val = S_apu_env_fall_rate_table[S_apu_param_env_sr[m] & 0x1F];
    else
      val = S_apu_env_fall_rate_table[S_apu_param_env_rr[m] & 0x1F];

    if (val > APU_ENV_PHASE_INC_MAX_INDEX)
      val = APU_ENV_PHASE_INC_MAX_INDEX;
    else if (val < 0)
      val = 0;

    index = val % APU_ENV_PHASE_INC_TABLE_SIZE;
    block = val / APU_ENV_PHASE_INC_TABLE_SIZE;

    shift = (APU_ENV_PHASE_INC_NUM_BLOCKS - 1) - block;
    increment = S_apu_env_phase_inc_table[index];

    if (shift > 0)
      increment = increment >> shift;

    /* update phase and envelope step */
    S_apu_env_phase[m] += increment;

    if (S_apu_env_phase[m] >= APU_PHASE_REG_SIZE)
    {
      step = S_apu_env_step[m];

      if (S_apu_env_stage[m] == APU_ENV_STAGE_A)
      {
        if (step < 63)
          step += 1;

        if (step == 63)
          S_apu_env_stage[m] = APU_ENV_STAGE_D;
      }
      else
      {
        if (step > 0)
          step -= 1;
      }

      S_apu_env_step[m] = step;
    }

    S_apu_env_phase[m] &= APU_PHASE_REG_MASK;
  }

  return 0;
}

/******************************************************************************/
/* apu_advance_dcos()                                                         */
/******************************************************************************/
int apu_advance_dcos()
{
  int m;

  unsigned short index;
  unsigned short block;
  unsigned short shift;

  unsigned int   increment;
  unsigned short step;

  unsigned short addr;
  unsigned char  code;

  int val;

  for (m = 0; m < APU_NUM_DCO_VOICES; m++)
  {
    /* testing: just update 1st voice for now */
    if (m > 0)
      continue;

    /* determine phase increment */
    val = S_apu_dco_note[m] * APU_DCO_PHASE_INC_SORTA_CENTS;

    if (val > APU_DCO_PHASE_INC_MAX_INDEX)
      val = APU_DCO_PHASE_INC_MAX_INDEX;
    else if (val < 0)
      val = 0;

    index = val % APU_DCO_PHASE_INC_TABLE_SIZE;
    block = val / APU_DCO_PHASE_INC_TABLE_SIZE;

    shift = (APU_DCO_PHASE_INC_NUM_BLOCKS - 1) - block;
    increment = S_apu_dco_phase_inc_table[index];

    if (shift > 0)
      increment = increment >> shift;

    /* update phase and wavetable step */
    S_apu_dco_phase[m] += increment;
    S_apu_dco_phase[m] &= APU_PHASE_REG_MASK;

    step = (S_apu_dco_phase[m] >> APU_PHASE_REG_MANTISSA) & 0x1F;
    addr = 0 * APU_WAVE_BYTES;

    if ((step % 2) == 0)
      code = (S_apu_dco_waves[addr + (step / 2)] >> 4) & 0x0F;
    else
      code = S_apu_dco_waves[addr + (step / 2)] & 0x0F;

    /* determine wave level */
    if (code >= 8)
      val = S_apu_dco_step_db_table[code - 8];
    else
      val = S_apu_dco_step_db_table[7 - code];

#if 0
    /* testing: set level to 1/2 for now */
    if (m == 0)
      S_apu_dco_level_db[m] += 256;
#endif

    /* apply envelope */
    if (S_apu_env_stage[m] == APU_ENV_STAGE_A)
      val += S_apu_env_step_db_rise_table[S_apu_env_step[m]];
    else
      val += S_apu_env_step_db_fall_table[S_apu_env_step[m]];

    if (val > APU_VOL_LIN_MAX_INDEX)
      val = APU_VOL_LIN_MAX_INDEX;
    else if (val < 0)
      val = 0;

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

