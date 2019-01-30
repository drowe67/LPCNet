% plot_speech_quant.m
% David Rowe Jan 2019
%
% Octave script to speech and the quantiser performance
% against time. Runs from the command line and outputs a PNG.

nb_lpcnet_features=55;
nb_lpcnet_bands=18;
Fs = 16000;   % speech sample rate

graphics_toolkit ("gnuplot")

% command line arguments

arg_list = argv ();
if nargin == 0
  printf("\nusage: %s rawSpeechFile svFile PNGname svSamplePeriodms\n\n", program_name());
  exit(0);
end

fn_raw = arg_list{1};
fn_sv = arg_list{2};

sv=load(fn_sv);
Tsv = str2num(arg_list{4});
Fs_sv = 1/(Tsv/1000);

fs=fopen(fn_raw,"rb");
s = fread(fs,Inf,"short");
fclose(fs);

st_sec=0; en_sec=length(s)/Fs;

figure(1); clf;

st = Fs*st_sec; en = Fs*en_sec;
t = st_sec:1/Fs:en_sec-1/Fs;

subplot(311,"position",[0.1 0.8 0.8 0.15]);
plot(t,s(st+1:en));
mx = max(abs(s));
axis([st_sec en_sec -mx mx])

st = Fs_sv*st_sec; en = Fs_sv*en_sec;
t = st_sec:1/Fs_sv:en_sec-1/Fs_sv;

subplot(312,"position",[0.1 0.62 0.8 0.15]);
plot(t,sv(st+1:en,1));
ylabel('ENERGY dB');
amin = 10*floor(min(sv(st+1:en,1))/10)
amax = 10*ceil(max(sv(st+1:en,1))/10)
axis([st_sec en_sec amin amax])

subplot(313,"position",[0.1 0.05 0.8 0.55]);
ax = plotyy(t,sv(st+1:en,2),t,sv(st+1:en,end));
ylabel (ax(1), "PRED RMS ERROR");
ylabel (ax(2), "FINAL VQ STAGE RMS ERROR");

str=sprintf("-S%d,700",floor(1200*length(s)/(2*Fs)))
print(arg_list{3},'-dpng',str)

