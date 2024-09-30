import os
import sys
import argparse
from subprocess import Popen
from subprocess import STDOUT

platforms = ['linux_x64', 'linux_x86', 'osx', 'win32', 'win64', 'uwp']

def is_valid_platform(arg : str):

    if not len(arg):
        return False

    for p in platforms:
        if p == arg:
            return True
        
    return False

def run_process(args):
    print('Running process:' + str(args))
    proc = Popen(args, stderr=STDOUT)
    proc.wait()

if __name__ == "__main__":

    parser = argparse.ArgumentParser(prog='GA Build', 
                                     description='Build GA for the desired platform',
                                     usage=f'build.py --platform={platforms}')
    
    parser.add_argument('-p', '--platform')
    args = parser.parse_args()

    platform = args.platform.strip()

    print(platform)

    if not is_valid_platform(platform):
        print('Invalid argument', parser.usage)
        exit(1)

    os.makedirs('build', exist_ok=True)
    os.chdir('build')

    run_process(['cmake', '..', f'-DPLATFORM:STRING={platform}'])

