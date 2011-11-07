#!/usr/bin/env python


"""
Converts smrf2 JSON files to concuno-readable data and label table files.
"""


def main():
    import json
    from os.path import join
    from re import sub
    from sys import argv
    from time import strftime
    #base = '../temp'
    name = argv[1]
    #case = argv[2]
    time = strftime('%Y%m%d')
    #with open(join(base, 'smrf', name, case) + '.json') as file:
    with open(name) as file:
        bags, fields = json.load(file)
        #out_name = join(base, name) + '_' + case;
        out_name = sub(r"\.json$", "", name)
        features_name = out_name + '_features_' + time + '.txt'
        labels_name = out_name + '_labels_' + time + '.txt'
        with open(features_name, 'w') as features_out:
            with open(labels_name, 'w') as labels_out:
                process(features_out, labels_out, bags, fields)


def process(features_out, labels_out, bags, fields):

    # Remove fields that are bogus for us.
    for bogus_field in ['combo', 'distractor_matlab']:
        if bogus_field in fields:
            fields.remove(bogus_field)

    # For features, print the first column header immediately, but then wait
    # until the first item, so we know how many values for each field.
    print >>features_out, '%Bag',
    first = True

    # As for labels, each smrf dataset has only one.
    print >>labels_out, '%Bag', 'True'

    for bag in bags:
        # Here, __label__ is id, and label is label.
        bag_id = bag['__label__']

        # Features.
        for item in bag['items']:
            if first:
                # Output column headers.
                for field in fields:
                    try:
                        for value in item[field]:
                            print >>features_out, field.capitalize(),
                    except TypeError:
                        # Must not be iterable. Go direct.
                        print >>features_out, field.capitalize(),
                print >>features_out
                first = False
            print >>features_out, bag_id,
            for field in fields:
                values = item[field]
                try:
                    for value in values:
                        # SMRF uses column vectors represented as nested arrays.
                        print >>features_out, value[0],
                except TypeError:
                    # Must not be iterable. Go direct.
                    print >>features_out, values,
            print >>features_out

        # Label. Concuno uses 0 and 1 for False and True.
        print >>labels_out, bag_id, int(bag['label'])


if __name__ == '__main__':
    main()
