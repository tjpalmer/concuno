#!/usr/bin/env python

def combine_sift(dir):
    from os import listdir
    from os.path import join
    from re import compile
    expected_feature_size = 128
    is_sift_text = compile(r'sift_.*\.txt').match
    extractor_sub = compile(r'^.*_([0-9]+)\..*$').sub
    extract_number = lambda name: extractor_sub(r'\1', name)
    with open(join(data_dir, 'sift_combined.txt'), 'w') as output:
        # Print column headers.
        # The % in front is to make Matlab loading happy. I don't put a space
        # after it, though, so it simplifies to one word for spreadsheet import.
        print >>output, '%File', 'Location', 'Location', 'Scale', 'Angle',
        for i in xrange(expected_feature_size):
            print >>output, 'Feature',
        print >>output
        # Print the actual data.
        for file in sorted(listdir(dir)):
            if is_sift_text(file):
                print 'Parsing: %s' % file
                with open(join(dir, file)) as input:
                    # Line 1 describes the file in general.
                    expected_feature_count, feature_size = (
                        int(i) for i in input.readline().split())
                    if feature_size != expected_feature_size:
                        raise ValueError(
                            'Unexpected feature size: %d' % feature_size)
                    # Starting features at line 2.
                    feature_count = 0
                    for feature in read_features(input, 2, feature_size):
                        print >>output, extract_number(file),
                        print >>output, ' '.join(val for val in feature)
                        feature_count += 1
                    # Audit the count.
                    if feature_count != expected_feature_count:
                        raise ValueError(
                            'Unexpected feature count: %d' % feature_count)

data_dir = '../temp/images'

def main():
    from os import listdir
    from os.path import join
    from re import compile
    dir = join(data_dir, 'pgm')
    is_input_image = compile(r'[0-9_]+\.pgm').match
    # Create sift images and text files.
    for file in listdir(dir):
        if is_input_image(file):
            sift(dir, file)
    combine_sift(dir)
    if False:
        # Find matches between image pairs.
        for file in listdir(dir):
            if is_input_image(file):
                for other in listdir(dir):
                    if other > file and is_input_image(other):
                        match_sift(dir, file, other)
    print 'Done!'

def match_sift(dir, file1, file2):
    from os import mkdir
    from os.path import getsize, isdir, isfile, join
    from re import sub
    from subprocess import call
    match_exe = join(sift_dir, 'match')
    # Prepare output dir and file.
    output_dir = join(data_dir, 'matches')
    if not isdir(output_dir):
        mkdir(output_dir)
    output_file = 'match_' + file1.replace('.pgm', '_') + file2
    output_file = join(output_dir, output_file)
    # If it's not already done, generate it.
    if not (isfile(output_file) and getsize(output_file)):
        with open(output_file, 'w') as output:
            print [
                match_exe,
                '-im1', join(dir, file1),
                '-k1', join(dir, sift_text_file(file1)),
                '-im2', join(dir, file2),
                '-k2', join(dir, sift_text_file(file2)),
            ], output_file
        print 'Output: %s' % output_file

def read_features(input, line_number, feature_size):
    while True:
        # See if we have a feature line.
        line = input.readline()
        if not line: break
        # Read x, y, scale, and angle.
        values = [val for val in line.split()]
        line_number += 1
        if len(values) != 4:
            raise ValueError(
                'Wrong value count (%d) for first line of feature on line %d' \
                % (len(values), line_number))
        # Read feature values.
        expected_len = len(values) + feature_size
        while len(values) < expected_len:
            values += input.readline().split()
            line_number += 1
        yield values

def sift(dir, file):
    from os.path import getsize, isfile, join
    from re import sub
    from subprocess import call
    sift_exe = join(sift_dir, 'sift')
    with open(join(dir, file)) as input:
        # Output sift image.
        output_file = join(dir, 'sift_' + file)
        if not (isfile(output_file) and getsize(output_file)):
            with open(output_file, 'w') as output:
                call([sift_exe, '-display'], stdin=input, stdout=output)
            print 'Output: %s' % output_file
    with open(join(dir, file)) as input:
        # Output sift text file.
        output_file = join(dir, sift_text_file(file))
        if not (isfile(output_file) and getsize(output_file)):
            with open(output_file, 'w') as output:
                call(sift_exe, stdin=input, stdout=output)
            print 'Output: %s' % output_file

sift_dir = '../temp/siftDemoV4'

def sift_text_file(original_image_file):
    return 'sift_' + original_image_file.replace('.pgm', '.txt')

if __name__ == '__main__':
    main()
