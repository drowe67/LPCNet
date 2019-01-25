% plot_wo_test_ext.m
% David Rowe Jan 2019
%
% Octave script to plot external and internal pitch estimator

nb_lpcnet_features=55;
nb_lpcnet_bands=18;
Fs = 16000;
Fsp = 100;

feat_lpcnet=load_f32("../speech_orig_16k_features.f32", nb_lpcnet_features);
feat_lpcnet_ext=load_f32("../speech_orig_16k_extpitch.f32", nb_lpcnet_features);

pitch_index_lpcnet = 100*feat_lpcnet(:,2*nb_lpcnet_bands+1) + 200;
f0_lpcnet = 2*Fs ./ pitch_index_lpcnet;
pitch_index_lpcnet_ext = 100*feat_lpcnet_ext(:,2*nb_lpcnet_bands+1) + 200;
f0_lpcnet_ext = 2*Fs ./ pitch_index_lpcnet_ext;

fs=fopen("../speech_orig_16k_centre.s16","rb");
s = fread(fs,Inf,"short");
fclose(fs);

figure(1); clf;

st_sec=8; en_sec=9;
st = Fs*st_sec; en = Fs*en_sec;
t = st_sec:1/Fs:en_sec;

subplot(211);
plot(t,s(st+1:en+1));
s_max = max(abs(s(st+1:en+1)));
axis([st_sec en_sec -s_max s_max])
subplot(212);
st = Fsp*st_sec; en = Fsp*en_sec;
t = st_sec:1/Fsp:en_sec;
plot(t,f0_lpcnet(st+1:en+1),'b;F0 lpcnet;');
hold on;
plot(t,f0_lpcnet_ext(st+1:en+1),'g;F0 ext est;');
hold off;
axis([st_sec en_sec 0 400])
