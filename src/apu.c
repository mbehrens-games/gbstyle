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
#define APU_DCO_PHASE_TABLE_SIZE (12 * 64)

#define APU_DCO_PHASE_MAX_INDEX  6911 /* num_blocks * table_size - 1 */

/* dco phase tables */
static unsigned short S_apu_dco_phase_shifts_table[APU_DCO_PHASE_NUM_BLOCKS] = 
  { 6, 5, 4, 3, 2, 1, 0, 0, 0 };

static unsigned short S_apu_dco_phase_steps_table[APU_DCO_PHASE_NUM_BLOCKS] = 
  { 1, 1, 1, 1, 1, 1, 1, 2, 4 };

static unsigned short S_apu_dco_phase_incs_table[APU_DCO_PHASE_TABLE_SIZE] = 
  { 22861, 22882, 22902, 22923, 22944, 22965, 22985, 23006,
    23027, 23048, 23068, 23089, 23110, 23131, 23152, 23173,
    23194, 23215, 23236, 23257, 23278, 23299, 23320, 23341,
    23362, 23383, 23404, 23425, 23446, 23467, 23489, 23510,
    23531, 23552, 23574, 23595, 23616, 23638, 23659, 23680,
    23702, 23723, 23744, 23766, 23787, 23809, 23830, 23852,
    23873, 23895, 23916, 23938, 23960, 23981, 24003, 24025,
    24046, 24068, 24090, 24112, 24133, 24155, 24177, 24199,
    24221, 24242, 24264, 24286, 24308, 24330, 24352, 24374,
    24396, 24418, 24440, 24462, 24484, 24506, 24529, 24551,
    24573, 24595, 24617, 24640, 24662, 24684, 24706, 24729,
    24751, 24773, 24796, 24818, 24840, 24863, 24885, 24908,
    24930, 24953, 24975, 24998, 25020, 25043, 25066, 25088,
    25111, 25134, 25156, 25179, 25202, 25225, 25247, 25270,
    25293, 25316, 25339, 25362, 25384, 25407, 25430, 25453,
    25476, 25499, 25522, 25545, 25568, 25591, 25615, 25638,
    25661, 25684, 25707, 25730, 25754, 25777, 25800, 25823,
    25847, 25870, 25893, 25917, 25940, 25964, 25987, 26011,
    26034, 26058, 26081, 26105, 26128, 26152, 26175, 26199,
    26223, 26246, 26270, 26294, 26318, 26341, 26365, 26389,
    26413, 26437, 26460, 26484, 26508, 26532, 26556, 26580,
    26604, 26628, 26652, 26676, 26700, 26724, 26749, 26773,
    26797, 26821, 26845, 26870, 26894, 26918, 26942, 26967,
    26991, 27015, 27040, 27064, 27089, 27113, 27138, 27162,
    27187, 27211, 27236, 27260, 27285, 27310, 27334, 27359,
    27384, 27408, 27433, 27458, 27483, 27508, 27532, 27557,
    27582, 27607, 27632, 27657, 27682, 27707, 27732, 27757,
    27782, 27807, 27832, 27857, 27882, 27908, 27933, 27958,
    27983, 28009, 28034, 28059, 28085, 28110, 28135, 28161,
    28186, 28212, 28237, 28263, 28288, 28314, 28339, 28365,
    28390, 28416, 28442, 28467, 28493, 28519, 28545, 28570,
    28596, 28622, 28648, 28674, 28699, 28725, 28751, 28777,
    28803, 28829, 28855, 28881, 28907, 28934, 28960, 28986,
    29012, 29038, 29064, 29091, 29117, 29143, 29170, 29196,
    29222, 29249, 29275, 29301, 29328, 29354, 29381, 29407,
    29434, 29461, 29487, 29514, 29540, 29567, 29594, 29621,
    29647, 29674, 29701, 29728, 29755, 29781, 29808, 29835,
    29862, 29889, 29916, 29943, 29970, 29997, 30024, 30051,
    30079, 30106, 30133, 30160, 30187, 30215, 30242, 30269,
    30296, 30324, 30351, 30379, 30406, 30434, 30461, 30488,
    30516, 30544, 30571, 30599, 30626, 30654, 30682, 30709,
    30737, 30765, 30793, 30820, 30848, 30876, 30904, 30932,
    30960, 30988, 31016, 31044, 31072, 31100, 31128, 31156,
    31184, 31212, 31241, 31269, 31297, 31325, 31354, 31382,
    31410, 31439, 31467, 31495, 31524, 31552, 31581, 31609,
    31638, 31666, 31695, 31724, 31752, 31781, 31810, 31838,
    31867, 31896, 31925, 31953, 31982, 32011, 32040, 32069,
    32098, 32127, 32156, 32185, 32214, 32243, 32272, 32301,
    32331, 32360, 32389, 32418, 32448, 32477, 32506, 32536,
    32565, 32594, 32624, 32653, 32683, 32712, 32742, 32771,
    32801, 32830, 32860, 32890, 32919, 32949, 32979, 33009,
    33039, 33068, 33098, 33128, 33158, 33188, 33218, 33248,
    33278, 33308, 33338, 33368, 33398, 33428, 33459, 33489,
    33519, 33549, 33580, 33610, 33640, 33671, 33701, 33732,
    33762, 33792, 33823, 33854, 33884, 33915, 33945, 33976,
    34007, 34037, 34068, 34099, 34130, 34160, 34191, 34222,
    34253, 34284, 34315, 34346, 34377, 34408, 34439, 34470,
    34501, 34532, 34564, 34595, 34626, 34657, 34689, 34720,
    34751, 34783, 34814, 34846, 34877, 34908, 34940, 34972,
    35003, 35035, 35066, 35098, 35130, 35161, 35193, 35225,
    35257, 35289, 35320, 35352, 35384, 35416, 35448, 35480,
    35512, 35544, 35576, 35609, 35641, 35673, 35705, 35737,
    35770, 35802, 35834, 35867, 35899, 35931, 35964, 35996,
    36029, 36061, 36094, 36126, 36159, 36192, 36224, 36257,
    36290, 36323, 36355, 36388, 36421, 36454, 36487, 36520,
    36553, 36586, 36619, 36652, 36685, 36718, 36751, 36785,
    36818, 36851, 36884, 36918, 36951, 36984, 37018, 37051,
    37085, 37118, 37152, 37185, 37219, 37252, 37286, 37320,
    37353, 37387, 37421, 37455, 37488, 37522, 37556, 37590,
    37624, 37658, 37692, 37726, 37760, 37794, 37828, 37862,
    37897, 37931, 37965, 37999, 38034, 38068, 38102, 38137,
    38171, 38206, 38240, 38275, 38309, 38344, 38378, 38413,
    38448, 38483, 38517, 38552, 38587, 38622, 38657, 38691,
    38726, 38761, 38796, 38831, 38866, 38902, 38937, 38972,
    39007, 39042, 39077, 39113, 39148, 39183, 39219, 39254,
    39290, 39325, 39361, 39396, 39432, 39467, 39503, 39539,
    39574, 39610, 39646, 39682, 39718, 39753, 39789, 39825,
    39861, 39897, 39933, 39969, 40005, 40041, 40078, 40114,
    40150, 40186, 40223, 40259, 40295, 40332, 40368, 40404,
    40441, 40477, 40514, 40551, 40587, 40624, 40661, 40697,
    40734, 40771, 40808, 40844, 40881, 40918, 40955, 40992,
    41029, 41066, 41103, 41140, 41178, 41215, 41252, 41289,
    41327, 41364, 41401, 41439, 41476, 41513, 41551, 41588,
    41626, 41664, 41701, 41739, 41777, 41814, 41852, 41890,
    41928, 41965, 42003, 42041, 42079, 42117, 42155, 42193,
    42231, 42270, 42308, 42346, 42384, 42422, 42461, 42499,
    42537, 42576, 42614, 42653, 42691, 42730, 42768, 42807,
    42846, 42884, 42923, 42962, 43001, 43039, 43078, 43117,
    43156, 43195, 43234, 43273, 43312, 43351, 43391, 43430,
    43469, 43508, 43547, 43587, 43626, 43666, 43705, 43744,
    43784, 43823, 43863, 43903, 43942, 43982, 44022, 44061,
    44101, 44141, 44181, 44221, 44261, 44301, 44341, 44381,
    44421, 44461, 44501, 44541, 44581, 44622, 44662, 44702,
    44743, 44783, 44823, 44864, 44904, 44945, 44986, 45026,
    45067, 45108, 45148, 45189, 45230, 45271, 45312, 45352,
    45393, 45434, 45475, 45517, 45558, 45599, 45640, 45681
  };

