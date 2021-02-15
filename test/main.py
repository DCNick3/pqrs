#!/usr/bin/env python3

import io
import time
from pathlib import Path
from tqdm import tqdm
import PIL
from PIL import Image
import argparse
import subprocess
from datetime import datetime

from metrics import Metrics


def process(program, dataset_path, metrics):
	# Make all path absolute
	dataset_path = dataset_path.absolute()
	program = program.absolute()

	# Find all files in dataset
	all_files = list(dataset_path.rglob("*.*"))
	all_files.sort()

	if not all_files:
		raise RuntimeError(f"dataset '{dataset_path}' is empty")

	for i, file in enumerate(tqdm(all_files, leave=False)):

		# Try to load image
		try:
			img = Image.open(file)
		except PIL.UnidentifiedImageError as e:
			print(f"Error while loading image {file}")
			continue # If it is not an image

		# Convert image to RGB PPM format
		with io.BytesIO() as output:
			img.convert("RGB").save(output, format="PPM")
			img_ppm = output.getvalue()

		# Run program and collect output
		start_time = time.perf_counter_ns()
		p = subprocess.Popen([str(program), "/dev/stdin"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		(stdout, stderr) = p.communicate(input=img_ppm)
		returncode = p.wait()
		end_time = time.perf_counter_ns()
		
		# Append collected data to metrics
		metrics.append({
			"image_path": str(file.relative_to(dataset_path)),
			"stdout": stdout,
			"stderr": stderr,
			"return_code": returncode,
			"process_time_ns": end_time - start_time,
			})

	return metrics



parser = argparse.ArgumentParser(description="Testbench", add_help=True)
parser.add_argument("-o", "--output-file", type=str, metavar="FILE_NAME", help="specify name of the output file")
parser.add_argument("-m", "--message", type=str, metavar="<msg>", help="add message to file")
parser.add_argument("PROGRAM", type=str, help="path to program you want to test")
parser.add_argument("DATASET_PATH", type=str, help="path to dataset")
args = parser.parse_args()


if args.output_file is None:
	output_file = datetime.now().strftime("metrics_results/%Y%m%d_%H%M%S_%f.pkl")
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

res = process(program, dataset_path, metrics)

print(f"Save result to {output_file}")
res.save(output_file)