import sys
import json
from collections import OrderedDict

def convert(res_file, out_file):
    with open(res_file) as rf:
        new_json = json.loads(rf.read(), object_pairs_hook=OrderedDict)

    for bm_matrix in new_json:
        converted = 0
        print(f"Converting {bm_matrix['bm_name']}")
        for bm in bm_matrix['benchmarks']:
            bw = bm['bandwidth']
            if len(bw) != 2:
                continue

            # has read and write
            write_ratio = bm['config']['write_ratio']
            bw['read'] = bw['read'] * (1 - write_ratio)
            bw['write'] = bw['write'] * write_ratio
            converted += 1

        print(f" --> converted: {converted}")

    with open(out_file, 'w') as of:
        of.write(json.dumps(new_json, indent=2, sort_keys=True))


if __name__ == '__main__':
    convert(sys.argv[1], sys.argv[2])