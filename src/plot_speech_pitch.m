% plot_speech_pitch.m
% David Rowe Jan 2019
%
% Octave script to plot speech and the pitch F0 in Hz from a .f32 feature
% file against time. Runs from the shell command line and outputs a PNG.
%
% usage1 - plot pitch est in feastures1 and features2.f32 together:
%   $ octave --no-gui -p src -qf src/plot_speech_pitch.m speech.s16 features1.f32 features2.f32 plot.png 400
%
% usage2 - ploty pitch est and voicing from features1.f32 together:
%   $ octave --no-gui -p src -qf src/plot_speech_pitch.m speech.s16 features1.f32 - plot.png 400

nb_lpcnet_features=55;
nb_lpcnet_bands=18;
Fs = 16000;   % speech sample rate
Fsp = 100;    % pitch sample rate

graphics_toolkit ("gnuplot")

% command line arguments

arg_list = argv ();
if nargin == 0
  printf("\nusage: %s rawSpeechFile featureFile [featureFile_c2 | -] PNGname f0AxisMax\n\n", program_name());
  exit(0);
end

fn_raw = arg_list{1};
fn_feat = arg_list{2};
fn_feat_c2 = arg_list{3};

feat=load_f32(fn_feat, nb_lpcnet_features);
pitch_index_lpcnet = 100*feat(:,2*nb_lpcnet_bands+1) + 200;
f0 = 2*Fs ./ pitch_index_lpcnet;
gain = feat(:,2*nb_lpcnet_bands+2);

if !strcmp(fn_feat_c2,"-")
  % usage1 - two pitch estimators
  usage = 1;
  feat_c2=load_f32(fn_feat_c2, nb_lpcnet_features);
  pitch_index_c2 = 100*feat_c2(:,2*nb_lpcnet_bands+1) + 200;
  f0_c2 = 2*Fs ./ pitch_index_c2;
  gain_c2 = feat_c2(:,2*nb_lpcnet_bands+2);
else
  usage = 2;
  % usage2 - pitch plus "gain" (voicing)
  gain = feat(:,2*nb_lpcnet_bands+2);
end

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

% adjust scale to make plot clearer
mx = str2num(arg_list{5})

if usage == 1
  plot(t,f0(st+1:en),'b;F0 Hz;');
  hold on;
  plot(t,f0_c2(st+1:en),'g;F0 Hz Codec 2;');
  hold off;
  ylabel('F0 Hz')
  axis([st_sec en_sec 50 mx])
else
  ax = plotyy(t,f0(st+1:en),t,gain(st+1:en));
  ylabel (ax(1), "F0 HZ");
  ylabel (ax(2), "GAIN");
  axis(ax(1), [st_sec en_sec 50 mx])
  axis(ax(2), [st_sec en_sec 0 1])
end

str=sprintf("-S%d,700",floor(1200*length(s)/(2*Fs)))
print(arg_list{4},'-dpng',str)

% extra figures to compare voicing ests
if usage == 1
  # fig 2 - speech at top and two voicing estimators
  figure(2);
  st = Fs*st_sec; en = Fs*en_sec;
  t = st_sec:1/Fs:en_sec-1/Fs;
  subplot(211,"position",[0.1 0.8 0.8 0.15]);
  plot(t,s(st+1:en));
  mx = max(abs(s));
  axis([st_sec en_sec -mx mx])

  subplot(212,"position",[0.1 0.05 0.8 0.7]);
  st = Fsp*st_sec; en = Fsp*en_sec;
  t = st_sec:1/Fsp:en_sec-1/Fsp;
  plot(t,gain(st+1:en),'b;pitch gain;');
  hold on;
  plot(t,gain_c2(st+1:en),'g;voicing Codec 2;');
  hold off;
  axis([st_sec en_sec 0 1])
  str=sprintf("-S%d,700",floor(1200*length(s)/(2*Fs)))
  [dir name ext] = fileparts(arg_list{4})
  if length(dir) == 0
    dir = "."
  end
  fn = sprintf("%s/%s_fig2.png",dir,name)
  print(fn,'-dpng',str)

  # fig 3 - scatter plot of two estimators
  figure(3); clf;
  %plot(gain(st+1:en),gain_c2(st+1:en),'+');
  subplot(211)
  hist(gain(st+1:en));
  subplot(212)
  hist(gain_c2(st+1:en));
 
  fn = sprintf("%s/%s_fig3.png",dir,name);
  print(fn,'-dpng')
 
end
