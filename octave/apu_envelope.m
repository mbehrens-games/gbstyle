clear;
clc;

% envelope phase incs (13 blocks, 8 entries per block)
env_phase_shifts = [9 - (0:8), zeros(1,4)];
env_phase_steps = [ones(1,10), 2, 4, 8];

env_phase_incs = round(65536 * ((8:15) / 16));

% envelope wavetables (64 indices)
env_index_db_rise = round(4095 * exp(log(7 / 8) * (0:63)));
env_index_db_fall = 65 * (63 - (0:63));

env_index_db_rise(1) = 4095;
env_index_db_fall(1) = 4095;

% patch param envelope speed mapping (32 values)
patch_param_fall_speeds = round((0:31) * (11 / 4));
patch_param_rise_speeds = 16 + patch_param_fall_speeds;

% print out tables
printf("Envelope Phase Shifts Table: \n");
for m = 1:13
  printf("%d, ", env_phase_shifts(m))
end
printf("\n\n");

printf("Envelope Phase Steps Table: \n");
for m = 1:13
  printf("%d, ", env_phase_steps(m))
end
printf("\n\n");

printf("Envelope Phase Incs Table: \n");
for m = 1:8
  printf("%5d, ", env_phase_incs(m))
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
