% plot_lpc.m
% David Rowe April 2020
%
% Visualise LPC spectra for 700C decoder experiments

Fs  = 8000;       % speech sample rate
Fsf = 100;        % frame sample rate
nb_features = 55;
nb_rateK = 18;    % number of rateK (log amplitude) features
nb_lpc = 10;      % number of LPCs

function plot_against_time(v, st_sec, en_sec, Fs, leg='b')
  st = Fs*st_sec; en = Fs*en_sec;
  t = st_sec:1/Fs:en_sec;
  plot(t,v(st+1:en+1),leg);
endfunction

function mesh_against_time(m, st_sec, en_sec, Fs)
  st = Fs*st_sec; en = Fs*en_sec;
  t = st_sec:1/Fs:en_sec;
  mesh(m(st+1:en+1,:));  
endfunction

function mesh_aks_against_time(aks, st_sec, en_sec, Fs)
  st = Fs*st_sec; en = Fs*en_sec;
  t = st_sec:1/Fs:en_sec;
  aks = aks(st+1:en+1,:); A = [];
  for f=1:length(aks)
    A = [A freqz(1,[1 aks(f,:)],64)];
  end
  AdB = 20*log10(abs(A));
  max(AdB(:))
  mesh(AdB);  
endfunction

# plots of speech (input), rateK vectors, LPC spectra

features=load_f32("../build_linux/all_8k.f32", nb_features);
rateK=features(:, 1:nb_rateK);
aks = features(:, nb_rateK+1:nb_rateK+nb_lpc);
fs=fopen("../build_linux/all_8k_10ms.sw","rb");
s = fread(fs,Inf,"short");
fclose(fs);

st_sec=14; en_sec=16;

figure(1); clf; plot_against_time(s, st_sec, en_sec, Fs, 'b')
figure(2); clf; mesh_against_time(rateK, st_sec, en_sec, Fsf);
figure(3); clf; mesh_aks_against_time(aks, st_sec, en_sec, Fsf);

