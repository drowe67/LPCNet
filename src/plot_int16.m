% plot_int16.m
% David Rowe April 2019
%
% Octave script to plot the excitation and predicted speech generated during training
%
% Example to generate data
%   $ ./dump_data --train -z 0 -n 300 -c -i -s birch.int16 --mask 111111111111000000 ~/Downloads/birch.raw birch.f32 birch.pcm

function plot_int16(fn)
  nb_lpcnet_features=55;
  nb_lpcnet_bands=18;
  Fs = 16000;

  graphics_toolkit ("gnuplot")

  f=fopen(fn,"rb");
  s = fread(f,Inf,"short");
  fclose(f);

  % separate interleaved samples

  signal = s(1:4:end);
  pred   = s(2:4:end);
  ex_in  = s(3:4:end);
  ex_out = s(4:4:end);

  figure(1); clf;

  st_sec=0; en_sec=length(signal)/Fs;
  st = Fs*st_sec; en = Fs*en_sec;
  t = st_sec:1/Fs:en_sec-1/Fs;
  plot(t, signal(st+1:en), "b;signal;");
  %mx = max(abs(s))
  mx = 1E4;
  axis([st_sec en_sec -mx mx])
  hold on;
  plot(t,pred(st+1:en,1), "g;pred;");
  plot(t,ex_in(st+1:en,1), "r;ex;");
  hold off;
end

