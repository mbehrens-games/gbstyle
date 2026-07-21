clear;
clc;

% sequencer phase incs
seq_1hz_inc = exp(log(2) * 16) / 6000; % 16 bit mantissa, 6 khz clock
seq_tempo_freqs = ((32:255)/60) * 960; % bpm is 32 to 255, 960 parts per beat

seq_phase_incs = round(seq_1hz_inc * seq_tempo_freqs);

% midi tables
seq_midi_note_vel = [4095, 8 * (127 - (1:127))];

% print out tables
printf("Sequencer Phase Incs Table: \n")
for m = 1:28
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%5d", seq_phase_incs(8 * (m - 1) + n))
    if ((m < 28) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

printf("MIDI Note Velocity Table: \n")
for m = 1:16
  if (m == 1)
    printf("  { ")
  else
    printf("    ")
  endif
  for n = 1:8
    printf("%4d", seq_midi_note_vel(8 * (m - 1) + n))
    if ((m < 16) || (n < 8))
      printf(", ")
    endif
  endfor
  printf("\n")
endfor
printf("  };")
printf("\n\n")

