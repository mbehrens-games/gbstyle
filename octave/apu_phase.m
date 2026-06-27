clear;
clc;

fs = 96000;
bits = 20;

% dco phase incs: 12 semitones * 64 steps each = 768 steps (c8 to c9)
dco_c8 = 16 * 440 * exp(log(2) * (-9/12));
dco_1hz_inc = exp(log(2) * bits) / fs;

dco_phase_incs = round(dco_c8 * dco_1hz_inc * exp(log(2) * (0:767)/768));

% envelope phase incs: 12 semitones * 16 steps each = 192 steps (12 ms to 6 ms)
env_div = 6;
env_1hz_inc = exp(log(2) * bits) / (fs / env_div);
env_base_f = 63 * (1 / 0.012); % 63 envelope levels in 12 ms

env_phase_incs = round(env_base_f * env_1hz_inc * exp(log(2) * (0:191)/192));

% envelope rate patch param mapping (32 values)
patch_param_fall_rates = round((192 * (0:31)) / 3);
patch_param_rise_rates = 2 * 192 + patch_param_fall_rates;

% print out tables
printf("DCO Phase Increments: \n");
for m = 1:768
  printf("%6d, ", dco_phase_incs(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

printf("Envelope Phase Increments: \n");
for m = 1:192
  printf("%6d, ", env_phase_incs(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

printf("Patch Param Envelope Rise Rates: \n");
for m = 1:32
  printf("%4d, ", patch_param_rise_rates(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

printf("Patch Param Envelope Fall Rates: \n");
for m = 1:32
  printf("%4d, ", patch_param_fall_rates(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");
