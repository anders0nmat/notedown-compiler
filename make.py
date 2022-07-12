# TODO make this a makefile replacement because i dont understand sh*t about makefiles
import os
import time
import pymake

pymake.verbose_level = 2
pymake.rootdir = os.path.dirname(__file__)

builds = pymake.project_path(r"build/")
src = pymake.project_path(r"src/")
output = pymake.project_path(r"bin/notedown.exe")

cc = r"g++"
ccflags = r"-fdiagnostics-color=always -O0 -std=c++20 -g"

targets = pymake.get_targets_folder(
	src, builds,
	filter=lambda e : e.endswith(".cpp"),
	target_name=lambda e : e + ".o")

start_time = time.time()

pymake.make_dir([builds, output])

# Rebuild all objs
objs = pymake.rebuild(targets, f"{cc} {ccflags} -c {{0}} -o {{1}}")

# Rebuild exe
pymake.rebuild((objs, output), f"{cc} {ccflags} {{0}} -o {{1}}")

elapsed = time.time() - start_time
print(f"Done in {round(elapsed * 1000, 2)}ms")