/* dco value to db table (4 bits) */
static unsigned short S_apu_dco_val_to_db_table[8] = 
  { 1000, 594, 406, 281, 189, 115, 53, 0 };

/*************/
/* ENVELOPES */
/*************/
enum
{
  APU_ENV_STAGE_A = 0,
  APU_ENV_STAGE_D, 
  APU_ENV_STAGE_S, 
  APU_ENV_STAGE_R 
};

#define APU_ENV_PHASE_NUM_BLOCKS 13
#define APU_ENV_PHASE_TABLE_SIZE  8

#define APU_ENV_PHASE_MAX_INDEX 103 /* num_blocks * table_size - 1 */

/* envelope phase tables */
static unsigned char S_apu_env_phase_shifts_table[APU_ENV_PHASE_NUM_BLOCKS] = 
  { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0 };

static unsigned char S_apu_env_phase_steps_table[APU_ENV_PHASE_NUM_BLOCKS] = 
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 8 };

static unsigned short S_apu_env_phase_incs_table[APU_ENV_PHASE_TABLE_SIZE] = 
  { 32768, 36864, 40960, 45056, 49152, 53248, 57344, 61440 };

/* envelope value to db tables (6 bits) */
static unsigned short S_apu_env_rise_val_to_db_table[64] = 
  { 4095, 3583, 3135, 2743, 2400, 2100, 1838, 1608,
    1407, 1231, 1077,  943,  825,  722,  631,  553,
     483,  423,  370,  324,  283,  248,  217,  190,
     166,  145,  127,  111,   97,   85,   75,   65,
      57,   50,   44,   38,   33,   29,   26,   22,
      20,   17,   15,   13,   11,   10,    9,    8,
       7,    6,    5,    5,    4,    3,    3,    3,
       2,    2,    2,    2,    1,    1,    1,    1
  };

