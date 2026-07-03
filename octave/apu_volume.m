clear;
clc;

vol_lin_tab = round(32768 * exp(-1 * log(2) * (0:255)/256));

% print out tables
printf("Volume DB to Linear Table: \n")
for m = 1:32
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", vol_lin_tab(8 * (m - 1) + n))
    if ((m < 32) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")
