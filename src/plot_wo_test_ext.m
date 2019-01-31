% plot_wo_test_ext.m
% David Rowe Jan 2019
%
% Octave script to plot external and internal pitch estimator
% Runs from the command line and outputs a PNG.

nb_lpcnet_features=55;
nb_lpcnet_bands=18;
Fs = 16000;
Fsp = 100;

graphics_toolkit ("gnuplot")

% command line arguments

arg_list = argv ();
if nargin == 0
  printf("\nusage: %s rawSpeechFile featurefile1 featurefile2 PNGname\n\n", program_name());
  exit(0);
end

fn_raw = arg_list{1};
fn_feat = arg_list{2};
fn_feat_ext = arg_list{3};

feat_lpcnet=load_f32(fn_feat, nb_lpcnet_features);
feat_lpcnet_ext=load_f32(fn_feat_ext, nb_lpcnet_features);

pitch_index_lpcnet = 100*feat_lpcnet(:,2*nb_lpcnet_bands+1) + 200;
f0_lpcnet = 2*Fs ./ pitch_index_lpcnet;
pitch_index_lpcnet_ext = 100*feat_lpcnet_ext(:,2*nb_lpcnet_bands+1) + 200;
f0_lpcnet_ext = 2*Fs ./ pitch_index_lpcnet_ext;

% acccount for (we assume d==1) frame delay thru quant_feat
f0_lpcnet_ext = [f0_lpcnet_ext(2:end)' 0]';

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
plot(t,f0_lpcnet(st+1:en),'b;F0 lpcnet;');
hold on;
plot(t,f0_lpcnet_ext(st+1:en),'g;F0 ext est;');
hold off;
axis([st_sec en_sec 0 400])

str=sprintf("-S%d,700",floor(1200*length(s)/(2*Fs)))
print(arg_list{4},'-dpng',str)
