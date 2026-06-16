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

/* db to linear table */
#define APU_DB_NUM_BLOCKS 16  /* blocks 0 to 15 */
#define APU_DB_TABLE_SIZE 256

#define APU_MAX_DB_INDEX  ((APU_DB_NUM_BLOCKS * APU_DB_TABLE_SIZE) - 1)

/* gnu octave code to generate the table:     */
/*   a = 0:255;                               */
/*   b = round(32767 * exp(-log(2) * a/256)); */
static unsigned short S_apu_db_to_linear_table[APU_DB_TABLE_SIZE] = 
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

/* envelope level to db table (4 bits, 0 to 1) */

/* gnu octave code to generate the table:     */
/*   a = 0:15;                                */
/*   b = round((-256 * log(a/15)) / log(2));  */
static unsigned short S_apu_env_level_to_db_table[16] = 
  { 4095, 1000,  744,  594,  488,  406,  338,  281, 
     232,  189,  150,  115,   82,   53,   25,    0 
  };

/* wave level to db table (5 bits, -1 to 1) */

/* gnu octave code to generate the table:               */
/*   a = 0:15;                                          */
/*   b = round(-256 * (log((2 * a + 1)/31) / log(2)));  */
static unsigned short S_apu_wave_wt_level_to_db_table[16] = 
  { 1268,  863,  674,  550,  457,  383,  321,  268, 
     222,  181,  144,  110,   79,   51,   25,    0 
  };

/* pcm level to db table (8 bits, -1 to 1) */

/* gnu octave code to generate the table:               */
/*   a = 0:127;                                         */
/*   b = round(-256 * (log((2 * a + 1)/255) / log(2))); */
static unsigned short S_apu_pcm_level_to_db_table[128] = 
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

/* wave pitch table */
/* this contains the phase increments for the highest octave, C8 - B8 */
#define APU_WAVE_SEMITONE_STEPS   64
#define APU_WAVE_NUM_OCTAVES       9 /* octaves 0 to 8 */
#define APU_WAVE_PITCH_TABLE_SIZE (12 * APU_WAVE_SEMITONE_STEPS)

#define APU_WAVE_MAX_PITCH_INDEX  ((APU_WAVE_NUM_OCTAVES * APU_WAVE_PITCH_TABLE_SIZE) - 1)

