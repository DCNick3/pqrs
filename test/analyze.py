#!/usr/bin/env python3

import argparse
from pathlib import Path

from metrics import Metrics


def print_general_statistics(stage_name, metrics):
	positive = len(metrics.filter(lambda k, v: v["return_code"] == 0)) / len(metrics)
	negative = len(metrics.filter(lambda k, v: v["return_code"] == 255)) / len(metrics)
	errors = len(metrics.filter(lambda k, v: v["return_code"] not in [0, 255])) / len(metrics)

	average_process_time_ns = sum(v["process_time_ns"] for v in metrics.get_data().values()) / len(metrics)

	print(f"{stage_name}:")
	print("\tpositive/negative/errors: %.1f/%.1f/%.3f" % (positive * 100, negative * 100, errors * 100))
	print("\taverage process time: %.3fms" % (average_process_time_ns / 1000 / 1000))


def print_errors(stage_name, metrics):
	errors = metrics.filter(lambda k, v: v["return_code"] not in [0, 255])
	if not errors:
		print(f"{stage_name}: No errors detected")
		return

	print(f"{stage_name}: Files, which return errors:")
	print("\t" + "\n\t".join([f"{k}: {v['return_code']}" for k, v in errors.items()]))


def folder_statistics(stage_name, metrics):
	res = metrics.split_by_folders()

	print(f"{stage_name}:")

	for k, v in res.items():
		positive = len(v.filter(lambda k, v: v["return_code"] == 0)) / len(v)
		print(f"\t{k}: %.1f%%" % (positive * 100))


def file_statistics(stage_name, metrics):
	print(f"{stage_name}:")

	for k, v in metrics.get_data().items():
		print(f"\t{k}: {v['return_code']}")



parser = argparse.ArgumentParser(description="Program for analyze results of the test", add_help=True)
parser.add_argument("-v", "--verbose", action="store_true", help="print detailed output")
parser.add_argument("METRICS_FILE", type=str, help="path to file with metrics you want to analyze")
args = parser.parse_args()

file_name = Path(args.METRICS_FILE)

if not file_name.is_file():
	raise RuntimeError(f"'{file_name}' is not a file")

metrics = Metrics(file_name)
print_general_statistics("Stage01 (general)", metrics)
print_errors("Stage02 (errors)", metrics)
folder_statistics("Stage03 (folder statistics)", metrics)

if args.verbose:
	file_statistics("Stage04 (file statistics)", metrics)
else:
	print("Stage04 (file statistics): skipped (use flag -v to run it)")

