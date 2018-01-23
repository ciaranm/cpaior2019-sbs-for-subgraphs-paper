#!/usr/bin/python

import sys, os, time, re
from subprocess import Popen, PIPE

# algo, instance, instance information, cutoff time, cutoff length, seed,
# param...

# Read in first 5 arguments.
instance = sys.argv[1]
ignored_1 = sys.argv[2]
cutoff = int(float(sys.argv[3]) + 1)
seed = int(sys.argv[4])
ignored_2 = int(sys.argv[5])

params = sys.argv[6:]
configMap = dict((name, value) for name, value in zip(params[::2], params[1::2]))

filename1, filename2 = instance.strip().split("@")
cmd = "../../code/solve_subgraph_isomorphism --luby-multiplier {} --timeout {} --restarts --softmax-shuffle --input-order customisable-sequential ../{} ../{}".format(
        configMap['-luby-multiplier'], cutoff, filename1, filename2)

# Execute the call and track its runtime.
#start_time = time.time()
#res = os.system(cmd)
#runtime = time.time() - start_time

io = Popen(cmd.split(" "), stdout=PIPE, stderr=PIPE)
(stdout_, stderr_) = io.communicate()

status = "SUCCESS"
if io.returncode != 0:
    status = "CRASHED"
elif re.search('status = aborted', stdout_):
    status = "TIMEOUT"

lines = stdout_.split("\n")
for line in lines:
    if re.search('runtime =', line):
        runtime = float(line.split()[2]) / 1000

# status, runtime, runlength, quality, seed, additional stuff
print("Result of algorithm run: %s, %f, 0, 0, %i" %(status, runtime, seed))