/* gnu octave code to generate the table:       */
/*   c8 = 440 * 16 * exp(log(2) * (-9/12));     */
/*   inc = exp(log(2) * 21) / 96000;            */
/*   a = 0:767;                                 */
/*   b = round(c8 * inc * exp(log(2) * a/768)); */
static unsigned int S_apu_wave_pitch_table[APU_WAVE_PITCH_TABLE_SIZE] = 
  {  91445,  91527,  91610,  91693,  91775,  91858,  91941,  92024, 
     92107,  92191,  92274,  92357,  92441,  92524,  92608,  92691, 
     92775,  92859,  92942,  93026,  93110,  93194,  93279,  93363, 
     93447,  93532,  93616,  93701,  93785,  93870,  93955,  94039, 
     94124,  94209,  94294,  94379,  94465,  94550,  94635,  94721, 
     94806,  94892,  94978,  95063,  95149,  95235,  95321,  95407, 
     95493,  95580,  95666,  95752,  95839,  95925,  96012,  96099, 
     96185,  96272,  96359,  96446,  96533,  96620,  96708,  96795, 
     96882,  96970,  97057,  97145,  97233,  97321,  97408,  97496, 
     97584,  97673,  97761,  97849,  97937,  98026,  98114,  98203, 
     98292,  98380,  98469,  98558,  98647,  98736,  98825,  98914, 
     99004,  99093,  99183,  99272,  99362,  99452,  99541,  99631, 
     99721,  99811,  99901,  99992, 100082, 100172, 100263, 100353, 
    100444, 100535, 100625, 100716, 100807, 100898, 100989, 101080, 
    101172, 101263, 101354, 101446, 101538, 101629, 101721, 101813, 
    101905, 101997, 102089, 102181, 102273, 102366, 102458, 102551, 
    102643, 102736, 102829, 102922, 103015, 103108, 103201, 103294, 
    103387, 103480, 103574, 103667, 103761, 103855, 103948, 104042, 
    104136, 104230, 104324, 104419, 104513, 104607, 104702, 104796, 
    104891, 104986, 105080, 105175, 105270, 105365, 105460, 105556, 
    105651, 105746, 105842, 105937, 106033, 106129, 106225, 106321, 
    106417, 106513, 106609, 106705, 106801, 106898, 106994, 107091, 
    107188, 107284, 107381, 107478, 107575, 107672, 107770, 107867, 
    107964, 108062, 108159, 108257, 108355, 108453, 108551, 108649, 
    108747, 108845, 108943, 109042, 109140, 109239, 109337, 109436, 
    109535, 109634, 109733, 109832, 109931, 110030, 110130, 110229, 
    110329, 110428, 110528, 110628, 110728, 110828, 110928, 111028, 
    111128, 111228, 111329, 111429, 111530, 111631, 111731, 111832, 
    111933, 112034, 112136, 112237, 112338, 112440, 112541, 112643, 
    112744, 112846, 112948, 113050, 113152, 113254, 113357, 113459, 
    113561, 113664, 113767, 113869, 113972, 114075, 114178, 114281, 
    114384, 114488, 114591, 114694, 114798, 114902, 115005, 115109, 
    115213, 115317, 115421, 115526, 115630, 115734, 115839, 115943, 
    116048, 116153, 116258, 116363, 116468, 116573, 116678, 116784, 
    116889, 116995, 117100, 117206, 117312, 117418, 117524, 117630, 
    117736, 117842, 117949, 118055, 118162, 118269, 118375, 118482, 
    118589, 118696, 118803, 118911, 119018, 119126, 119233, 119341, 
    119449, 119556, 119664, 119772, 119881, 119989, 120097, 120206, 
    120314, 120423, 120531, 120640, 120749, 120858, 120967, 121077, 
    121186, 121295, 121405, 121515, 121624, 121734, 121844, 121954, 
    122064, 122174, 122285, 122395, 122506, 122616, 122727, 122838, 
    122949, 123060, 123171, 123282, 123393, 123505, 123616, 123728, 
    123840, 123951, 124063, 124175, 124287, 124400, 124512, 124624, 
    124737, 124850, 124962, 125075, 125188, 125301, 125414, 125528, 
    125641, 125754, 125868, 125982, 126095, 126209, 126323, 126437, 
    126551, 126666, 126780, 126894, 127009, 127124, 127238, 127353, 
    127468, 127583, 127699, 127814, 127929, 128045, 128160, 128276, 
    128392, 128508, 128624, 128740, 128856, 128973, 129089, 129206, 
    129322, 129439, 129556, 129673, 129790, 129907, 130025, 130142, 
    130260, 130377, 130495, 130613, 130731, 130849, 130967, 131085, 
    131203, 131322, 131441, 131559, 131678, 131797, 131916, 132035, 
    132154, 132274, 132393, 132513, 132632, 132752, 132872, 132992, 
    133112, 133232, 133352, 133473, 133593, 133714, 133835, 133955, 
    134076, 134198, 134319, 134440, 134561, 134683, 134804, 134926, 
    135048, 135170, 135292, 135414, 135536, 135659, 135781, 135904, 
    136027, 136149, 136272, 136395, 136519, 136642, 136765, 136889, 
    137012, 137136, 137260, 137384, 137508, 137632, 137756, 137881, 
    138005, 138130, 138255, 138379, 138504, 138629, 138755, 138880, 
    139005, 139131, 139256, 139382, 139508, 139634, 139760, 139886, 
    140013, 140139, 140265, 140392, 140519, 140646, 140773, 140900, 
    141027, 141154, 141282, 141409, 141537, 141665, 141793, 141921, 
    142049, 142177, 142306, 142434, 142563, 142692, 142820, 142949, 
    143078, 143208, 143337, 143466, 143596, 143726, 143855, 143985, 
    144115, 144245, 144376, 144506, 144636, 144767, 144898, 145029, 
    145160, 145291, 145422, 145553, 145685, 145816, 145948, 146079, 
    146211, 146343, 146476, 146608, 146740, 146873, 147005, 147138, 
    147271, 147404, 147537, 147670, 147804, 147937, 148071, 148204, 
    148338, 148472, 148606, 148740, 148875, 149009, 149144, 149278, 
    149413, 149548, 149683, 149818, 149953, 150089, 150224, 150360, 
    150496, 150632, 150768, 150904, 151040, 151176, 151313, 151450, 
    151586, 151723, 151860, 151997, 152135, 152272, 152409, 152547, 
    152685, 152823, 152961, 153099, 153237, 153375, 153514, 153652, 
    153791, 153930, 154069, 154208, 154347, 154487, 154626, 154766, 
    154906, 155045, 155185, 155326, 155466, 155606, 155747, 155887, 
    156028, 156169, 156310, 156451, 156592, 156734, 156875, 157017, 
    157159, 157301, 157443, 157585, 157727, 157870, 158012, 158155, 
    158298, 158441, 158584, 158727, 158870, 159014, 159157, 159301, 
    159445, 159589, 159733, 159877, 160021, 160166, 160310, 160455, 
    160600, 160745, 160890, 161035, 161181, 161326, 161472, 161618, 
    161764, 161910, 162056, 162202, 162349, 162495, 162642, 162789, 
    162936, 163083, 163230, 163378, 163525, 163673, 163821, 163969, 
    164117, 164265, 164413, 164562, 164710, 164859, 165008, 165157, 
    165306, 165455, 165605, 165754, 165904, 166054, 166204, 166354, 
    166504, 166654, 166805, 166955, 167106, 167257, 167408, 167559, 
    167710, 167862, 168013, 168165, 168317, 168469, 168621, 168773, 
    168926, 169078, 169231, 169384, 169537, 169690, 169843, 169996, 
    170150, 170303, 170457, 170611, 170765, 170919, 171074, 171228, 
    171383, 171538, 171692, 171847, 172003, 172158, 172313, 172469, 
    172625, 172781, 172937, 173093, 173249, 173405, 173562, 173719, 
    173876, 174033, 174190, 174347, 174504, 174662, 174820, 174978, 
    175136, 175294, 175452, 175610, 175769, 175928, 176087, 176246, 
    176405, 176564, 176723, 176883, 177043, 177203, 177363, 177523, 
    177683, 177843, 178004, 178165, 178326, 178487, 178648, 178809, 
    178971, 179132, 179294, 179456, 179618, 179780, 179942, 180105, 
    180267, 180430, 180593, 180756, 180919, 181083, 181246, 181410, 
    181574, 181738, 181902, 182066, 182230, 182395, 182560, 182725
  };

