% plot_features.m
% David Rowe Jan 2019
%
% Octave script to plot features against time

nb_lpcnet_features=55;
nb_lpcnet_bands=18;
Fs = 16000;
Fsf = 100;
Fssv = 50;

function plot_against_time(v, st_sec, en_sec, Fs, leg)
  st = Fs*st_sec; en = Fs*en_sec;
  t = st_sec:1/Fs:en_sec;
  plot(t,v(st+1:en+1),leg);
endfunction

feat_lpcnet=load_f32("../speech_orig_16k_features.f32", nb_lpcnet_features);
dctLy = feat_lpcnet(:,1:nb_lpcnet_bands);

fs=fopen("../speech_orig_16k_centre.s16","rb");
s = fread(fs,Inf,"short");
fclose(fs);

sv = load("../speech_orig_sv.txt");

st_sec=2.5; en_sec=5.5;

figure(1); clf;
subplot(211);
plot_against_time(s, st_sec, en_sec, Fs, 'b')
subplot(212);
plot_against_time(sqrt(sv(:,4)), st_sec, en_sec, Fssv, 'r');
%hold on;
%plot_against_time(sqrt(sv(:,5)),st_sec, en_sec, Fssv,'g');
%hold off;

