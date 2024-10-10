import argparse
import os
import subprocess
import shutil
import glob

def run_command(command, shell=True, cwd=None):
	if os.name == 'nt':  # Check if the OS is Windows
		command = f'powershell.exe -Command "{command}"'
	result = subprocess.run(command, shell=shell, check=True, text=True, cwd=cwd)
	return result

def main():
	parser = argparse.ArgumentParser(description="CMake Build and Test Script")
	parser.add_argument('--os', required=True, choices=['linux', 'windows', 'macos'], help='Operating System')
	parser.add_argument('--build_type', default='Debug', choices=['Release', 'Debug'], help='Build Type')
	parser.add_argument('--platform', choices=['linux_x64', 'linux_x86', 'osx', 'win32', 'win64', 'uwp'], help='Platform string for CMake')
	parser.add_argument('--build', action='store_true', help='Execute the build step')
	parser.add_argument('--test', action='store_true', help='Execute the test step')
	args = parser.parse_args()

	build_output_dir = os.path.join(os.getcwd(), 'build')
	os.makedirs(build_output_dir, exist_ok=True)

	if args.os == 'windows':
		c_compiler = 'cl'
		cpp_compiler = 'cl'
	elif args.os == 'linux':
		c_compiler = 'gcc'
		cpp_compiler = 'g++'
	elif args.os == 'macos':
		c_compiler = 'clang'
		cpp_compiler = 'clang++'

	# Configure CMake
	cmake_command = f'cmake -B {build_output_dir} -DCMAKE_CXX_COMPILER={cpp_compiler} -DCMAKE_C_COMPILER={c_compiler} -DCMAKE_BUILD_TYPE={args.build_type} -S {os.getcwd()}'
	if args.os == 'macos':
		cmake_command += ' -G "Xcode"'
	if args.platform:
		cmake_command += f' -DPLATFORM:STRING={args.platform}'
	run_command(cmake_command)

	# Build
	if args.build:
		run_command(f'cmake --build {build_output_dir} --config {args.build_type}')
	else:
		exit(0)

	# Test
	if args.test:
		run_command(f'ctest --build-config {args.build_type} --verbose --output-on-failure', cwd=build_output_dir)


	# Package Build Artifacts
	package_dir = os.path.join(build_output_dir, 'package')
	os.makedirs(package_dir, exist_ok=True)
	files_to_copy = glob.glob(f'{build_output_dir}/{args.build_type}/*GameAnalytics.*')
	for file in files_to_copy:
		shutil.copy(file, package_dir)
	shutil.copytree(os.path.join(os.getcwd(), 'include'), os.path.join(package_dir, 'include'), dirs_exist_ok=True)

	# Print Package Contents
	if args.os == 'windows':
		run_command(f'dir {package_dir}', shell=True)
	else:
		run_command(f'ls -la {package_dir}', shell=True)

	# Print architecture information
	#use lipo on macos and linux and dumpbin on windows
	if args.os == 'macos':
		run_command(f'lipo -info {package_dir}/*GameAnalytics.*')
	elif args.os == 'linux':
		run_command(f'file {package_dir}/*GameAnalytics.*')
	elif args.os == 'windows':
		run_command(f'dumpbin /headers {package_dir}/GameAnalytics.lib | findstr machine')
	

if __name__ == "__main__":
	main()