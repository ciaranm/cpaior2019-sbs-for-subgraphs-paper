from functools import cmp_to_key

algorithms = [
	"sequential13",
	"sequentialinputordersoftmax13",
	"sequentialshuffle13",
	"sequentialantiheuristic13",
	"sequentialinputordersoftmaxrestarts13",
	"sequentialrestartsshuffle13",
	"sequentialdds13"
]

results_dir = "results/"
instances_file = "instances.txt"

output_files = [open("{}{}.choices.data".format(results_dir, algorithm), "w") for algorithm in algorithms]
extremal_choice_counts = [[0,0] for a in algorithms]
extremal_zeroes = [[0,0] for a in algorithms]

for file in output_files:
	file.write("# instance family n_choices choice frac_choice heur_choice frac_heur_choice depth max_depth frac_depth weird\n")

def heur_compare(item1, item2):
	cmp_deg = item2[1] - item1[1]
	if cmp_deg:
		return cmp_deg
	else:
		return item1[0] - item2[0]

def parse_file(instance, family, algorithm, pattern_path, outstream, count_store, zeroes):
	filename = "{}{}/{}.out".format(results_dir, algorithm, name)

	with open(pattern_path, "r") as pattern_file:
		max_depth = int(pattern_file.readline().strip())

	try:
		with open(filename, "r") as stats_file:
			data = [l.strip().split(" =") for l in stats_file.readlines()]

			# status
			if len(data) < 6 or data[5][1].strip()=="false":
				return

			# where
			choice_depths = [i+1 for (i, val) in enumerate(data[9][1].strip().split(" ")) if int(val)>=0]

			c_index = -1
			for i, [lhs, rhs] in enumerate(data):
				if lhs == "choices":
					c_index = i
					break

			# choices
			if c_index >= 0 and len(data[c_index][1].strip()) > 0:
				for depth, entry in zip(choice_depths, data[c_index][1].split(".")):
					if not len(entry):
						continue

					[choices_str, choice_index_str] = entry.strip()[1:].split("], ")

					choice_index = int(choice_index_str)
					choices = [tuple(map(int, pair)) for pair in [x.split("-") for x in choices_str.split(",")]]

					n_choices = len(choices)
					frac_choice = float(choice_index)/n_choices

					the_choice = choices[choice_index]
					heur_choices = sorted(choices, key=cmp_to_key(heur_compare))
					heur_choice_index = heur_choices.index(the_choice)
					frac_heur_choice_index = float(heur_choice_index)/n_choices

					frac_depth = float(depth)/max_depth

					if frac_depth <= 0.1:
						count_store[0] += 1
						if choice_index == 0:
							zeroes[0]+=1
					elif frac_depth >= 0.9:
						count_store[1] += 1
						if choice_index == 0:
							zeroes[1]+=1

					weird = frac_choice / frac_depth

					outstream.write("{} {} {} {} {} {} {} {} {} {} {}\n".format(
						instance, family, n_choices, choice_index, frac_choice, heur_choice_index, frac_heur_choice_index, depth, max_depth, frac_depth, weird
					))
	except FileNotFoundError:
		return
	

with open(instances_file, "r") as instances:
	for line in instances:
		[name, pat, targ, family] = line.strip().split(" ")
		for algorithm, out_file, count_store, zeroes in zip(algorithms, output_files, extremal_choice_counts, extremal_zeroes):
			parse_file(name, family, algorithm, pat, out_file, count_store, zeroes)

for algo, counts, zeroes in zip(algorithms, extremal_choice_counts, extremal_zeroes):
	print(algo, counts, zeroes)

