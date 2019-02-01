% plot_speech_pitch.m
% David Rowe Jan 2019
%
% Octave script to speech and the pitch F0 in Hz from a .f32 feature
% file against time. Runs from the command line and outputs a PNG.

nb_lpcnet_features=55;
nb_lpcnet_bands=18;
Fs = 16000;   % speech sample rate
Fsp = 100;    % pitch sample rate

graphics_toolkit ("gnuplot")

% command line arguments

arg_list = argv ();
if nargin == 0
  printf("\nusage: %s rawSpeechFile featureFile featureFile_c2 PNGname f0AxisMax\n\n", program_name());
  exit(0);
end

fn_raw = arg_list{1};
fn_feat = arg_list{2};
fn_feat_c2 = arg_list{3};

feat=load_f32(fn_feat, nb_lpcnet_features);
pitch_index_lpcnet = 100*feat(:,2*nb_lpcnet_bands+1) + 200;
f0 = 2*Fs ./ pitch_index_lpcnet;

feat_c2=load_f32(fn_feat_c2, nb_lpcnet_features);
pitch_index_c2 = 100*feat_c2(:,2*nb_lpcnet_bands+1) + 200;
f0_c2 = 2*Fs ./ pitch_index_c2;

fs=fopen(fn_raw,"rb");
s = fread(fs,Inf,"short");
fclose(fs);

st_sec=0; en_sec=length(s)/Fs;

figure(1); clf;
st = Fs*st_sec; en = Fs*en_sec;
t = st_sec:1/Fs:en_sec-1/Fs;
subplot(211,"position",[0.1 0.8 0.8 0.15]);
plot(t,s(st+1:en));
mx = max(abs(s));
axis([st_sec en_sec -mx mx])

subplot(212,"position",[0.1 0.05 0.8 0.7]);
st = Fsp*st_sec; en = Fsp*en_sec;
t = st_sec:1/Fsp:en_sec-1/Fsp;
plot(t,f0(st+1:en),'b;F0 Hz;');
hold on;
plot(t,f0_c2(st+1:en),'g;F0 Hz CODEC 2;');
hold off;
ylabel('F0 Hz')

% adjust scale to make plot clearer
mx = str2num(arg_list{5})
axis([st_sec en_sec 50 mx])

str=sprintf("-S%d,700",floor(1200*length(s)/(2*Fs)))
print(arg_list{4},'-dpng',str)

