clear;
clc;

% pulse waves
wavetables( 1,:) = 15 * [ones(1,16), zeros(1,16)];
wavetables( 2,:) = 15 * [ones(1,14), zeros(1,18)];
wavetables( 3,:) = 15 * [ones(1,12), zeros(1,20)];
wavetables( 4,:) = 15 * [ones(1,10), zeros(1,22)];
wavetables( 5,:) = 15 * [ones(1, 8), zeros(1,24)];
wavetables( 6,:) = 15 * [ones(1, 6), zeros(1,26)];
wavetables( 7,:) = 15 * [ones(1, 4), zeros(1,28)];
wavetables( 8,:) = 15 * [ones(1, 2), zeros(1,30)];

wavetables( 9,:) = 15 - 15 * [ones(1,16), zeros(1,16)];
wavetables(10,:) = 15 - 15 * [ones(1, 8), zeros(1,24)];

% saw waves
wavetables(11,:) = floor(0.5 * (31 - (0:31)));
wavetables(12,:) = 15 - floor(0.5 * (31 - (0:31)));

% triangle waves
wavetables(13,:) = [8 + (0:7), 15 - (0:15), (0:7)];
wavetables(14,:) = [(0:15), 15 - (0:15)];

% ramp waves
wavetables(15,:) = [8 + floor(0.5 * (0:15)), floor(0.5 * (0:15))];
wavetables(16,:) = [7 - floor(0.5 * (0:15)), 15 - floor(0.5 * (0:15))];

% print out wavetables
printf("Preset Wavetables: \n");

for n = 1:16
  for m = 1:16
    printf("0x%X%X, ", wavetables(n, (2 * m) - 1), wavetables(n, 2 * m))
  end
   printf("\n");
end
printf("\n");

