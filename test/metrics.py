import os.path
import zlib
import pickle
from collections import defaultdict
from datetime import datetime


class Metrics:

	def __init__(self, file_name=None, comment=None):
		self.data = {}
		self.info = {"comment": comment}
		if file_name is not None:
			self.load(file_name)

	def __len__(self):
		return len(self.data)

	def append(self, dictionary):
		assert isinstance(dictionary, dict)
		assert all(k in dictionary for k in ["image_path", "stdout", "stderr", "return_code", "process_time_ns"])

		self.data[dictionary.pop("image_path")] = dictionary

	def set_info(self, key, value):
		self.info[key] = value

	def get_info(self):
		return self.info

	def save(self, file_name):
		self.set_info("save_time", datetime.now().strftime("%d.%m.%Y %H:%M:%S.%f"))

		data = {"data": self.data, "info": self.info}

		with open(file_name, "wb") as f:
			f.write(zlib.compress(pickle.dumps(data), level=9))

	def load(self, file_name):
		with open(file_name, "rb") as f:
			data = pickle.loads(zlib.decompress(f.read()))

		self.data = data["data"]
		self.info = data["info"]

		for v in self.data.values():
			assert all(k in v for k in ["stdout", "stderr", "return_code", "process_time_ns"])

	def get_data(self):
		"""Get all data"""
		return self.data

	def filter(self, filter):
		"""Get list of all data which pass throw filter."""
		return {k: v for k, v in self.data.items() if filter(k, v)}

	def split_by_folders(self):
		res = defaultdict(Metrics)

		for k, v in self.data.items():
			group = os.path.dirname(k)
			res[group].append({"image_path": k, **v})

		return dict(res)
