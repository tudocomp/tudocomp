#!/usr/bin/env python3

from python_libs import sh

import argparse
import os
from os import path
from pathlib import Path

parser = argparse.ArgumentParser()

parser.add_argument("dir")
parser.add_argument("dataset_config")
parser.add_argument("--delete_original", action="store_true")
parser.add_argument("--max_prefix_size", type=int)

args = parser.parse_args()

sh.mkdir("-p", args.dir)

TMP_DIR = path.join(args.dir, "tmp")
sh.mkdir("-p", TMP_DIR)

STAMP_DIR = path.join(args.dir, "stamp")
sh.mkdir("-p", STAMP_DIR)

with open(args.dataset_config, 'r') as myfile:
    config = myfile.read()

def filesize(p):
    return path.getsize(str(p))

def abspath(p):
    return path.abspath(str(p))

def filesize_in_mb(p):
    return int(filesize(p) / 1024 / 1024)

def gen_prefix(p, sizes):
    if path.exists(str(p)):
        file_size_in_mb = filesize_in_mb(p)
        for size in sizes:
            under_cutoff = True
            if args.max_prefix_size:
                under_cutoff = (size <= args.max_prefix_size)
            if size <= file_size_in_mb and under_cutoff:
                prefix_file_name = str(p) + "." + str(size) + "MB"
                if (not path.exists(str(prefix_file_name))) or (filesize_in_mb(prefix_file_name) != size):
                    print("Generating {}MB prefix of {} to {}".format(size, p, prefix_file_name))
                    sh.sh("-c", "cat {} | dd count={}K bs=1K > {}".format(p, size, prefix_file_name))
            elif under_cutoff:
                print("Info: {} is too small to generate {}MB prefix".format(p, size))

def remove_suffix(path):
    return Path(
        Path(path).parent,
        Path(path).stem
    )

def download_and_extract(OUT_DIR, SIZES, URLS):
    SIZES.sort()

    # max_size = max(*SIZES)
    # Can not download only the maximum prefix, because
    # the downloaded files can be compressed, which prevents a partial download

    for url in URLS:
        url_filename = path.basename(url)
        tmp_download_path = path.join(TMP_DIR, url_filename)

        target_filename = OUT_DIR + "_" + url_filename
        target_path = Path(args.dir, target_filename)
        stamp_target_path = Path(STAMP_DIR, target_filename)
        preprocess = ""

        if tmp_download_path.endswith(".gz"):
            target_path = remove_suffix(target_path)
            preprocess = "gz"
        elif tmp_download_path.endswith(".7z"):
            target_path = remove_suffix(target_path)
            preprocess = "7z"
        elif tmp_download_path.endswith(".xz"):
            target_path = remove_suffix(target_path)
            preprocess = "xz"

        if not path.exists(str(stamp_target_path)):
            print("Download", target_path)

            sh.wget("-c", "-O", tmp_download_path, url, _fg=True)

            if preprocess != "":
                print("Extract downloaded file...")
            if preprocess == "gz":
                sh.gzip("-d", tmp_download_path)
                tmp_download_path = remove_suffix(tmp_download_path)
            elif preprocess == "7z":
                _7z = sh.Command("7z")
                _7z("e", "-o" + TMP_DIR, tmp_download_path)
                sh.rm(tmp_download_path)
                tmp_download_path = remove_suffix(tmp_download_path)
            elif preprocess == "xz":
                sh.xz("-d", tmp_download_path)
                tmp_download_path = remove_suffix(tmp_download_path)

            sh.mv(tmp_download_path, target_path)

        gen_prefix(target_path, SIZES)

        if not path.exists(str(stamp_target_path)):
            if args.delete_original:
                sh.rm(target_path)
                sh.touch(stamp_target_path)

exec(config)
