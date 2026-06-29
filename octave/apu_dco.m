clear;
clc;

fs = 96000;
bits = 21;

% dco phase incs: (9 blocks, 12 * 32 entries per block)
dco_phase_shifts = [6 - (0:5), zeros(1,3)];
dco_phase_steps = [ones(1,7), 2, 4];

dco_c6 = 4 * 440 * exp(log(2) * (-9/12));
dco_1hz_inc = exp(log(2) * bits) / fs;

dco_phase_incs = round(dco_c6 * dco_1hz_inc * exp(log(2) * (0:383)/384));

dco_index_db = round(-256 * (log((2 * (0:7) + 1)/15) / log(2)));

% print out tables
printf("DCO Phase Shifts Table: \n");
for m = 1:9
  printf("%d, ", dco_phase_shifts(m))
end
printf("\n\n");

printf("DCO Phase Steps Table: \n");
for m = 1:9
  printf("%d, ", dco_phase_steps(m))
end
printf("\n\n");

printf("DCO Phase Incs Table: \n");
for m = 1:384
  printf("%5d, ", dco_phase_incs(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

printf("DCO Index DB Table: \n");
for m = 1:8
  printf("%d, ", dco_index_db(m))
end
printf("\n\n");