static unsigned short S_apu_env_fall_val_to_db_table[64] = 
  { 4095, 4030, 3965, 3900, 3835, 3770, 3705, 3640,
    3575, 3510, 3445, 3380, 3315, 3250, 3185, 3120,
    3055, 2990, 2925, 2860, 2795, 2730, 2665, 2600,
    2535, 2470, 2405, 2340, 2275, 2210, 2145, 2080,
    2015, 1950, 1885, 1820, 1755, 1690, 1625, 1560,
    1495, 1430, 1365, 1300, 1235, 1170, 1105, 1040,
     975,  910,  845,  780,  715,  650,  585,  520,
     455,  390,  325,  260,  195,  130,   65,    0
  };

/* envelope rise/fall rate tables */
static unsigned short S_apu_env_rise_rate_table[32] = 
  {  16,  19,  22,  24,  27,  30,  33,  35,
     38,  41,  44,  46,  49,  52,  55,  57,
     60,  63,  66,  68,  71,  74,  77,  79,
     82,  85,  88,  90,  93,  96,  99, 101
  };

static unsigned short S_apu_env_fall_rate_table[32] = 
  {  0,   3,   6,   8,  11,  14,  17,  19,
    22,  25,  28,  30,  33,  36,  39,  41,
    44,  47,  50,  52,  55,  58,  61,  63,
    66,  69,  72,  74,  77,  80,  83,  85,
  };

/* envelope keyscaling */
#define APU_ENV_KEYSCALING_DIVISOR  256
#define APU_ENV_TIME_KS_BREAKPOINT  (APU_NOTE_MIDDLE_C - 4 * 12 + 0)  /* C-0 */
#define APU_ENV_LEVEL_KS_BREAKPOINT (APU_NOTE_MIDDLE_C - 2 * 12 + 9)  /* A-2 */

/*******/
/* LFO */
/*******/

/* lfo phase increments */
static unsigned short S_apu_lfo_phase_incs_table[32] = 
  {  2796,  4194,  5592,  6991,  8389,  9787, 11185, 12583,
    13981, 15379, 16777, 18175, 19573, 20972, 22370, 23768,
    25166, 26564, 27962, 29360, 30758, 32156, 33554, 34953,
    36351, 37749, 39147, 40545, 41943, 43341, 44739, 46137
  }; 

/* lfo delays */
static unsigned short S_apu_lfo_delay_periods_table[32] = 
  {   0,   6,  12,  18,  23,  29,  35,  41,
     47,  53,  59,  64,  70,  76,  82,  88,
     94, 100, 105, 111, 117, 123, 129, 135,
    141, 146, 152, 158, 164, 170, 176, 182
  };

