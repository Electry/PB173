import subprocess, os

__BIN = os.path.dirname(__file__) + "/../../recfun"

__TESTS = [
    ("/hw5/test1.s", "/hw5/test1.txt", ""),
    ("/hw5/test2.s", "/hw5/test2.txt", "-s")
]

def hex2elf(hexfile, gcc_arg):
    out = hexfile.replace(".s", ".out")

    arg = ["gcc", hexfile, "-o", out, "-nostdlib"]
    if len(gcc_arg) > 0:
        arg.append(gcc_arg)

    subprocess.run(arg)
    if os.path.isfile(out):
        return out

def run():
    for (path, txt, gcc_arg) in __TESTS:
        # Cvt
        test_file = hex2elf(os.path.dirname(__file__) + path, gcc_arg)

        f = open(os.path.dirname(__file__) + txt, "r")
        expected = ''.join(map(str.strip, f.readlines()))
        f.close()

        result = ''.join(map(str.strip, subprocess.check_output(__BIN + " " + test_file, shell=True).decode("ascii").split("\n")))

        if result != expected:
            print("Test-case " + test_file + " failed!")
            print(result)
            print("~~~~~~~~~~~~~~~")
            print(expected)
            return

    print("All tests completed SUCCESSFULLY!")

if __name__== "__main__":
    run()
