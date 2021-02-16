#!/usr/bin/env python3

import argparse
import os
import sys
from pathlib import Path

from main import process, Metrics

parser = argparse.ArgumentParser(description="Testbench", add_help=True)
parser.add_argument("PROGRAM", type=str, help="path to program you want to test")
args = parser.parse_args()
    
dataset_path = Path(os.path.dirname(os.path.realpath(__file__)) + "/basic_testcases")
program = Path(args.PROGRAM)

metrics = Metrics(comment='')
process(program, dataset_path, metrics, 1)

positive = len(metrics.filter(lambda k, v: v["return_code"] == 0))
negative = len(metrics.filter(lambda k, v: v["return_code"] == 255))
errors = len(metrics.filter(lambda k, v: v["return_code"] not in [0, 255]))
print("positive/negative/errors: %d/%d/%d" % (positive, negative, errors))

if negative != 0 or errors != 0:
    print("Some tests are not passing")
    sys.exit(1)
