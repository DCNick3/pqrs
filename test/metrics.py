import os.path
import pickle
from collections import defaultdict


class Metrics:

	def __init__(self, file_name=None):
		self.data = {}
		if file_name is not None:
			self.load(file_name)

	def __len__(self):
		return len(self.data)

	def append(self, dictionary):
		assert isinstance(dictionary, dict)
		assert all(k in dictionary for k in ["image_path", "stdout", "stderr", "return_code", "process_time_ns"])

		self.data[dictionary.pop("image_path")] = dictionary

	def save(self, file_name):
		with open(file_name, "wb") as f:
			pickle.dump(self.data, f)

	def load(self, file_name):
		with open(file_name, "rb") as f:
			self.data = pickle.load(f)

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
