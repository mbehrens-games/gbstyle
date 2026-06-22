clear;
clc;

wave_steps = round(-256 * (log((2 * (0:7) + 1)/15) / log(2)));
pcm_steps  = round(-256 * (log((2 * (0:127) + 1)/255) / log(2)));

env_steps_lin = round(-256 * (log((0:15)/15) / log(2)));
env_steps_exp = 64 * (15 - (0:15));

env_steps_lin(1) = 4095;
env_steps_exp(1) = 4095;

% print out wave steps
printf("Wave Steps: \n");
for m = 1:8
  printf("%4d, ", wave_steps(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

% print out pcm steps
printf("PCM Steps: \n");
for m = 1:128
  printf("%4d, ", pcm_steps(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

% print out envelope steps
printf("Envelope Steps (Linear): \n");
for m = 1:16
  printf("%4d, ", env_steps_lin(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

printf("Envelope Steps (Exponential): \n");
for m = 1:16
  printf("%4d, ", env_steps_exp(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");
