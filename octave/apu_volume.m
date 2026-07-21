clear;
clc;

vol_db_to_lin = round(32768 * exp(-1 * log(2) * (0:255)/256));

inst_vol = [4095, 24 * (64 - (1:64))];
inst_pan = [4095, round(-256 * (log(sin((2 * pi * (1:64))/256)) / log(2)))];

% print out tables
printf("Volume DB to Linear Table: \n")
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", vol_db_to_lin(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Instrument Volume Table: \n")
printf("  { %4d, \n", inst_vol(1))
for m = 1:8
  printf("    ")
  for n = 1:8
    printf("%4d", inst_vol(8 * (m - 1) + n + 1))
    if ((m < 8) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("Instrument Panning Table: \n")
printf("  { %4d, \n", inst_pan(1))
for m = 1:8
  printf("    ")
  for n = 1:8
    printf("%4d", inst_pan(8 * (m - 1) + n + 1))
    if ((m < 8) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

