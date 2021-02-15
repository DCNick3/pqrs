#!/usr/bin/env python3

import argparse
from pathlib import Path

from metrics import Metrics


def show_not_implemented():
	print("\033[1;31m" + repr(NotImplementedError("Когда понадобится, тогда сделаем")) + "\033[0m")


def print_info(metrics1, metrics2):
	info1 = metrics1.get_info()
	info2 = metrics2.get_info()

	print(f"\tComment: '{info1['comment']}'  ->  '{info2['comment']}'")
	print(f"\tSave time: {info1['save_time']}  ->  {info2['save_time']}")


def print_general_statistics(metrics1, metrics2):

	def get(metrics):
		positive = len(metrics.filter(lambda k, v: v["return_code"] == 0)) / len(metrics)
		negative = len(metrics.filter(lambda k, v: v["return_code"] == 255)) / len(metrics)
		errors = len(metrics.filter(lambda k, v: v["return_code"] not in [0, 255])) / len(metrics)

		average_process_time_ns = sum(v["process_time_ns"] for v in metrics.get_data().values()) / len(metrics)

		return "%.1f/%.1f/%.3f" % (positive * 100, negative * 100, errors * 100), "%.3fms" % (average_process_time_ns / 1000 / 1000)

	pne1, apt1 = get(metrics1)
	pne2, apt2 = get(metrics2)

	print(f"\tpositive/negative/errors: {pne1}  ->  {pne2}")
	print(f"\taverage process time: {apt1}  ->  {apt2}")


def print_errors(metrics1, metrics2):
	errors1 = metrics1.filter(lambda k, v: v["return_code"] not in [0, 255])
	errors2 = metrics2.filter(lambda k, v: v["return_code"] not in [0, 255])
	if not errors1 and not errors2:
		print(f"\tNo errors detected")
		return

	show_not_implemented()

	# print(f"\tFiles, which return errors only:")
	# print("\t" + "\n\t".join([f"{k}: {v['return_code']}" for k, v in errors.items()]))


def folder_statistics(metrics1, metrics2):
	res1 = metrics1.split_by_folders()
	res2 = metrics2.split_by_folders()

	if res1.keys() ^ res2.keys():
		show_not_implemented()
		return

	for k in res1:
		positive1 = len(res1[k].filter(lambda k, v: v["return_code"] == 0)) / len(res1[k])
		positive2 = len(res2[k].filter(lambda k, v: v["return_code"] == 0)) / len(res2[k])
		print(f"\t{k}: %.1f%%  ->  %.1f%%" % (positive1 * 100, positive2 * 100))


def file_statistics(metrics1, metrics2):
	global args
	if not args.verbose:
		print("\tskipped (use flag -v to run it)")
		return

	if metrics1.get_data().keys() ^ metrics2.get_data().keys():
		show_not_implemented()
		return

	positive1 = metrics1.filter(lambda k, v: v["return_code"] == 0)
	positive2 = metrics2.filter(lambda k, v: v["return_code"] == 0)

	print("\tРаспознаются в [1] и не распознаются в [2]:")  # TODO: translate to English
	res = sorted(list(positive1.keys() - positive2.keys()))
	if not res:
		print("Nothing")
	else:
		print("\t\t" + "\n\t\t".join(res))

	print("\tРаспознаются в [2] и не распознаются в [1]:")  # TODO: translate to English
	res = sorted(list(positive2.keys() - positive1.keys()))
	if not res:
		print("Nothing")
	else:
		print("\t\t" + "\n\t\t".join(res))



parser = argparse.ArgumentParser(description="Program for compare results between two test", add_help=True)
parser.add_argument("-v", "--verbose", action="store_true", help="print detailed output")
parser.add_argument("METRICS_FILE1", type=str, help="path to the first file with metrics")
parser.add_argument("METRICS_FILE2", type=str, help="path to the second file with metrics")
args = parser.parse_args()

with open(args.METRICS_FILE1, "rb") as f:
	metrics1 = Metrics.load(f)

with open(args.METRICS_FILE2, "rb") as f:
	metrics2 = Metrics.load(f)

stages = {
	"Stage00 (info)": print_info,
	"Stage01 (general)": print_general_statistics,
	"Stage02 (errors)": print_errors,
	"Stage03 (folder statistics)": folder_statistics,
	"Stage04 (file statistics)": file_statistics,
}


for k, v in stages.items():
	print(f"\033[1;33m{k}\033[0m:")
	v(metrics1, metrics2)
