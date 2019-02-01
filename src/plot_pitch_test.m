% plot_pitch_test_ext.m
% David Rowe Feb 2019
%
% Octave script to test Codec 2 pitch track from tnlp and est integrated into dump_data

Fs = 16000;
Fsp = 100;

graphics_toolkit ("gnuplot")

fn_raw = "~/Desktop/deep/quant/birch.wav";
tnlp = load("../birch_f0_pp.txt");
tcodec2_pitch = load("../birch_f0_pp2.txt");
dump_data_pitch = load("../birch_dd_pp.txt");
lpcnet_f0 = 2*Fs./dump_data_pitch(:,3);

fs=fopen(fn_raw,"rb");
s = fread(fs,Inf,"short");
fclose(fs);

figure(1); clf;

st_sec=0; en_sec=en_sec=length(s)/Fs;
st = Fs*st_sec; en = Fs*en_sec;
t = st_sec:1/Fs:en_sec-1/Fs;

subplot(211,"position",[0.1 0.8 0.8 0.15]);
plot(t,s(st+1:en));
s_max = max(abs(s(st+1:en)));
axis([st_sec en_sec -s_max s_max])

subplot(212,"position",[0.1 0.05 0.8 0.7]);
st = Fsp*st_sec; en = Fsp*en_sec;
t = st_sec:1/Fsp:en_sec-1/Fsp;
plot(t,tnlp(st+1:en,1),'b;F0 tnlp;');
hold on;
%plot(t,tcodec2_pitch(st+1:en,1),'g;F0 tcodec2_pitch;');
plot(t,dump_data_pitch(st+1:en,1),'r;F0 dump_data_pitch;');
plot(t,lpcnet_f0,'m;F0 lpcnet_pitch;');
hold off;
axis([st_sec en_sec 0 400])

figure(2); clf;
subplot(211,"position",[0.1 0.8 0.8 0.15]);
st = Fs*st_sec; en = Fs*en_sec;
t = st_sec:1/Fs:en_sec-1/Fs;
plot(t,s(st+1:en));
axis([st_sec en_sec -s_max s_max])

subplot(212,"position",[0.1 0.05 0.8 0.7]);
st = Fsp*st_sec; en = Fsp*en_sec;
t = st_sec:1/Fsp:en_sec-1/Fsp;
plot(t,dump_data_pitch(st+1:en,2),'b;pitch codec 2;');
hold on;
plot(t,dump_data_pitch(st+1:en,3),'r;pitch lpcnet;');
hold off;
