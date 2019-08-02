% linux_v_windows.m
% David Rowe May 2019
%
% Compare the LPCNet states from two runs.  Use to track down issues
% with Windows port and run time loading on NNs.
%

% returns 0 for pass

function f = compare_states(fn_one, fn_two, fig_en=0)
  n_pitch_embed = 64;
  n_pitch = 1;
  n_pitch_gain = 1; 
  n_lpc = 16;
  n_condition = 128;
  n_gru_a = 1152;
  n_last_sig = 160;
  n_pred = 160;
  n_exc = 160;
  n_pcm = 160;
  n_cols = n_pitch_embed + n_pitch + n_pitch_gain + n_lpc + n_condition + n_gru_a + n_last_sig + n_pred + n_exc + n_pcm;

  linux=load_f32(fn_one, n_cols);
  [r c]=size(linux); 
  printf("linux %d x %d\n", r, c);
  windows=load_f32(fn_two, n_cols);
  [r c]=size(windows);
  printf("windows %d x %d\n", r, c);

  fig = 1; st = 1; f = 0;
  en = st + n_pitch_embed-1; f+= check_matrix(fig++, "pitch_embed", linux(:,st:en), windows(:,st:en));        st += n_pitch_embed;
                             f+= check_vec(fig++, "pitch", linux(:,st), windows(:,st), fig_en);               st += n_pitch;
                             f+= check_vec(fig++, "pitch_gain", linux(:,st), windows(:,st), fig_en);          st += n_pitch_gain;
  en = st + n_lpc-1;         f+= check_matrix(fig++, "lpc", linux(:,st:en), windows(:,st:en), fig_en);        st += n_lpc;
  en = st + n_condition-1;   f+= check_matrix(fig++, "condition", linux(:,st:en), windows(:,st:en), fig_en);  st += n_condition;
  en = st + n_gru_a-1;       f+= check_matrix(fig++, "gru_a_condition", linux(:,st:en), windows(:,st:en), fig_en); st += n_gru_a;

  en = st + n_last_sig-1;    f+= check_vec(fig++, "last_sig", linux(:,st:en), windows(:,st:en), fig_en);      st += n_last_sig;
  en = st + n_pred-1;        f+= check_vec(fig++, "pred", linux(:,st:en), windows(:,st:en), fig_en);          st += n_pred;
  en = st + n_exc-1;         f+= check_vec(fig++, "exc", linux(:,st:en), windows(:,st:en), fig_en);           st += n_exc;
  en = st + n_pcm-1;         f+= check_vec(fig++, "pcm", linux(:,st:en), windows(:,st:en), fig_en);           st += n_pcm;
 
  printf("fails: %d\n", f);
endfunction


function f = check_vec(fig_num, name, vec1, vec2, fig_en=0)
  # vector may be supplied as matrix that we need to reshape, e.g. for excitation signal
  [rows cols] = size(vec1);
  if cols != 1
    vec1 = reshape(vec1', 1, rows*cols);
    vec2= reshape(vec2', 1, rows*cols);
  end
  if fig_en
    figure(fig_num); clf;
    plot(vec1); hold on; plot(vec2); hold off;
    title(name);
  end
  diff = max(abs(vec1-vec2));
  if diff < 1E-3
    f = 0; printf("%-15s [PASS]\n", name);
  else
    f = 1; first_error = 1; diffv = abs(vec1-vec2);
    for r=1:length(vec1)
      if first_error && (diffv(r) > 1E-3)
        first_error = 0;
        printf("%-15s [FAIL] at sample %d vec1: %f vec2: %f %f\n", name, r, vec1(r), vec2(r), vec1(r)-vec2(r));
      end
    end
  end
endfunction


function f = check_matrix(fig_num, name, mat1, mat2, fig_en=0)
  if fig_en; figure(fig_num); clf; end
  [r c] = size(mat1);
  plotr=min(r,100);
  plotc=min(c,100);
  if fig_en
    mesh(mat1(1:plotr,1:plotc)); hold on; mesh(mat2(1:plotr,1:plotc)); hold off;
    title(name);
  end
  
  mdiff = mat1-mat2; diff = max(abs(mdiff(:)));
  if diff < 1E-3
    f = 0;
    printf("%-15s [PASS]\n", name);
  else
    f = 1;
    printf("%-15s [FAIL]\n", name);
    % find first row where problem occurs
    [rows cols]= size(mat1);
    first_error = 1; nerr = 0;
    for r=1:rows
      e = mat1(r,:) - mat2(r,:);
      if max(e) > 1E-3 && nerr < 3
        nerr++;
        [mx col] = max(e);
        printf("max error %f in row: %d col: %d\n", max(e), r, col);
        if (first_error)
          clf; plot(mat1(r,:),'g'); hold on; plot(mat2(r,:),'b'); plot(mat1(r,:) - mat2(r,:),'r'); hold off;
          first_error = 0;
          mat1(r,1:10)
          mat2(r,1:10)
        end
      end
    end
  end
endfunction


