clear;
clc;

% lfo phase incs
lfo_1hz_inc = exp(log(2) * 21) / 375; % 21 bit phase, 375 hz clock
lfo_freqs = 0.5 + ((8 * (0:31)) / 32);

lfo_phase_incs = round(lfo_1hz_inc * lfo_freqs);

% lfo delay amounts
lfo_delay_sec = (0.5 * (0:31)) / 32;
lfo_delay_periods = round(375 * lfo_delay_sec); % 375 hz clock

% vibrato (max sensitivity: 2 semitones)
lfo_vib_shifts = [7, 6, 5, 4, 3, 2, 1, 0];

lfo_vib_val_to_cent = round(128 * ((0:15) / 15));

% tremolo (max sensitivity: -48 db)
lfo_trem_shifts = [12, 2, 1, 0];

lfo_trem_val_to_db = round(4096 * ((0:15) / 15));

% print out tables
printf("LFO Phase Incs Table: \n")
for m = 1:4
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", lfo_phase_incs(8 * (m - 1) + n))
    if ((m < 4) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("LFO Delay Periods Table: \n")
for m = 1:2
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:16
    printf("%3d", lfo_delay_periods(16 * (m - 1) + n))
    if ((m < 2) || (n < 16))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

