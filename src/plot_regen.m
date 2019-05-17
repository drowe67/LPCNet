% plot_regen.m
% David Rowe April 2019
% Generate a few plots for HF regen

nb_features=55;
nb_bands=18;
n=100;
% 10x to convert log10(bandenergy) -> dB
feat_orig=10*load_f32("../build_linux/c01_01.f32",nb_features);
feat_regen=10*load_f32("../build_linux/1_f.f32",nb_features);

figure(1); clf;
mesh(feat_orig(1:n, 1:nb_bands))
ylabel('Time'); xlabel('Frequency'); zlabel('Amplitude dB');
axis([1 nb_bands 1 n 0 50]);
print fig1_timefreq.png
figure(2); clf;
difference=feat_orig-feat_regen;
mesh(difference(1:n, 1:nb_bands))
ylabel('Time'); xlabel('Frequency'); zlabel('Amplitude dB');
axis([1 nb_bands 1 n 0 50]);
print fig2_timefreq_regen_error.png
figure(3); clf;
plot(feat_orig(50, 1:nb_bands),'b;frame 50 original;');
hold on;
plot(feat_regen(50, 1:nb_bands),'g;frame 50 regen;');
hold on;
xlabel('Frequency'); ylabel('Amplitude dB');
hold off;
print fig3_fr50_orig_regen.png
