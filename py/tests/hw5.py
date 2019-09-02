import subprocess, os

__BIN = os.path.dirname(__file__) + "/../../recfun"

__TESTS = [
    ("/hw5/test1.s", "/hw5/test1.txt")
]

def hex2elf(hexfile):
    out = hexfile.replace(".s", ".out")

    subprocess.run(["gcc", hexfile, "-o", out, "-nostdlib"])
    if os.path.isfile(out):
        return out

def run():
    for (path, txt) in __TESTS:
        # Cvt
        test_file = hex2elf(os.path.dirname(__file__) + path)

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
