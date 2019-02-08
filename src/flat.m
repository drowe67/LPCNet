% flat.m
% David Rowe Feb 2019
%
% Generates a spectally flat synthestic signal for testing LPCNet
% amplitude estimation

Fs=16000;   % sample rate
F0=100;     % pitch in Hz
L=(Fs/2)/F0 % number of harmonics
A=10;       % amplitude of each harmonic
N=Fs;

s = zeros(1,N);
for m=1:L
  s += A*cos(2*pi*m*F0*(1:N)/Fs);     
end

% simulate a couple of formants

w1=pi/8; beta1=0.99;
a1 = [1 -2*beta1*cos(w1) beta1*beta1];
s = filter(1,a1,s);
w2=pi/3; beta2=0.99;
a2 = [1 -2*beta2*cos(w2) beta2*beta2];
s = filter(0.5,a2,s);

figure(1); clf;
plot(s(1:800));

f=fopen("flat.s16", "wb");
fwrite(f, s, 'int16');
fclose(f);

