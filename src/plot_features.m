% plot_features.m
% David Rowe Jan 2019
%
% Octave script to plot features against time

function plot_features(fn_speech, fn_feat, fn_feat2)
  nb_lpcnet_features=55;
  nb_lpcnet_bands=18;
  Fs = 16000;
  Fsf = 100;
  Fssv = 50;

  st_sec=1; en_sec=2;

  feat_lpcnet=load_f32(fn_feat, nb_lpcnet_features);
  dctLy = feat_lpcnet(:,1:nb_lpcnet_bands);
  dctLydB = 10*dctLy;
  pitch_index_lpcnet = 100*feat_lpcnet(:,2*nb_lpcnet_bands+1) + 200;
  f0 = 2*Fs ./ pitch_index_lpcnet;
  gain = feat_lpcnet(:,2*nb_lpcnet_bands+2);
  %snr = feat_lpcnet(:,2*nb_lpcnet_bands+3);
  voicing = feat_lpcnet(:,2*nb_lpcnet_bands+3);

  fs=fopen(fn_speech,"rb");
  s = fread(fs,Inf,"short");
  fclose(fs);

  figure(1); clf;
  subplot(311);
  plot_against_time(s, st_sec, en_sec, Fs, 'b;speech;')
  subplot(312);
  plot_against_time(f0, st_sec, en_sec, Fsf, 'b;f0;');
  %plot_against_time(snr, st_sec, en_sec, Fsf, 'g;snr;');
  subplot(313);
  plot_against_time(gain, st_sec, en_sec, Fsf, 'b;gain;');
  hold on;
  plot_against_time(voicing, st_sec, en_sec, Fsf, 'g;voicing;');
  %plot_against_time(snr, st_sec, en_sec, Fsf, 'g;snr;');
  %axis([st_sec en_sec 0 10])
  hold off;
  
  LydB = idct(dctLydB')';
  figure(2); clf;
  mesh_against_time(LydB, st_sec, en_sec, Fsf); title('LydB');

  figure(3); clf;
  bar(mean(LydB)); title ('mean LydB'); axis([1 18 0 25]);
  figure(4); clf;
  subplot(211); hist(gain,0:0.1:1); title ('gain'); 

  if nargin == 3
    feat_lpcnet2=load_f32(fn_feat2, nb_lpcnet_features);
    dctLy2 = feat_lpcnet2(:,1:nb_lpcnet_bands);
    dctLydB2 = 10*dctLy2;
    LydB2 = idct(dctLydB2')';
    figure(5); clf;
    bar(mean(LydB2)); title ('mean LydB2'); axis([1 18 0 25]);
    figure(4); subplot(212);
    gain2 = feat_lpcnet2(:,2*nb_lpcnet_bands+2);
    hist(gain2,0:0.1:1);
  end
endfunction

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

