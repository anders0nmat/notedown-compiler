import os
from sys import argv
import sys
import time

# 0 : No output at all
# 1 : Output if something was done
# 2 : Output if anything happened
verbose_level = 0
rootdir = r""

def cmd_target():
	return argv[1] if len(argv) >= 2 else ""

def project_path(path):
	"""
	Converts any path to a path relative to the project root
	"""
	return os.path.normcase(os.path.join(rootdir, path))

def make_dir(dirname):
	"""
	Creates the path given if it does not exist
	"""
	if isinstance(dirname, str):
		dirname = [dirname]
	for d_raw in dirname:
		d = os.path.dirname(d_raw)
		if not os.path.isdir(d):
			if verbose_level > 0: print("Folder created:", d)
			os.mkdir(d)
		elif verbose_level > 1: print("Folder already existed", d)

def is_newer(file: str, compare: str | list[str]):
	"""
	Checks if `file` is newer than any of `compare`
	"""
	if isinstance(compare, str):
		compare = [compare]

	return os.path.getmtime(file) > max([os.path.getmtime(e) for e in compare])

def build(target: tuple[str, str] | list[tuple[str, str]], command: str):
	"""
	Formats command with target. Then executes command. 
	If multiple targets are given, it is effectively doing this for every
	target.

	Returns a list of all Targets
	"""
	if isinstance(target, tuple):
		target = [target]

	built_targets = []
	for src, build in target:
		if isinstance(src, str):
			if os.path.exists(src):
				print("Executing:", command.format(f'"{src}"', f'"{build}"'), end="")
				exit_code = os.system(command.format(f'"{src}"', f'"{build}"'))
				if exit_code != 0:
					print("Build Command failed, build aborted.")
					sys.exit(-1)
				built_targets.append(build)
			elif verbose_level > 0: print("Source does not exist:", src)
		elif isinstance(src, list):
			paths = [f'"{e}"' for e in src]
			for path in src:
				if not os.path.exists(path): break
			else:
				print("Executing:", command.format(' '.join(paths), f'"{build}"'), end="")
				exit_code = os.system(command.format(' '.join(paths), f'"{build}"'))
				if exit_code != 0:
					print("Build Command failed, build aborted.")
					sys.exit(-1)
				built_targets.append(build)
				continue
			if verbose_level > 0: print("Source does not exist:", src)
	return built_targets

def rebuild(target: tuple[str, str] | list[tuple[str, str]], command: str):
	"""
	Checks if target is older than any source and if so rebuilds it

	Returns a list of all Targets, even those not rebuilt
	"""
	if isinstance(target, tuple):
		target = [target]
	built_targets = []
	for src, dest in target:
		if not os.path.exists(dest) or not is_newer(dest, src):
			if verbose_level > 0: print("recompiling", dest)
			start_time = time.time()
			build((src, dest), command)
			elapsed = (time.time() - start_time) * 1000
			if verbose_level > 0: print(f" (in {round(elapsed, 2)}ms)")
		elif verbose_level > 1: print("up to date", dest)
		built_targets.append(dest)
	return built_targets

def get_targets_folder(folder: str, target_folder = None, *, filter, target_name):
	"""
	Returns a list of targets based on files in a folder.
	`folder`: the folder to search
	`target_folder`: where the targets will be
	`filter`: func(str) -> bool whether the given item should be included
	`target`: func(str) -> str name of target file
	"""

	if not os.path.isdir(folder):
		if verbose_level > 0: print("Folder does not exist", folder)
		return []

	if target_folder == None:
		target_folder = folder
	
	return [(os.path.normcase(os.path.join(folder, e)), 
		os.path.normcase(os.path.join(target_folder, target_name(e)))) 
		for e in os.listdir(folder) if filter(e)]