/* envelope time table */

/* for the attack stage, the fastest rate should have a rise time of 6 ms   */
/* for the decay stage, the fastest rate should have a fall time of 24 ms   */
#define APU_ENV_TIME_TABLE_SIZE (12 * 16) /* 16 steps per semitone */
#define APU_ENV_NUM_OCTAVES     11

#define APU_ENV_MAX_TIME_INDEX  ((APU_ENV_NUM_OCTAVES * APU_ENV_TIME_TABLE_SIZE) - 1)

/* gnu octave code to generate the table:       */
/*   f = (32 / 0.006) * 0.5;                    */
/*   inc = exp(log(2) * 21) / (96000 / 6);      */
/*   a = 0:191;                                 */
/*   b = round(f * inc * exp(log(2) * a/192));  */
static unsigned int S_apu_env_time_table[APU_ENV_TIME_TABLE_SIZE] = 
  { 370309, 371648, 372993, 374342, 375695, 377054, 378418, 379787, 
    381160, 382539, 383922, 385311, 386704, 388103, 389506, 390915, 
    392329, 393748, 395172, 396601, 398035, 399475, 400920, 402370, 
    403825, 405286, 406751, 408222, 409699, 411181, 412668, 414160, 
    415658, 417161, 418670, 420184, 421704, 423229, 424760, 426296, 
    427838, 429385, 430938, 432497, 434061, 435631, 437206, 438787, 
    440374, 441967, 443565, 445170, 446780, 448396, 450017, 451645, 
    453278, 454918, 456563, 458214, 459871, 461535, 463204, 464879, 
    466560, 468248, 469941, 471641, 473347, 475059, 476777, 478501, 
    480232, 481968, 483712, 485461, 487217, 488979, 490747, 492522, 
    494303, 496091, 497885, 499686, 501493, 503307, 505127, 506954, 
    508788, 510628, 512475, 514328, 516188, 518055, 519929, 521809, 
    523696, 525590, 527491, 529399, 531314, 533235, 535164, 537099, 
    539042, 540991, 542948, 544912, 546882, 548860, 550845, 552837, 
    554837, 556844, 558857, 560879, 562907, 564943, 566986, 569037, 
    571095, 573160, 575233, 577314, 579402, 581497, 583600, 585711, 
    587829, 589955, 592089, 594230, 596379, 598536, 600701, 602874, 
    605054, 607242, 609438, 611643, 613855, 616075, 618303, 620539, 
    622783, 625036, 627296, 629565, 631842, 634127, 636421, 638722, 
    641032, 643351, 645677, 648013, 650356, 652708, 655069, 657438, 
    659816, 662202, 664597, 667001, 669413, 671834, 674264, 676703, 
    679150, 681606, 684071, 686546, 689029, 691521, 694022, 696532 
  };

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

/* the multipliers are fractions over */ 
/* 32768, expressed as numerators     */

/* gnu octave code to generate them:  */
/*   fs = 96000;                      */
/*   fc = 27.5 / (fs / 2);            */
/*   [b, a] = butter(1, fc, "high");  */
/*   d = round(32768 * b);            */
/*   c = round(32768 * a);            */
#define APU_HP_MULT_A0  32768
#define APU_HP_MULT_A1 -32709

