% design_huffman.m
% David Rowe Jan 2019
%
% Octave script to design a huffman encoder, and measure entropy

% D is a matrix, each row is a vector of features

function [symbols huff] = design_huffman(D, qstepdB=3)
  [nr K] = size(surf);

  printf("K: %d nr: %d qstepdB: %3.2f\n", K, nr, qstepdB);

  % quantise to step size in dB
    
  E = round(D/qstepdB);

  % count symbols

  symbols = []; count = [];
  [nr nc]= size(E);
  for r=1:nr
    for c=1:nc
      s = E(r,c);
      ind = find(symbols == s);
      if length(ind)
        count(ind)++;
      else
        symbols = [symbols s];
        count(length(symbols)) = 1;
      end
    end
  end

  % sort into order

  [count ind] = sort(count, "descend");
  symbols = symbols(ind);

  Nsymbols = sum(count);
  printf("Nsymbols = %d\n", Nsymbols);
  
  % estimate entropy
    
  H = 0;
  p_table = [];
  printf(" i symb  count  prob    wi\n");
  for i=1:length(symbols)
    wi = count(i)/Nsymbols; p_table = [p_table wi];
    printf("%2d %4d %6d %4.3f %4.3f\n", i, symbols(i), count(i), wi, -wi*log2(wi));
    H += -wi*log2(wi);
  end

  % design Huffman code

  huff = huffmandict (1, p_table, 1);
  L = 0;
  for i=1:length(huff)
    L += p_table(i)*length(huff{i});
  end

  printf("Entropy: %3.2f bits/symbol  Huffman code: %3.2f bits/symbol %3d bits/fr\n",  H, L, ceil(L*nc));
endfunction
