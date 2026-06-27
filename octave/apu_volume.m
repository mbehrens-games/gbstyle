clear;
clc;

vol_lin_tab = round(32768 * exp(-1 * log(2) * (0:255)/256));

dco_step_att_db = round(-256 * (log((2 * (0:7) + 1)/15) / log(2)));
pcm_step_att_db = round(-256 * (log((2 * (0:127) + 1)/255) / log(2)));

%{
env_rise_att_db = round(-256 * (log((0:63)/63) / log(2)));
env_fall_att_db = 65 * (63 - (0:63));
%}

env_rise_att_db = round(4095 * exp(log(7 / 8) * (0:63)));
env_fall_att_db = 65 * (63 - (0:63));

env_rise_att_db(1) = 4095;
env_fall_att_db(1) = 4095;

% print out tables
printf("Volume DB to Linear Table: \n");
for m = 1:256
  printf("%5d, ", vol_lin_tab(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

printf("DCO Step to Attenuation DB Table: \n");
for m = 1:8
  printf("%4d, ", dco_step_att_db(m))
end
printf("\n\n");

printf("PCM Step to Attenuation DB Table: \n");
for m = 1:128
  printf("%4d, ", pcm_step_att_db(m))
   if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

printf("Envelope Rise Step to Attenuation DB Table: \n");
for m = 1:64
  printf("%4d, ", env_rise_att_db(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

printf("Envelope Fall Step to Attenuation DB Table: \n");
for m = 1:64
  printf("%4d, ", env_fall_att_db(m))
  if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

