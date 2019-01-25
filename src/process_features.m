% process_features.m
% David Rowe Jan 2019
%
% Take a set of features and process them, e.g. simulated quantisation
% Note: now quant_features.c does all this and more

function process_features(input_fn, output_fn, varargin)
  nb_features=55;
  nb_bands = 18;

  f=fopen(input_fn,"rb");
  features_lin_in=fread(f,'float32');
  fclose(f);
  
  % convert to matrix
  
  nb_extra_features = 2*nb_bands;
  frames = length(features_lin_in)/nb_features;
  printf("frames: %d\n", frames);
  features = [];
  for i=1:frames
    features = [features; features_lin_in((i-1)*nb_features+1:i*nb_features)'];
  end

  features_ = features;
  
  if (length (varargin) > 0)
    ind = arg_exists(varargin, "qstepdB");
    if ind
      % quantise DCTs to step size
      qstepdB = varargin{ind+1};
      dctLydB = 10*features(:,1:nb_bands);
      dctLydB_ =  qstepdB*round(dctLydB/qstepdB);
      features_(:,1:nb_bands) = dctLydB_/10;
      error = dctLydB_ - dctLydB;
      mse = mean(mean(error .^ 2));
      #{
      dctLy = features(:,1:nb_bands);
      %dctLy(:,1) += 4;
      Ly = idct(dctLy')';
      LydB = 10*Ly;
      LydB_ =  qstepdB*round(LydB/qstepdB);
      dctLydB_ = dct(LydB_')';
      features_(:,1:nb_bands) = dctLydB_/10;
      
      error = LydB_ - LydB;
      mse = mean(mean(error .^ 2));
      #}
      printf("qstepdB: %3.2f dB mse: %3.2f dB\n", qstepdB, mse);
      #figure(1); clf; mesh(LydB_(1:100,:));
    end
  end
  
  % convert back to linear files

  features_lin_out = [];
  for i=1:frames
    features_lin_out = [features_lin_out; features_(i,:)'];
  end
  
  f=fopen(output_fn,"wb");
  fwrite(f, features_lin_out, 'float32');
  fclose(f);
endfunction

function ind = arg_exists(v, str) 
   ind = 0;
   for i=1:length(v)
      if !ind && strcmp(v{i}, str)
        ind = i;
      end     
    end
endfunction
