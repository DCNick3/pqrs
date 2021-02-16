#!/usr/bin/env python3

import time
from pathlib import Path
from tqdm import tqdm
import argparse
import subprocess
from datetime import datetime
import numpy as np
import zlib
import pickle
import base64
import os
import fcntl

from metrics import Metrics

worker_path = os.path.dirname(os.path.abspath(__file__)) + "/worker.py"

def set_non_block(output):
    fd = output.fileno()
    fl = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)


def read_line(p):
    res = p.stdout.readline()
    if not res: return None
    print(res)
    res = res.decode().strip("\n")
    return pickle.loads(zlib.decompress(base64.b64decode(res)))

def write(p, s):
    p.stdin.write(f"{s}\n".encode())
    p.stdin.flush()


def process(program, dataset_path, metrics, jobs=1):
    # Make all path absolute
    dataset_path = dataset_path.absolute()
    program = program.absolute()

    # Find all files in dataset
    all_files = list(dataset_path.rglob("*.*"))
    np.random.shuffle(all_files)

    assert len(all_files) > 2 * jobs

    processes = []
    for i in range(jobs):
        p = subprocess.Popen([worker_path, str(program), str(dataset_path)], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        set_non_block(p.stdout)
        processes.append(p)
        write(p, all_files[0])
        write(p, all_files[1])
        all_files = all_files[2:]


    processes_i = 0
    for file in tqdm(all_files):
        while True:
            p = processes[processes_i]
            res = read_line(p)
            if res is not None: break
            processes_i = (processes_i + 1) % len(processes)
            # time.sleep(0.01)

        write(p, file)

        if isinstance(res, dict):
            metrics.append(res)
        else:
            print(res)

    for p in processes:
        for i in range(2):
            while True:
                res = read_line(p)
                if res is not None: break
                time.sleep(0.1)

            if isinstance(res, dict):
                metrics.append(res)
            else:
                print(res)

        stdout, _ = p.communicate()
        p.wait()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Testbench", add_help=True)
    parser.add_argument("-o", "--output-file", type=str, metavar="FILE_NAME", help="specify name of the output file")
    parser.add_argument("-m", "--message", type=str, metavar="msg", help="add message to file")
    parser.add_argument("-j", "--jobs", type=int, default=1, metavar="jobs", help="specifies the number of jobs (commands) to run simultaneously")
    parser.add_argument("PROGRAM", type=str, help="path to program you want to test")
    parser.add_argument("DATASET_PATH", type=str, help="path to dataset")
    args = parser.parse_args()


    if args.output_file is None:
        output_file = datetime.now().strftime("metrics_results/%Y%m%d_%H%M%S_%f.mtr")
        output_file = Path(__file__).absolute().parent.joinpath(output_file)
    else:
        output_file = Path(args.output_file).absolute()

    if output_file.is_file():
        raise RuntimeError("Output file already exist")

    if not output_file.parent.is_dir():
        raise RuntimeError(f"Directory {output_file.parent} do not exist")

    program = Path(args.PROGRAM)
    dataset_path = Path(args.DATASET_PATH)

    if not dataset_path.is_dir():
        raise RuntimeError(f"'{dataset_path}' is not a directory")

    if not program.is_file():
        raise RuntimeError(f"'{program}' is not a file")

    # Metrics object will store all collected data
    metrics = Metrics(comment=args.message)
    process(program, dataset_path, metrics, args.jobs)

    print(f"Save result to {output_file}")

    with open(output_file, "wb") as f:
        Metrics.dump(metrics, f)