#define APU_HP_MULT_B0  32739
#define APU_HP_MULT_B1 -32739

static short S_apu_hp_left_in[2];
static short S_apu_hp_left_out[2];

static short S_apu_hp_right_in[2];
static short S_apu_hp_right_out[2];

/* lowpass filters */
#define APU_LP_M 64

#define APU_LP_KERNEL_SIZE (APU_LP_M + 1)

/* gnu octave code to generate the kernel:  */
/*   fs = 96000                             */
/*   fc =  9000 / (fs / 2);                 */
/*   b = fir1(64, fc);                      */
/*   c = round(32768 * b);                  */
static short S_apu_lp_kernel[APU_LP_KERNEL_SIZE] = 
  {   -3,   -18,   -30,   -35,   -28,    -6,    29,    67, 
      93,    89,    43,   -42,  -143,  -222,  -236,  -156, 
      17,   242,   442,   527,   425,   116,  -342,  -818, 
   -1130, -1096,  -588,   415,  1805,  3356,  4774,  5768, 
    6126, 
    5768,  4774,  3356,  1805,   415,  -588, -1096, -1130, 
    -818,  -342,   116,   425,   527,   442,   242,    17, 
    -156,  -236,  -222,  -143,   -42,    43,    89,    93, 
      67,    29,    -6,   -28,   -35,   -30,   -18,    -3 
  };

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
    S_apu_wave_level_db[m] = APU_MAX_DB_INDEX;
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

  int total_pitch;

  unsigned short pitch_index;
  unsigned short pitch_octave;

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
    total_pitch = S_apu_wave_note[m] * APU_WAVE_SEMITONE_STEPS;

    if (total_pitch > APU_WAVE_MAX_PITCH_INDEX)
      total_pitch = APU_WAVE_MAX_PITCH_INDEX;
    else if (total_pitch < 0)
      total_pitch = 0;

    pitch_index = total_pitch % APU_WAVE_PITCH_TABLE_SIZE;
    pitch_octave = total_pitch / APU_WAVE_PITCH_TABLE_SIZE;

    increment = S_apu_wave_pitch_table[pitch_index];

    if (pitch_octave < (APU_WAVE_NUM_OCTAVES - 1))
      increment = increment >> (APU_WAVE_NUM_OCTAVES - 1 - pitch_octave);

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

      if (S_apu_wave_level_db[m] > APU_MAX_DB_INDEX)
        S_apu_wave_level_db[m] = APU_MAX_DB_INDEX;
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

  short db_index;
  short db_block;

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
      db_index = S_apu_wave_level_db[m] % APU_DB_TABLE_SIZE;
      db_block = S_apu_wave_level_db[m] / APU_DB_TABLE_SIZE;

      if (S_apu_wave_level_sign[m] == 0)
      {
        if (db_block == 0)
          samp_left += S_apu_db_to_linear_table[db_index];
        else
          samp_left += S_apu_db_to_linear_table[db_index] >> db_block;

        if (db_block == 0)
          samp_right += S_apu_db_to_linear_table[db_index];
        else
          samp_right += S_apu_db_to_linear_table[db_index] >> db_block;
      }
      else
      {
        if (db_block == 0)
          samp_left -= S_apu_db_to_linear_table[db_index];
        else
          samp_left -= S_apu_db_to_linear_table[db_index] >> db_block;

        if (db_block == 0)
          samp_right -= S_apu_db_to_linear_table[db_index];
        else
          samp_right -= S_apu_db_to_linear_table[db_index] >> db_block;
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
    S_apu_hp_left_out[0] = ((APU_HP_MULT_B0 * S_apu_hp_left_in[0]) / 32768) + 
                           ((APU_HP_MULT_B1 * S_apu_hp_left_in[1]) / 32768) - 
                           ((APU_HP_MULT_A1 * S_apu_hp_left_out[1]) / 32768);

    S_apu_hp_right_in[1] = S_apu_hp_right_in[0];
    S_apu_hp_right_in[0] = samp_left;

    S_apu_hp_right_out[1] = S_apu_hp_right_out[0];
    S_apu_hp_right_out[0] = ((APU_HP_MULT_B0 * S_apu_hp_right_in[0]) / 32768) + 
                            ((APU_HP_MULT_B1 * S_apu_hp_right_in[1]) / 32768) - 
                            ((APU_HP_MULT_A1 * S_apu_hp_right_out[1]) / 32768);

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

  unsigned char wavetable[APU_WT_STEPS];
  unsigned char level;

  int index;

  unsigned char bit;
  unsigned char mask;

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

  /* testing */
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

