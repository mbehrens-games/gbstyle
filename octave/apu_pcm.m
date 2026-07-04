clear;
clc;

% pcm phase incs
pcm_1hz_inc = exp(log(2) * 16) / 32000; % 16 bit mantissa, 32 khz clock
pcm_samp_rates = [11025, 12000, 22050, 24000];

pcm_phase_incs = round(pcm_1hz_inc * pcm_samp_rates);

% pcm level curve
pcm_val_to_db = round(-256 * (log((2 * (0:127) + 1)/255) / log(2)));

% print out tables
printf("PCM Phase Incs Table: \n")
printf("  { ")
for m = 1:4
  printf("%5d", pcm_phase_incs(m))
  if (m < 4)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("PCM Value to DB Table: \n")
for m = 1:16
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", pcm_val_to_db(8 * (m - 1) + n))
    if ((m < 16) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

