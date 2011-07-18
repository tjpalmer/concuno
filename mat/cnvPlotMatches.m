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

      % X and Y are 2 and 3. Presumed standard image coordinates.
      a = [match(2) match(3)];
      % Scale and angle are 4 and 5. I'm not entirely how to employ these, but
      % this is probably better than nothing.
      b = 6 * match(4) * [cos(match(5)) sin(match(5))];;
      b = a + b;

      % Draw the line from the feature point showing orientation and scale.
      plot([a(2) b(2)], [a(1) b(1)], 'b-');

      % Draw the main point on top of the line.
      plot(a(2), a(1), 'r.');
    end
    hold off;

    % Save it.
    saveas(gcf, fullfile(dirName, ['hits-' imageName]));
  end

end
