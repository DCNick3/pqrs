import os.path
import zlib
import pickle
from collections import defaultdict
from datetime import datetime


class Metrics:

	def __init__(self, comment=None):
		self.data = {}
		self.info = {"comment": comment}

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

	@classmethod
	def dumps(cls, metrics):
		metrics.set_info("save_time", datetime.now().strftime("%d.%m.%Y %H:%M:%S.%f"))
		data = {"data": metrics.data, "info": metrics.info}
		return zlib.compress(pickle.dumps(data), level=9)

	@classmethod
	def dump(cls, metrics, f):
		f.write(cls.dumps(metrics))

	@classmethod
	def loads(cls, data):
		data = pickle.loads(zlib.decompress(data))

		res = cls()
		res.data = data["data"]
		res.info = data["info"]

		for v in res.data.values():
			assert all(k in v for k in ["stdout", "stderr", "return_code", "process_time_ns"])

		return res

	@classmethod
	def load(cls, f):
		return cls.loads(f.read())

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

	@classmethod
	def concat(cls, metrics, comment=None):
		assert all(isinstance(m, cls) for m in metrics)

		res = cls(comment=comment)
		# res.data = 

