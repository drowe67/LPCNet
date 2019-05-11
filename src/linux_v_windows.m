% linux_v_windows.m
% David Rowe May 2019
%
% Part of system to generate and compare Linux and Windows test files
% that contain LPCNet states.  Use to track down issue with Windows version
%
% Run in Octave from LPCNet/src

1;

function check_vec(fig_num, name, vec1, vec2)
  figure(fig_num); clf;
  plot(vec1); hold on; plot(vec2); hold off;
  title(name);
  diff = var(vec1-vec2);
  if diff < 1E-3
    printf("%s [PASS]\n", name);
  else
    printf("%s [FAIL]\n", name);
  end
endfunction

function check_matrix(fig_num, name, mat1, mat2)
  figure(fig_num); clf;
  mesh(mat1); hold on; mesh(mat2); hold off;
  title(name);
  mdiff = mat1-mat2; diff = var(mdiff(:));
  if diff < 1E-3
    printf("%s [PASS]\n", name);
  else
    printf("%s [FAIL]\n", name);
    % find first row where problem occurs
    [rows cols]= size(mat1);
    first_error = 1;
    for r=1:rows
      e = var(mat1(r,:) - mat2(r,:));
      if e > 1E-3
        printf("error %f in row: %d\n", e, r);
        if (first_error)
          clf; plot(mat1(r,:)); hold on; plot(mat2(r,:)); plot(mat1(r,:) - mat2(r,:)); hold off;
          first_error = 0;
        end
      end
    end
  end
endfunction

n_pitch_embed = 64;
n_pitch = 1;
n_pitch_gain = 1;
n_lpc = 16;
n_condition = 128;
n_cols = n_pitch_embed + n_pitch + n_pitch_gain + n_lpc + n_condition;

linux=load_f32("../build_linux/src/test_lpcnet_states.f32", n_cols);
[r c]=size(linux);
printf("linux %d x %d\n", r, c);
windows=load_f32("../build_win/src/test_lpcnet_states.f32", n_cols);
[r c]=size(windows);
printf("windows %d x %d\n", r, c);

fig = 1;
st = 1;
en = st + n_pitch_embed-1; check_matrix(fig++, "pitch_embed", linux(:,st:en), windows(:,st:en)); st += n_pitch_embed;
check_vec(fig++, "pitch", linux(:,st), windows(:,st)); st += n_pitch;
check_vec(fig++, "pitch_gain", linux(:,st), windows(:,st)); st += n_pitch_gain;
en = st + n_lpc-1; check_matrix(fig++, "lpc", linux(:,st:en), windows(:,st:en)); st += n_lpc;
en = st + n_condition-1; check_matrix(fig++, "condition", linux(:,st:en), windows(:,st:en)); st += n_condition;


