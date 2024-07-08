#!/usr/bin/env python3

import re
import subprocess
import sys


def track_includes(start_file):
    remove_start = re.compile("include/")

    def clean(file):
        return re.sub(remove_start, "", file)

    unseen_files = [start_file]
    res = {}
    # import ipdb
    # ipdb.set_trace(context=30)
    while unseen_files:
        current_file = unseen_files[0]
        del unseen_files[0]

        cmd = ["grep", "-Rl", clean(current_file), "include/"]
        try:
            next_files = subprocess.check_output(cmd)
            lines = [line for line in next_files.decode().splitlines() if line]
        except subprocess.CalledProcessError:
            lines = []

        res[current_file] = lines
        for line in lines:
            if line not in res:
                unseen_files.append(line)
    return res


if __name__ == "__main__":

    remove_start = re.compile("include/(openPMD/)?")
    remove_slash = re.compile("/|\\.")

    def clean(file):
        return re.sub(remove_slash, "_", re.sub(remove_start, "", file))

    res = track_includes(sys.argv[1])
    print("digraph {")
    for key, values in res.items():
        key = clean(key)
        for target in values:
            print("\t{} -> {};".format(key, clean(target)))
    print("}")
