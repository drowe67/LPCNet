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

function mesh_against_time(m, st_sec, en_sec, Fs)
  st = Fs*st_sec; en = Fs*en_sec;
  t = st_sec:1/Fs:en_sec;
  mesh(m(st+1:en+1,:));  
endfunction

feat_lpcnet=load_f32("../birch.f32", nb_lpcnet_features);
dctLy = feat_lpcnet(:,1:nb_lpcnet_bands);
dctLydB = 10*dctLy;

fs=fopen("../speech_orig_16k_centre.s16","rb");
s = fread(fs,Inf,"short");
fclose(fs);

st_sec=0; en_sec=1;

figure(1); clf;
subplot(211);
plot_against_time(s, st_sec, en_sec, Fs, 'b')
subplot(212);
plot_against_time(dctLydB(:,1), st_sec, en_sec, Fsf, 'r');

figure(2); clf;
mesh_against_time(dctLydB(:,1:5), st_sec, en_sec, Fsf);

LydB = idct(dctLydB')';
figure(3); clf;
mesh_against_time(LydB, st_sec, en_sec, Fsf);

dc = mean(LydB')';
figure(4);
subplot(211);
plot_against_time(dctLydB(:,1), st_sec, en_sec, Fsf, 'r');
subplot(212);
plot_against_time(dc, st_sec, en_sec, Fsf, 'r');

printf("mean dctLydB(:,1): %f mean dc: %f\n", mean(dctLydB(:,1)), mean(dc));