/* lfo sensitivities */
static unsigned char S_apu_lfo_vib_shifts_table[4] = 
  { 5, 4, 2, 1 };

static unsigned char S_apu_lfo_trem_shifts_table[2] = 
  { 2, 0 };

/* lfo step sizes */
static unsigned short S_apu_lfo_step_sizes_table[64] = 
  {  0,  2,  5,  7, 10, 12, 15, 17,
     2,  4,  6,  8, 11, 13, 15, 17,
     5,  7,  8, 10, 12, 14, 15, 17,
     7,  8, 10, 11, 13, 14, 16, 17,
    10, 11, 12, 13, 14, 15, 16, 17,
    12, 13, 13, 14, 15, 16, 16, 17,
    15, 15, 16, 16, 16, 16, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17
  };

/*******/
/* PCM */
/*******/

/* pcm phase increments */
static unsigned short S_apu_pcm_phase_incs[4] = 
  { 22579, 24576, 45158, 49152 };

/* pcm value to db table (8 bits) */
static unsigned short S_apu_pcm_val_to_db_table[128] = 
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

/***********/
/* PATCHES */
/***********/
enum
{
  APU_PARAM_BYTE_ENV_AR = 0, 
  APU_PARAM_BYTE_ENV_DR, 
  APU_PARAM_BYTE_ENV_SR, 
  APU_PARAM_BYTE_ENV_RR, 
  APU_PARAM_BYTE_ENV_SL, 
  APU_PARAM_BYTE_LFO_SPEED, 
  APU_PARAM_BYTE_LFO_DELAY, 
  APU_PARAM_BYTE_VIB_SENS_DEPTH, 
  APU_PARAM_BYTE_TREM_SENS_DEPTH, 
  APU_NUM_PARAM_BYTES 
};

#define APU_NUM_PATCHES       16
#define APU_PATCH_PARAMS_SIZE (APU_NUM_PATCHES * APU_NUM_PARAM_BYTES)

static unsigned char S_apu_patch_params[APU_PATCH_PARAMS_SIZE];

