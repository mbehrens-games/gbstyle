clear;
clc;

% lfo phase incs
lfo_1hz_inc = exp(log(2) * 21) / 375; % 21 bit phase, 375 hz clock
lfo_freqs = 0.5 + ((8 * (0:31)) / 32);

lfo_phase_incs = round(lfo_1hz_inc * lfo_freqs);

% lfo delay amounts
lfo_delay_sec = (0.5 * (0:31)) / 32;
lfo_delay_periods = round(375 * lfo_delay_sec); % 375 hz clock

% lfo sensitivities
lfo_vib_shifts = [5, 4, 2, 1];
lfo_trem_shifts = [2, 0];

% lfo step sizes
for m = 1:8
  dep = round(((m - 1) * (256 / 15)) / 7);
  rem = round((256 / 15) - dep);
  lfo_step_sizes(m,:) = round(dep + (rem * (0:7)) / 7);
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
for m = 1:4
  printf("%d", lfo_vib_shifts(m))
  if (m < 4)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("Tremolo Shifts (AMS) Table: \n")
printf("  { ")
for m = 1:2
  printf("%d", lfo_trem_shifts(m))
  if (m < 2)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("LFO Step Sizes Table: \n")
for m = 1:8
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%2d", lfo_step_sizes(m,n))
    if ((m < 8) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

