#!/usr/bin/env python3

import argparse
from pathlib import Path

from metrics import Metrics


def print_info(metrics):
	info = metrics.get_info()

	print(f"\tComment: {info['comment']}")
	print(f"\tSave time: {info['save_time']}")


def print_general_statistics(metrics):
	positive = len(metrics.filter(lambda k, v: v["return_code"] == 0)) / len(metrics)
	negative = len(metrics.filter(lambda k, v: v["return_code"] == 255)) / len(metrics)
	errors = len(metrics.filter(lambda k, v: v["return_code"] not in [0, 255])) / len(metrics)

	average_process_time_ns = sum(v["process_time_ns"] for v in metrics.get_data().values()) / len(metrics)

	print("\tpositive/negative/errors: %.1f/%.1f/%.3f" % (positive * 100, negative * 100, errors * 100))
	print("\taverage process time: %.3fms" % (average_process_time_ns / 1000 / 1000))


def print_errors(metrics):
	errors = metrics.filter(lambda k, v: v["return_code"] not in [0, 255])
	if not errors:
		print(f"\tNo errors detected")
		return

	print(f"\tFiles, which return errors:")
	print("\t" + "\n\t".join([f"{k}: {v['return_code']}" for k, v in errors.items()]))


def folder_statistics(metrics):
	res = metrics.split_by_folders()


	for k, v in res.items():
		positive = len(v.filter(lambda k, v: v["return_code"] == 0)) / len(v)
		print(f"\t{k}: %.1f%%" % (positive * 100))


def file_statistics(metrics):
	global args
	if not args.verbose:
		print("\tskipped (use flag -v to run it)")
		return

	for k, v in metrics.get_data().items():
		print(f"\t{k}: {v['return_code']}")



parser = argparse.ArgumentParser(description="Program for analyze results of the test", add_help=True)
parser.add_argument("-v", "--verbose", action="store_true", help="print detailed output")
parser.add_argument("METRICS_FILE", type=str, help="path to file with metrics you want to analyze")
args = parser.parse_args()

metrics = Metrics(args.METRICS_FILE)

stages = {
	"Stage00 (info)": print_info,
	"Stage01 (general)": print_general_statistics,
	"Stage02 (errors)": print_errors,
	"Stage03 (folder statistics)": folder_statistics,
	"Stage04 (file statistics)": file_statistics,
}


for k, v in stages.items():
	print(f"\033[1;33m{k}\033[0m:")
	v(metrics)