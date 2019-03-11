import subprocess, os

while (1):
  try:
    i = input("> ").split()
    f = open("tmp.S", "w")
    f.write(".byte 0x" + ', 0x'.join(i))
    f.close()

    subprocess.run(["gcc", "-c", "tmp.S", "-o", "tmp.o"])
    if os.path.isfile("tmp.o"):
      out = subprocess.check_output("objdump -D tmp.o", shell=True).decode("ascii").splitlines()

      in_text = False
      for line in out:
        if "<.text>" in line:
          in_text = True
          continue

        if in_text:
          print(line)

  except KeyboardInterrupt:
    print()
    break
