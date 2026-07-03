clear;
clc;

% dco phase incs (9 blocks, 12 * 64 entries per block)
dco_phase_shifts = [6 - (0:5), zeros(1,3)];
dco_phase_steps = [ones(1,7), 2, 4];

dco_c6 = 4 * 440 * exp(log(2) * (-9/12));
dco_1hz_inc = exp(log(2) * 21) / 96000; % 21 bit phase, 96 khz clock

dco_phase_incs = round(dco_c6 * dco_1hz_inc * exp(log(2) * (0:767)/768));

% dco level curves
val = (2 * (0:7) + 1)/15;
dco_val_to_db_tri = round(-256 * (log(val) / log(2)));
dco_val_to_db_sin = round(-256 * (log(sin((pi / 2) * val)) / log(2)));
dco_val_to_db_exp = round(-256 * (log(1 - cos((pi / 2) * val)) / log(2)));

% print out tables
printf("DCO Phase Shifts Table: \n")
printf("  { ")
for m = 1:9
  printf("%d", dco_phase_shifts(m))
  if (m < 9)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("DCO Phase Steps Table: \n")
printf("  { ")
for m = 1:9
  printf("%d", dco_phase_steps(m))
  if (m < 9)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("DCO Phase Incs Table: \n")
for m = 1:96
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", dco_phase_incs(8 * (m - 1) + n))
    if ((m < 48) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("DCO Value to DB Triangle Table: \n")
printf("  { ")
for m = 1:8
  printf("%d", dco_val_to_db_tri(m))
  if (m < 8)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("DCO Value to DB Sine Table: \n")
printf("  { ")
for m = 1:8
  printf("%d", dco_val_to_db_sin(m))
  if (m < 8)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("DCO Value to DB Exponential Table: \n")
printf("  { ")
for m = 1:8
  printf("%d", dco_val_to_db_exp(m))
  if (m < 8)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

