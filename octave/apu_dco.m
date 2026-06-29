clear;
clc;

fs = 96000;
bits = 21;

% dco phase incs: 12 semitones * 32 steps each = 384 steps (c6 to c7)
dco_c6 = 4 * 440 * exp(log(2) * (-9/12));
dco_1hz_inc = exp(log(2) * bits) / fs;

dco_phase_incs = round(dco_c6 * dco_1hz_inc * exp(log(2) * (0:383)/384));

dco_index_db = round(-256 * (log((2 * (0:7) + 1)/15) / log(2)));

% print out tables
printf("DCO Phase Increments: \n");
for m = 1:384
  printf("%6d, ", dco_phase_incs(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

printf("DCO Index DB Table: \n");
for m = 1:8
  printf("%4d, ", dco_index_db(m))
end
printf("\n\n");
