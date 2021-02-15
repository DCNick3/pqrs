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
import sys
import pickle
import zlib
import base64


def write(obj):
	sys.stdout.write(base64.b64encode(zlib.compress(pickle.dumps(obj))).decode() + "\n")
	sys.stdout.flush()

def process(program, dataset_path):
	for file in sys.stdin:
		file = Path(file.strip("\n"))

		# Try to load image
		try:
			img = Image.open(file)
		except Exception as e:
			# If it is not an image
			write(f"Error while loading image {file}: {repr(e)}")
			continue

		# Convert image to RGB PPM format
		with io.BytesIO() as output:
			img.convert("RGB").save(output, format="PPM")
			img_ppm = output.getvalue()

		# Run program and collect output
		start_time = time.perf_counter_ns()
		p = subprocess.Popen([str(program), "/dev/stdin"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		stdout, stderr = p.communicate(input=img_ppm)
		returncode = p.wait()
		end_time = time.perf_counter_ns()
		
		# Append collected data to metrics
		res = {
			"image_path": str(file.relative_to(dataset_path)),
			"stdout": stdout,
			"stderr": stderr,
			"return_code": returncode,
			"process_time_ns": end_time - start_time,
		}

		write(res)

try:
	process(Path(sys.argv[1]), Path(sys.argv[2]))
except Exception as e:
	write(e)
