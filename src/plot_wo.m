% plot_wo.m
% David Rowe Jan 2019
%
% Octave script to plot pitch countours from different estimators

nb_lpcnet_features=55;
nb_lpcnet_bands=18;
nb_codec2_features=16+3+16;
Fs = 16000;
Fsp = 100;
Fspsilk = 50;

feat_lpcnet=load_f32("../speech_orig_16k_features.f32", nb_lpcnet_features);
feat_codec2=load_f32("../speech_orig_16k_codec2_features.f32", nb_codec2_features);

pitch_index_lpcnet = 100*feat_lpcnet(:,2*nb_lpcnet_bands+1) + 200;
f0_lpcnet = 2*Fs ./ pitch_index_lpcnet;
f0_codec2 = Fs*feat_codec2(:,16+1+1)/(2*pi);

pitch_silk=load("../../silk_pitch.txt");
f0_silk=Fs./pitch_silk;
fs=fopen("../speech_orig_16k_centre.s16","rb");
s = fread(fs,Inf,"short");
fclose(fs);

figure(1); clf;
st_sec=8; en_sec=10.5;
st = Fs*st_sec; en = Fs*en_sec;
t = st_sec:1/Fs:en_sec;
subplot(211); plot(t,s(st+1:en+1));
subplot(212);
st = Fsp*st_sec; en = Fsp*en_sec;
t = st_sec:1/Fsp:en_sec;
plot(t,f0_lpcnet(st+1:en+1),'b;lpcnet;');
hold on;
t = st_sec:1/Fsp:en_sec;
plot(t, f0_codec2(st+1:en+1),'r;codec2;');
st = Fspsilk*st_sec; en = Fspsilk*en_sec;
t = st_sec:1/Fspsilk:en_sec;
plot(t,f0_silk(st+1:en+1),'g;silk;');
axis([st_sec en_sec 0 400])
hold off;
