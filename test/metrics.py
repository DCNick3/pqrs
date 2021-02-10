class Metrics:

	def __init__(self):
		self.data = []

	def append(self, dictionary):
		assert isinstance(dictionary, dict)
		assert all(k in dictionary for k in ["returncode", "process_time_ns"])
		self.data.append(dictionary)

	def print(self, verbose=False):
		rc = {}
		for item in self.data:
			code = item["returncode"]
			if code in rc:
				rc[code] += 1
			else:
				rc[code] = 1

		total = sum(rc.values())
		print("Return codes: " + ", ".join(["%s(%.3f%%)" % (k, v/total * 100) for k, v in rc.items()]))
		
		average_process_time_ns = sum(i["process_time_ns"] for i in self.data) / len(self.data)
		print("Average process time %.3fms" % (average_process_time_ns / 1000 / 1000))
