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
lfo_vib_shifts = 7 - (0:7);
lfo_vib_val_to_cent = round(128 * ((0:15) / 15));

% tremolo (max sensitivity: -48 db)
lfo_trem_shifts = 3 - (0:3);
lfo_trem_val_to_db = round(4096 * ((0:15) / 15));

% midi controller position mapping (m is the patch parameter PMD/AMD)
for m = 1:8
  lfo_midi_cont_pos_to_depth(m,:) = (m - 1) + round((9 - m) * ((0:7) / 7));
endfor

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
for m = 1:4
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%3d", lfo_delay_periods(8 * (m - 1) + n))
    if ((m < 4) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Vibrato Shifts (PMS) Table: \n")
printf("  { ")
for m = 1:8
  printf("%d", lfo_vib_shifts(m))
  if (m < 8)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("Vibrato Val to Cent Table: \n")
for m = 1:2
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%3d", lfo_vib_val_to_cent(8 * (m - 1) + n))
    if ((m < 2) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Tremolo Shifts (AMS) Table: \n")
printf("  { ")
for m = 1:4
  printf("%d", lfo_trem_shifts(m))
  if (m < 4)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("Tremolo Val to DB Table: \n")
for m = 1:2
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", lfo_trem_val_to_db(8 * (m - 1) + n))
    if ((m < 2) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("LFO MIDI Controller Position to Depth Table: \n")
for m = 1:8
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%d", lfo_midi_cont_pos_to_depth(m,n))
    if ((m < 8) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

