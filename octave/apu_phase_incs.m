clear;
clc;

fs = 96000;
bits = 20;

% pitch table: 12 semitones * 64 steps per semitone = 768 steps (c8 to c9)
wave_c8 = 16 * 440 * exp(log(2) * (-9/12));
wave_1hz_inc = exp(log(2) * bits) / fs;

wave_phase_incs = round(wave_c8 * wave_1hz_inc * exp(log(2) * (0:767)/768));

% envelope time table: 12 semitones (12 ms to 6 ms rise time), 15 levels
env_div = 6;
env_1hz_inc = exp(log(2) * bits) / (fs / env_div);

for n = 1:15
  env_phase_incs(n, :) = round((n / 0.012) * env_1hz_inc * exp(log(2) * (0:11)/12));
end

% print out wave phase incs
printf("Wave Phase Increments: \n");
for m = 1:768
  printf("%6d, ", wave_phase_incs(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

% print out envelope phase incs
printf("Envelope Phase Increment Matrix: \n");
for n = 1:15
  for m = 1:12
    printf("%6d, ", env_phase_incs(n, m))
  end
  printf("\n");
end
printf("\n");
