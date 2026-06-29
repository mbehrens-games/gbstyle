clear;
clc;

pcm_index_db = round(-256 * (log((2 * (0:127) + 1)/255) / log(2)));

% print out tables
printf("PCM Index DB Table: \n");
for m = 1:128
  printf("%4d, ", pcm_index_db(m))
   if (mod(m, 8) == 0)
    printf("\n")
  end
end
printf("\n");

