clear;
clc;

% lfo vibrato
lfo_waves(1,:) = [8 + (0:7), 15 - (0:15), (0:7)];
lfo_waves(2,:) = 15 * [ones(1,16), zeros(1,16)];
lfo_waves(3,:) = [8 + floor(0.5 * (0:15)), floor(0.5 * (0:15))];
lfo_waves(4,:) = [7 - floor(0.5 * (0:15)), 15 - floor(0.5 * (0:15))];

% lfo tremolo
lfo_waves(5,:) = [(0:15), 15 - (0:15)];
lfo_waves(6,:) = 15 - 15 * [ones(1,16), zeros(1,16)];
lfo_waves(7,:) = floor(0.5 * (31 - (0:31)));
lfo_waves(8,:) = 15 - floor(0.5 * (31 - (0:31)));

% print out tables
printf("LFO Wavetables: \n");
for n = 1:8
  for m = 1:16
    printf("0x%X%X, ", lfo_waves(n, (2 * m) - 1), lfo_waves(n, 2 * m))
    if (mod(m, 8) == 0)
      printf("\n")
    end
  end
end
printf("\n");

