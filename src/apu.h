/******************************************************************************/
/* apu.h (faux sound chip)                                                    */
/******************************************************************************/

#ifndef APU_H
#define APU_H

#define APU_OUT_SAMPLING_RATE   24000
#define APU_OUT_SAMPLES_PER_MS  (APU_OUT_SAMPLING_RATE / 1000)

/* output levels */
extern short G_apu_out_left;
extern short G_apu_out_right;

/* function declarations */
int apu_reset();
int apu_update();

#endif