#define APU_PARAM_BYTE(patch_num, param)                                       \
  S_apu_patch_params[patch_num * APU_NUM_PARAM_BYTES + APU_PARAM_BYTE_##param]

#define APU_NUM_WAVE_BYTES    (32 / 2)
#define APU_PATCH_WAVES_SIZE  (APU_NUM_PATCHES * APU_NUM_WAVE_BYTES)

static unsigned char S_apu_patch_waves[APU_PATCH_WAVES_SIZE];

/**********/
/* VOICES */
/**********/
enum
{
  APU_WAVE_REG_LFO_PHASE = 0, 
  APU_WAVE_REG_LFO_INDEX, 
  APU_WAVE_REG_VIB_LEVEL, 
  APU_WAVE_REG_TREM_LEVEL, 
  APU_WAVE_REG_ENV_STAGE, 
  APU_WAVE_REG_ENV_PHASE, 
  APU_WAVE_REG_ENV_INDEX, 
  APU_WAVE_REG_ENV_LEVEL, 
  APU_WAVE_REG_DCO_NOTE, 
  APU_WAVE_REG_DCO_PHASE, 
  APU_WAVE_REG_DCO_INDEX, 
  APU_WAVE_REG_WAVE_LEVEL, 
  APU_NUM_WAVE_REGS 
};

#define APU_NUM_WAVE_VOICES   10
#define APU_WAVE_VOICES_SIZE  (APU_NUM_WAVE_VOICES * APU_NUM_WAVE_REGS)

static unsigned short S_apu_wave_voices[APU_WAVE_VOICES_SIZE];

#define APU_WAVE_REG(voice_num, reg)                                           \
  S_apu_wave_voices[voice_num * APU_NUM_WAVE_REGS + APU_WAVE_REG_##reg]

enum
{
  APU_PCM_REG_PHASE = 0, 
  APU_PCM_REG_INDEX, 
  APU_PCM_REG_LEVEL, 
  APU_NUM_PCM_REGS 
};

#define APU_NUM_PCM_VOICES    6
#define APU_PCM_VOICES_SIZE   (APU_NUM_PCM_VOICES * APU_NUM_PCM_REGS)

static unsigned short S_apu_pcm_voices[APU_PCM_VOICES_SIZE];

#define APU_PCM_REG(voice_num, reg)                                            \
  S_apu_pcm_voices[voice_num * APU_NUM_PCM_REGS + APU_PCM_REG_##reg]

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

  /* reset patch params */
  for (m = 0; m < APU_NUM_PATCHES; m++)
  {
    APU_PARAM_BYTE(m, ENV_AR) = 0;
    APU_PARAM_BYTE(m, ENV_DR) = 0;
    APU_PARAM_BYTE(m, ENV_SR) = 0;
    APU_PARAM_BYTE(m, ENV_RR) = 0;
    APU_PARAM_BYTE(m, ENV_SL) = 0;

    APU_PARAM_BYTE(m, LFO_SPEED) = 0;
    APU_PARAM_BYTE(m, LFO_DELAY) = 0;
    APU_PARAM_BYTE(m, VIB_SENS_DEPTH) = 0;
    APU_PARAM_BYTE(m, TREM_SENS_DEPTH) = 0;
  }

  /* reset patch waves */
  for (m = 0; m < APU_NUM_PATCHES; m++)
  {
    for (n = 0; n < APU_NUM_WAVE_BYTES; n++)
    {
      if (n < (APU_NUM_WAVE_BYTES / 2))
        S_apu_patch_waves[m * APU_NUM_WAVE_BYTES + n] = 0xFF;
      else
        S_apu_patch_waves[m * APU_NUM_WAVE_BYTES + n] = 0x00;
    }
  }

  /* reset voice registers */
  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    APU_WAVE_REG(m, LFO_PHASE)  = 0;
    APU_WAVE_REG(m, LFO_INDEX)  = 0;
    APU_WAVE_REG(m, VIB_LEVEL)  = 0;
    APU_WAVE_REG(m, TREM_LEVEL) = 0;
    APU_WAVE_REG(m, ENV_STAGE)  = APU_ENV_STAGE_R;
    APU_WAVE_REG(m, ENV_PHASE)  = 0;
    APU_WAVE_REG(m, ENV_INDEX)  = 0;
    APU_WAVE_REG(m, ENV_LEVEL)  = APU_VOL_LIN_MAX_INDEX;
    APU_WAVE_REG(m, DCO_NOTE)   = APU_NOTE_MIDDLE_C;
    APU_WAVE_REG(m, DCO_PHASE)  = 0;
    APU_WAVE_REG(m, DCO_INDEX)  = 0;
    APU_WAVE_REG(m, WAVE_LEVEL) = APU_VOL_LIN_MAX_INDEX;
  }

  for (m = 0; m < APU_NUM_PCM_VOICES; m++)
  {
    APU_PCM_REG(m, PHASE) = 0;
    APU_PCM_REG(m, INDEX) = 0;
    APU_PCM_REG(m, LEVEL) = APU_VOL_LIN_MAX_INDEX;
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

  /* testing: setup the 1st oscillator */
  APU_PARAM_BYTE(0, ENV_AR) = 24;
  APU_PARAM_BYTE(0, ENV_DR) = 16;
  APU_PARAM_BYTE(0, ENV_SR) = 8;
  APU_PARAM_BYTE(0, ENV_RR) = 16;
  APU_PARAM_BYTE(0, ENV_SL) = 8;

  APU_PARAM_BYTE(0, LFO_SPEED) = 24;
  APU_PARAM_BYTE(0, LFO_DELAY) = 0;
  APU_PARAM_BYTE(0, VIB_SENS_DEPTH) =  (1 << 3) | 7;
  APU_PARAM_BYTE(0, TREM_SENS_DEPTH) = (0 << 3) | 0;

  APU_WAVE_REG(0, ENV_STAGE) = APU_ENV_STAGE_A;
  APU_WAVE_REG(0, ENV_PHASE) = 0;

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
  int m;

  unsigned short increment;
  unsigned short vib_level;
  unsigned short trem_level;

  unsigned char wave_step;
  unsigned char vib_depth;
  unsigned char vib_sens;
  unsigned char trem_depth;
  unsigned char trem_sens;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    /* lookup phase increment */
    increment = S_apu_lfo_phase_incs_table[APU_PARAM_BYTE(m, LFO_SPEED)];

    /* update phase, check for wraparound */
    APU_WAVE_REG(m, LFO_PHASE) += increment;
    APU_WAVE_REG(m, LFO_PHASE) &= 0xFFFF;

    if (APU_WAVE_REG(m, LFO_PHASE) < increment)
    {
      APU_WAVE_REG(m, LFO_INDEX) += 1;
      APU_WAVE_REG(m, LFO_INDEX) &= 0x1F;
    }

    /* wavetable lookup */
    /* testing: just a triangle for now! */
    if (APU_WAVE_REG(m, LFO_INDEX) < 16)
      wave_step = APU_WAVE_REG(m, LFO_INDEX);
    else
      wave_step = 31 - APU_WAVE_REG(m, LFO_INDEX);

    /* obtain depth and sensitivity params */
    vib_depth = APU_PARAM_BYTE(m, VIB_SENS_DEPTH) & 0x07;
    vib_sens = (APU_PARAM_BYTE(m, VIB_SENS_DEPTH) >> 3) & 0x03;

    trem_depth = APU_PARAM_BYTE(m, TREM_SENS_DEPTH) & 0x07;
    trem_sens = (APU_PARAM_BYTE(m, TREM_SENS_DEPTH) >> 3) & 0x01;

    /* determine initial levels */
    vib_level = wave_step * S_apu_lfo_step_sizes_table[8 * vib_depth + 0];
    trem_level = wave_step * S_apu_lfo_step_sizes_table[8 * trem_depth + 0];

    /* apply sensitivity */
    if (S_apu_lfo_vib_shifts_table[vib_sens] > 0)
      vib_level = vib_level >> S_apu_lfo_vib_shifts_table[vib_sens];

    if (S_apu_lfo_trem_shifts_table[trem_sens] > 0)
      trem_level = trem_level >> S_apu_lfo_trem_shifts_table[trem_sens];

    /* set levels */
    APU_WAVE_REG(m, VIB_LEVEL) = vib_level;
    APU_WAVE_REG(m, TREM_LEVEL) = trem_level;
  }

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

  unsigned short rate;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    /* determine envelope rate */
    if (APU_WAVE_REG(m, ENV_STAGE) == APU_ENV_STAGE_A)
      rate = S_apu_env_rise_rate_table[APU_PARAM_BYTE(m, ENV_AR)];
    else if (APU_WAVE_REG(m, ENV_STAGE) == APU_ENV_STAGE_D)
      rate = S_apu_env_fall_rate_table[APU_PARAM_BYTE(m, ENV_DR)];
    else if (APU_WAVE_REG(m, ENV_STAGE) == APU_ENV_STAGE_S)
      rate = S_apu_env_fall_rate_table[APU_PARAM_BYTE(m, ENV_SR)];
    else
      rate = S_apu_env_fall_rate_table[APU_PARAM_BYTE(m, ENV_RR)];

    /* lookup phase increment */
    block = rate / APU_ENV_PHASE_TABLE_SIZE;
    entry = rate % APU_ENV_PHASE_TABLE_SIZE;

    increment = S_apu_env_phase_incs_table[entry];

    if (S_apu_env_phase_shifts_table[block] > 0)
      increment = increment >> S_apu_env_phase_shifts_table[block];

    /* update phase, check for wraparound */
    APU_WAVE_REG(m, ENV_PHASE) += increment;
    APU_WAVE_REG(m, ENV_PHASE) &= 0xFFFF; 

    if (APU_WAVE_REG(m, ENV_PHASE) < increment)
    {
      /* attack */
      if (APU_WAVE_REG(m, ENV_STAGE) == APU_ENV_STAGE_A)
      {
        if (APU_WAVE_REG(m, ENV_INDEX) <= (63 - S_apu_env_phase_steps_table[block]))
          APU_WAVE_REG(m, ENV_INDEX) += S_apu_env_phase_steps_table[block];
        else
          APU_WAVE_REG(m, ENV_INDEX) = 63;

        if (APU_WAVE_REG(m, ENV_INDEX) == 63)
          APU_WAVE_REG(m, ENV_STAGE) = APU_ENV_STAGE_D;
      }
      /* decay */
      else if (APU_WAVE_REG(m, ENV_STAGE) == APU_ENV_STAGE_D)
      {
        if (APU_WAVE_REG(m, ENV_INDEX) >= S_apu_env_phase_steps_table[block])
          APU_WAVE_REG(m, ENV_INDEX) -= S_apu_env_phase_steps_table[block];
        else
          APU_WAVE_REG(m, ENV_INDEX) = 0;

        if (APU_WAVE_REG(m, ENV_INDEX) <= (48 + APU_PARAM_BYTE(m, ENV_SL)))
          APU_WAVE_REG(m, ENV_STAGE) = APU_ENV_STAGE_S;
      }
      /* sustain & release */
      else
      {
        if (APU_WAVE_REG(m, ENV_INDEX) >= S_apu_env_phase_steps_table[block])
          APU_WAVE_REG(m, ENV_INDEX) -= S_apu_env_phase_steps_table[block];
        else
          APU_WAVE_REG(m, ENV_INDEX) = 0;
      }

      /* update envelope level */
      if (APU_WAVE_REG(m, ENV_STAGE) == APU_ENV_STAGE_A)
      {
        APU_WAVE_REG(m, ENV_LEVEL) = 
          S_apu_env_rise_val_to_db_table[APU_WAVE_REG(m, ENV_INDEX)];
      }
      else
      {
        APU_WAVE_REG(m, ENV_LEVEL) = 
          S_apu_env_fall_val_to_db_table[APU_WAVE_REG(m, ENV_INDEX)];
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

  int phase_index;
  int final_level_db;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    /* lookup phase increment, apply vibrato */
    phase_index = APU_WAVE_REG(m, DCO_NOTE) * 64;
    phase_index += APU_WAVE_REG(m, VIB_LEVEL);

    if (phase_index > APU_DCO_PHASE_MAX_INDEX)
      phase_index = APU_DCO_PHASE_MAX_INDEX;
    else if (phase_index < 0)
      phase_index = 0;

    block = phase_index / APU_DCO_PHASE_TABLE_SIZE;
    entry = phase_index % APU_DCO_PHASE_TABLE_SIZE;

    increment = S_apu_dco_phase_incs_table[entry];

    if (S_apu_dco_phase_shifts_table[block] > 0)
      increment = increment >> S_apu_dco_phase_shifts_table[block];

    /* update phase, check for wraparound */
    APU_WAVE_REG(m, DCO_PHASE) += increment;
    APU_WAVE_REG(m, DCO_PHASE) &= 0xFFFF;

    if (APU_WAVE_REG(m, DCO_PHASE) < increment)
    {
      APU_WAVE_REG(m, DCO_INDEX) += S_apu_dco_phase_steps_table[block];
      APU_WAVE_REG(m, DCO_INDEX) &= 0x1F;
    }

    /* wavetable lookup */
    addr = 0 * APU_NUM_WAVE_BYTES;
    addr += (APU_WAVE_REG(m, DCO_INDEX) / 2);

    if ((APU_WAVE_REG(m, DCO_INDEX) % 2) == 0)
      code = (S_apu_patch_waves[addr] >> 4) & 0x0F;
    else
      code = S_apu_patch_waves[addr] & 0x0F;

    /* determine wave level, apply envelope and tremolo */
    if (code >= 8)
      final_level_db = S_apu_dco_val_to_db_table[code - 8];
    else
      final_level_db = S_apu_dco_val_to_db_table[7 - code];

    final_level_db += APU_WAVE_REG(m, ENV_LEVEL);
    final_level_db += APU_WAVE_REG(m, TREM_LEVEL);

    if (final_level_db > APU_VOL_LIN_MAX_INDEX)
      final_level_db = APU_VOL_LIN_MAX_INDEX;
    else if (final_level_db < 0)
      final_level_db = 0;

    /* set final level and sign */
    APU_WAVE_REG(m, WAVE_LEVEL) = final_level_db & 0x0FFF;

    if (code < 8)
      APU_WAVE_REG(m, WAVE_LEVEL) |= 0x1000;
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

  unsigned short block;
  unsigned short entry;

  unsigned int voice_level;

  /* compute current samples (left & right) */
  samp_L = 0;
  samp_R = 0;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    block = (APU_WAVE_REG(m, WAVE_LEVEL) & 0x0FFF) / APU_VOL_LIN_TABLE_SIZE;
    entry = (APU_WAVE_REG(m, WAVE_LEVEL) & 0x0FFF) % APU_VOL_LIN_TABLE_SIZE;

    voice_level = S_apu_vol_lin_table[entry];

    if (block > 0)
      voice_level = voice_level >> block;

    if (APU_WAVE_REG(m, WAVE_LEVEL) & 0x1000)
      samp_L -= voice_level;
    else
      samp_L += voice_level;

    if (APU_WAVE_REG(m, WAVE_LEVEL) & 0x1000)
      samp_R -= voice_level;
    else
      samp_R += voice_level;
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

