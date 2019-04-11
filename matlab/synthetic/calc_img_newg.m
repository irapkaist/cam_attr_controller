function EWG = calc_img_ewg (img)
% function NEWG = calc_img_newg ()

img = im2double(img); % normalize
img = imresize(img,0.25); % image down scale 
% s_img = rgb2gray(s_img); 

% local entropy mask size = (3x3)
H = entropyfilt(img, ones(3)); 
mu = mean(sum(H)); 
sigma=std2(sum(H));
norm_entropy = normpdf(H, mu, sigma);

% generate saturation mask
[rs, cs] = find(H <= 0.01); % Saturated region  
M = [rs, cs]; 

% entropy weight
H_weight = norm_entropy / sum(sum(norm_entropy));

% image Gradient
[Gmag, Gdir] = imgradient(img,'sobel'); 

% Ours
G_weight= Gmag >= (mean(mean(Gmag))) * 0.1 ;

% Saturation weight
S = zeros(size(img));
S(M(:,1), M(:,2)) = (mean(mean(Gmag))) * 2;    



% Metric
EWG = sum(sum(G_weight .* Gmag - S));
