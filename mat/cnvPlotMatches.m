function cnvPlotMatches(center, threshold, features, dirName)

  % Find the matching images.
  diffs = features(:,6:end) - repmat(center, [size(features,1) 1]);
  dists = sqrt(sum(diffs.^2, 2));
  matches = features(dists <= threshold, 1:5);
  uniqueImageIds = unique(matches(:,1));

  % Find the list of images.
  imageNames = dir(dirName);
  imageNames = arrayfun(@(file) file.name, imageNames, 'UniformOutput',false);

  % Diplay each matching image.
  for i = 1:numel(uniqueImageIds)
    imageId = uniqueImageIds(i);

    % Load and display the image.
    imageMatches = strfind(imageNames, num2str(imageId));
    imageMatches = cellfun(@(m) ~isempty(m), imageMatches);
    imageName = imageNames{imageMatches};
    image = imread(fullfile(dirName, imageName));
    %figure(imageId);
    imshow(image);

    hold on;
    currentMatches = matches(matches(:,1) == imageId, :);
    for m = 1:size(currentMatches,1)
      match = currentMatches(m,:);

      % X and Y are 3 and 2, based on plotting code that comes with the SIFT
      % demo.
      a = [match(3) match(2)];
      % Scale and angle are 4 and 5. Usage (including the 6 *) is also based on
      % the SIFT demo plotting code.
      b = 6 * match(4) * [sin(match(5)) cos(match(5))];;
      b = a + b;

      % Draw the line from the feature point showing orientation and scale.
      plot([a(1) b(1)], [a(2) b(2)], 'b-');
      % Draw the main point on top of the line.
      plot(a(1), a(2), 'r.');
    end
    hold off;

    % Save it.
    saveas(gcf, fullfile(dirName, ['hits-' imageName]));
  end

end
