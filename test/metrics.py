import os.path


class Metrics:

	def __init__(self):
		self.data = {}

	def append(self, dictionary):
		assert isinstance(dictionary, dict)
		assert all(k in dictionary for k in ["image_path", "returncode", "process_time_ns"])

		image_path = os.path.dirname(dictionary["image_path"])
		if image_path in self.data:
			self.data[image_path].append(dictionary)
		else:
			self.data[image_path] = [dictionary]

	def print_verbose(self):
		for path, item in self.data.items():

			for el in item:
				print(f"{el['image_path']}: {el['returncode']}")


	def print(self, verbose=False):
		if verbose: return self.print_verbose()

		for path, item in self.data.items():
			print(f"{path}:")

			rc = {}
			for el in item:
				code = el["returncode"]
				if code in rc:
					rc[code] += 1
				else:
					rc[code] = 1

			total = sum(rc.values())

			print("\tReturn codes: " + ", ".join(["%s(%.3f%%)" % (k, v/total * 100) for k, v in sorted(rc.items())]))
			
			average_process_time_ns = sum(i["process_time_ns"] for i in item) / len(item)
			print("\tAverage process time %.3fms" % (average_process_time_ns / 1000 / 1000))
