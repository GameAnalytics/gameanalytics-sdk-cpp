import argparse
import os
import subprocess
import shutil
import glob
import platform as plat

def run_command(command, shell=True, cwd=None):
	if os.name == 'nt':  # Check if the OS is Windows
		command = f'powershell.exe -Command "{command}"'
  
	result = subprocess.run(command, shell=shell, check=True, text=True, cwd=cwd)
	return result

def get_compiler_for_platform(platform, compiler=None):
	"""Get compiler configuration for a platform (single compiler, matching cmake.yml behavior)"""
	if platform == 'osx':
		return {'c': 'clang', 'cxx': 'clang++'}
	elif platform.startswith('win'):
		return {'c': 'cl', 'cxx': 'cl'}
	elif platform.startswith('linux'):
		if compiler == 'gcc':
			return {'c': 'gcc', 'cxx': 'g++'}
		else:
			# Default to Clang for Linux
			return {'c': 'clang', 'cxx': 'clang++'}
	return {}

def main():
	parser = argparse.ArgumentParser(description="CMake Build and Test Script")
	parser.add_argument('--platform', choices=['linux_x64', 'linux_x86', 'osx', 'win32', 'win64', 'uwp'], help='Platform to build for', required=True)
	parser.add_argument('--cfg', default='Debug', choices=['Release', 'Debug'], help='Configuration Type')
	parser.add_argument('--compiler', choices=['gcc', 'clang'], help='Compiler to use (Linux only: gcc or clang, default=clang)')
	parser.add_argument('--shared', action='store_true', help='Build shared library instead of static')
	parser.add_argument('--build', action='store_true', help='Execute the build step')
	parser.add_argument('--test', action='store_true', help='Execute the test step')
	parser.add_argument('--coverage', action='store_true', help='Generate code coverage report')
 
	args = parser.parse_args()

	# Validate compiler argument is only used with Linux
	if args.compiler and not args.platform.startswith('linux'):
		parser.error('--compiler can only be used with Linux platforms')

	# Validate coverage is not used with shared library
	if args.coverage and args.shared:
		parser.error('--coverage cannot be used with --shared (coverage requires tests which need static library)')

	# Get compiler configuration for this platform (single compiler, like cmake.yml)
	compiler_config = get_compiler_for_platform(args.platform, args.compiler)
	c_compiler = compiler_config.get('c', '')
	cxx_compiler = compiler_config.get('cxx', '')
	compiler_name = c_compiler if c_compiler else 'default'
	
	lib_type = 'shared' if args.shared else 'static'
	print(f"\n{'='*60}")
	print(f"Building {lib_type} library for {args.platform} with {compiler_name}")
	print(f"{'='*60}\n")
	
	# Always use 'build/' directory (matching cmake.yml behavior)
	build_output_dir = os.path.join(os.getcwd(), 'build')
	os.makedirs(build_output_dir, exist_ok=True)

	# Configure
	cmake_command = f'cmake -B {build_output_dir} -S {os.getcwd()}'
	
	# Add compiler flags
	if c_compiler:
		cmake_command += f' -DCMAKE_C_COMPILER={c_compiler}'
	if cxx_compiler:
		cmake_command += f' -DCMAKE_CXX_COMPILER={cxx_compiler}'
	
	# Configure for shared library build
	if args.shared:
		cmake_command += ' -DGA_SHARED_LIB=ON'
	
	if args.platform == 'osx':
		cmake_command += ' -G "Xcode"'
	
	# Add build type for single-config generators (Linux uses Makefile/Ninja)
	# Multi-config generators (Xcode, Visual Studio) use --config at build time instead
	if args.platform.startswith('linux'):
		cmake_command += f' -DCMAKE_BUILD_TYPE={args.cfg}'
	if args.platform:
		cmake_command += f' -DPLATFORM:STRING={args.platform}'
	if args.coverage:
		cmake_command += ' -DENABLE_COVERAGE=ON'
  
	run_command(cmake_command)

	# Build
	if args.build:
		run_command(f'cmake --build {build_output_dir} --config {args.cfg} --verbose')
	else:
		exit(0)

	# Test
	if args.test:
		run_command(f'ctest --build-config {args.cfg} --verbose --output-on-failure', cwd=build_output_dir)
	else:
		exit(0)

	# Code Coverage
	if args.coverage:
		# Prepare coverage data
		run_command(f'cmake --build {build_output_dir} --target cov', cwd=build_output_dir)

	# Package Build Artifacts
	package_dir = os.path.join(build_output_dir, 'package')
	os.makedirs(package_dir, exist_ok=True)
	files_to_copy = glob.glob(f'{build_output_dir}/{args.cfg}/*GameAnalytics.*')
	for file in files_to_copy:
		shutil.copy(file, package_dir)
	
	# Copy include directory
	shutil.copytree(os.path.join(os.getcwd(), 'include'), os.path.join(package_dir, 'include'), dirs_exist_ok=True)
	
	# For shared library builds, also copy the C API header
	if args.shared:
		extern_header = os.path.join(os.getcwd(), 'source', 'gameanalytics', 'GameAnalyticsExtern.h')
		shutil.copy(extern_header, os.path.join(package_dir, 'include', 'GameAnalytics'))

	# Print Package Contents
	if args.platform.startswith('win'):
		run_command(f'dir {package_dir}', shell=True)
	else:
		run_command(f'ls -la {package_dir}', shell=True)

	if args.platform == 'osx':
		run_command(f'lipo -info {package_dir}/*GameAnalytics.*')
	
	print(f"\n[OK] {lib_type.capitalize()} library build completed for {args.platform} with {compiler_name}")
	print(f"  Package location: {package_dir}\n")

if __name__ == "__main__":
	main()
