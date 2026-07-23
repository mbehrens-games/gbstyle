clear;
clc;

% envelope phase incs (13 blocks, 8 entries per block)
env_phase_shifts = [9 - (0:8), zeros(1,4)];
env_phase_steps = [ones(1,10), 2, 4, 8];

env_phase_incs = round(65536 * ((8:15) / 16));

% envelope wavetables (64 indices)
env_rise_val_to_db = round(4095 * exp(log(7 / 8) * (0:63)));
env_fall_val_to_db = 65 * (63 - (0:63));

env_rise_val_to_db(1) = 4095;
env_fall_val_to_db(1) = 4095;

% patch param envelope speed mapping (32 values)
env_fall_speeds = round((0:31) * (11 / 4));
env_rise_speeds = 16 + env_fall_speeds;

% print out tables
printf("Envelope Phase Shifts Table: \n")
printf("  { ")
for m = 1:13
  printf("%d", env_phase_shifts(m))
  if (m < 13)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("Envelope Phase Steps Table: \n")
printf("  { ")
for m = 1:13
  printf("%d", env_phase_steps(m))
  if (m < 13)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("Envelope Phase Incs Table: \n");
printf("  { ")
for m = 1:8
  printf("%5d", env_phase_incs(m))
  if (m < 8)
    printf(", ")
  endif
endfor
printf(" };")
printf("\n\n")

printf("Envelope Rise Value to DB Table: \n");
for m = 1:8
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", env_rise_val_to_db(8 * (m - 1) + n))
    if ((m < 8) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Envelope Fall Value to DB Table: \n");
for m = 1:8
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", env_fall_val_to_db(8 * (m - 1) + n))
    if ((m < 8) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Envelope Rise Speeds: \n");
for m = 1:4
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%3d", env_rise_speeds(8 * (m - 1) + n))
    if ((m < 4) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Envelope Fall Speeds: \n");
for m = 1:4
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%3d", env_fall_speeds(8 * (m - 1) + n))
    if ((m < 4) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

