clear;
clc;

fs = 96000;
hp_fc = 27.5 / (fs / 2);
lp_fc =  9000 / (fs / 2);

[hp_b, hp_a] = butter(1, hp_fc, "high");
hp_b = round(32768 * hp_b);
hp_a = round(32768 * hp_a);

lp_b = fir1(64, lp_fc);
lp_b = round(32768 * lp_b);

%{
[h, w] = freqz(hp_b, hp_a, 2048);
plot(w/pi, abs(h))
%}

% print out highpass coefficents
printf("Highpass Coefficents: \n");
printf("#define APU_HP_MULT_A0 %d\n", hp_a(1))
printf("#define APU_HP_MULT_A1 %d\n", hp_a(2))
printf("#define APU_HP_MULT_B0 %d\n", hp_b(1))
printf("#define APU_HP_MULT_B1 %d\n", hp_b(2))
printf("\n")

% print out lowpass kernel
printf("Lowpass Filter Table: \n");
for m = 1:4
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", lp_b(8 * (m - 1) + n))
    if ((m < 48) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("    %5d\n", lp_b(33))
printf("  };")
printf("\n\n")

