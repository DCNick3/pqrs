#!/usr/bin/env python3

import io
import time
from pathlib import Path
from tqdm import tqdm
import PIL
from PIL import Image
import argparse
import subprocess

from metrics import Metrics


def main(program, dataset_path):
	all_files = [p for p in dataset_path.rglob("*.*")]  # .
	all_files.sort()

	if not all_files:
		raise RuntimeError(f"dataset '{dataset_path}' is empty")

	metrics = Metrics()

	with tqdm(total=len(all_files), leave=False) as pbar:
		for i, file in enumerate(all_files):
			pbar.n = i
			pbar.refresh()

			try:
				img = Image.open(file)
			except PIL.UnidentifiedImageError as e:
				continue

			with io.BytesIO() as output:
				img.save(output, format="PPM")
				img_ppm = output.getvalue()

			start_time = time.perf_counter_ns()
			p = subprocess.Popen([str(program.absolute()), "/dev/stdin"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
			(stdout, stderr) = p.communicate(input=img_ppm)
			returncode = p.wait()
			end_time = time.perf_counter_ns()
			
			metrics.append({
				"image_path": str(file.relative_to(dataset_path)),
				"stdout": stdout,
				"stderr": stderr,
				"returncode": returncode,
				"process_time_ns": end_time - start_time,
				})

	metrics.print(verbose=args.verbose)



parser = argparse.ArgumentParser(description="Testbanch", add_help=True)
parser.add_argument("-v", "--verbose", action="store_true", help="show more output")
parser.add_argument("PROGRAM", type=str, help="path to program you want to test")
parser.add_argument("DATASET_PATH", type=str, help="path to dataset")
args = parser.parse_args()

program = Path(args.PROGRAM)
dataset_path = Path(args.DATASET_PATH)

if not dataset_path.is_dir():
	raise RuntimeError(f"'{dataset_path}' is not a directory")

if not program.is_file():
	raise RuntimeError(f"'{program}' is not a file")

main(program, dataset_path)