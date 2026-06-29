clear;
clc;

vol_lin_tab = round(32768 * exp(-1 * log(2) * (0:255)/256));

% print out tables
printf("Volume DB to Linear Table: \n");
for m = 1:256
  printf("%5d, ", vol_lin_tab(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

