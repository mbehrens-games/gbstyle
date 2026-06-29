clear;
clc;

% note: envelope clock is 96000 hz / 64 = 1500 hz

% envelope period table (13 rows, 8 columns)
env_period_lengths = [round(exp(log(2) * (9 - (0:9)))), ones(1,3)];
env_period_steps = [ones(1,10), 2, 4, 8];

env_period_gates = [0x5555, \ % 0101 0101 0101 0101
                    0x5575, \ % 0101 0101 0111 0101
                    0x5757, \ % 0101 0111 0101 0111
                    0x5777, \ % 0101 0111 0111 0111
                    0x7777, \ % 0111 0111 0111 0111
                    0x77F7, \ % 0111 0111 1111 0111
                    0x7F7F, \ % 0111 1111 0111 1111
                    0x7FFF];  % 0111 1111 1111 1111

% envelope wavetables (64 steps)
env_index_db_rise = round(4095 * exp(log(7 / 8) * (0:63)));
env_index_db_fall = 65 * (63 - (0:63));

env_index_db_rise(1) = 4095;
env_index_db_fall(1) = 4095;

% patch param envelope speed mapping (32 values)
patch_param_fall_speeds = round((0:31) * (11 / 4));
patch_param_rise_speeds = 16 + patch_param_fall_speeds;

% print out tables
printf("Envelope Period Length Table: \n");
for m = 1:13
  printf("%d, ", env_period_lengths(m))
end
printf("\n\n");

printf("Envelope Period Step Table: \n");
for m = 1:13
  printf("%d, ", env_period_steps(m))
end
printf("\n\n");

printf("Envelope Period Gate Table: \n");
for m = 1:8
  printf("0x%4X, ", env_period_gates(m))
end
printf("\n\n");

printf("Envelope Index Rise DB Table: \n");
for m = 1:64
  printf("%4d, ", env_index_db_rise(m))
   if (mod(m, 8) == 0)
     printf("\n");
   end
end
printf("\n");

printf("Envelope Index Fall DB Table: \n");
for m = 1:64
  printf("%4d, ", env_index_db_fall(m))
   if (mod(m, 8) == 0)
     printf("\n");
   end
end
printf("\n");

printf("Patch Param Envelope Rise Speeds: \n");
for m = 1:32
  printf("%3d, ", patch_param_rise_speeds(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

printf("Patch Param Envelope Fall Speeds: \n");
for m = 1:32
  printf("%3d, ", patch_param_fall_speeds(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");
