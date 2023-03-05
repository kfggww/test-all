import sys
import os
import json

if __name__ == "__main__":
    if len(sys.argv) <= 1:
        exit(1)

    test_name = sys.argv[1]
    script_dir = os.path.dirname(os.path.realpath(sys.argv[0]))
    project_dir = os.path.realpath(script_dir + "/..")
    tests_bin_dir = os.path.realpath(project_dir + "/install/bin")

    with open(script_dir+"/tests.json", "r") as json_fd:
        tests = json.load(json_fd)

    if test_name not in tests:
        exit(2)

    test_args = tests[test_name]["args"]
    cmd = tests_bin_dir + "/" + test_name + " "
    cmd += " ".join(test_args)
    os.system(cmd)
