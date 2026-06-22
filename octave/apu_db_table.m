clear;
clc;

db_tab = round(32767 * exp(-log(2) * (0:255)/256));

% print out db to linear table
printf("DB Table: \n");
for m = 1:256
  printf("%5d, ", db_tab(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");
