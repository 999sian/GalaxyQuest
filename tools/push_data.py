#!/usr/bin/env python3
"""Copies the converted game files to the headset, where the app reads them.

  python tools/push_data.py [cooked dir]        (default: cooked)

Install the APK first.  The headset must be connected over USB with
developer mode on, and adb (Android platform-tools) on PATH or in ADB.
The files go to /sdcard/Android/data/com.galaxy.quest/files/game (about
3.3 GB).  adb cannot create folders under Android/data one by one, so they
go over as one tar file that the headset unpacks.
"""
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile

PACKAGE = 'com.galaxy.quest'
DEST = '/sdcard/Android/data/%s/files/game' % PACKAGE
TMP = '/data/local/tmp/petari_game_data.tar'


def find_adb():
    if os.environ.get('ADB'):
        return os.environ['ADB']
    adb = shutil.which('adb')
    if adb:
        return adb
    for sdk in (os.environ.get('ANDROID_HOME'), os.environ.get('ANDROID_SDK_ROOT'),
                os.path.join(os.environ.get('LOCALAPPDATA', ''), 'Android', 'Sdk'),
                os.path.expanduser('~/Android/Sdk'), os.path.expanduser('~/Library/Android/sdk')):
        if sdk:
            for name in ('adb.exe', 'adb'):
                path = os.path.join(sdk, 'platform-tools', name)
                if os.path.isfile(path):
                    return path
    sys.exit('adb not found: install the Android platform-tools and put adb on PATH (or set ADB)')


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else 'cooked'
    if not os.path.isfile(os.path.join(src, 'sys', 'fst.bin')):
        sys.exit('%s does not look like converted game files (no sys/fst.bin): run tools/cook/cook.py first' % src)
    adb = find_adb()

    def run(*args):
        subprocess.run([adb] + list(args), check=True)

    devices = subprocess.run([adb, 'devices'], check=True, capture_output=True, text=True).stdout.split('\n')[1:]
    if not any(line.strip().endswith('device') for line in devices):
        sys.exit('no headset found: connect it over USB, allow USB debugging in the headset, and try again')

    with tempfile.TemporaryDirectory() as tmp:
        tar_path = os.path.join(tmp, 'game_data.tar')
        print('packing %s' % src, flush=True)
        with tarfile.open(tar_path, 'w') as tar:
            for name in sorted(os.listdir(src)):
                tar.add(os.path.join(src, name), arcname=name)
        print('copying to the headset (%.1f GB)' % (os.path.getsize(tar_path) / 1e9), flush=True)
        run('push', tar_path, TMP)
    print('unpacking in %s' % DEST, flush=True)
    # The files belong to adb's shell user and the folder is group-only: the
    # app, under its own user, reads them through "other".
    run('shell', 'mkdir -p %s && cd %s && tar xf %s && rm %s && chmod -R a+rX %s' % (DEST, DEST, TMP, TMP, DEST))
    print('done: the game files are in %s' % DEST)


if __name__ == '__main__':
    main()
