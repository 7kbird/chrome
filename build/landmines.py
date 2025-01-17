#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This script runs every build as the first hook (See DEPS). If it detects that
the build should be clobbered, it will remove the build directory.

A landmine is tripped when a builder checks out a different revision, and the
diff between the new landmines and the old ones is non-null. At this point, the
build is clobbered.
"""

import difflib
import errno
import gyp_environment
import logging
import optparse
import os
import shutil
import sys
import subprocess
import time

import landmine_utils


SRC_DIR = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))


def get_build_dir(build_tool, is_iphone=False):
  """
  Returns output directory absolute path dependent on build and targets.
  Examples:
    r'c:\b\build\slave\win\build\src\out'
    '/mnt/data/b/build/slave/linux/build/src/out'
    '/b/build/slave/ios_rel_device/build/src/xcodebuild'

  Keep this function in sync with tools/build/scripts/slave/compile.py
  """
  ret = None
  if build_tool == 'xcode':
    ret = os.path.join(SRC_DIR, 'xcodebuild')
  elif build_tool in ['make', 'ninja', 'ninja-ios']:  # TODO: Remove ninja-ios.
    ret = os.path.join(SRC_DIR, os.environ.get('CHROMIUM_OUT_DIR', 'out'))
  elif build_tool in ['msvs', 'vs', 'ib']:
    ret = os.path.join(SRC_DIR, 'build')
  else:
    raise NotImplementedError('Unexpected GYP_GENERATORS (%s)' % build_tool)
  return os.path.abspath(ret)


def clobber_if_necessary(new_landmines):
  """Does the work of setting, planting, and triggering landmines."""
  out_dir = get_build_dir(landmine_utils.builder())
  landmines_path = os.path.normpath(os.path.join(out_dir, '..', '.landmines'))
  try:
    os.makedirs(out_dir)
  except OSError as e:
    if e.errno == errno.EEXIST:
      pass

  if os.path.exists(landmines_path):
    with open(landmines_path, 'r') as f:
      old_landmines = f.readlines()
    if old_landmines != new_landmines:
      old_date = time.ctime(os.stat(landmines_path).st_ctime)
      diff = difflib.unified_diff(old_landmines, new_landmines,
          fromfile='old_landmines', tofile='new_landmines',
          fromfiledate=old_date, tofiledate=time.ctime(), n=0)
      sys.stdout.write('Clobbering due to:\n')
      sys.stdout.writelines(diff)

      # Clobber.
      shutil.rmtree(out_dir)

  # Save current set of landmines for next time.
  with open(landmines_path, 'w') as f:
    f.writelines(new_landmines)


def process_options():
  """Returns a list of landmine emitting scripts."""
  parser = optparse.OptionParser()
  parser.add_option(
      '-s', '--landmine-scripts', action='append',
      default=[os.path.join(SRC_DIR, 'build', 'get_landmines.py')],
      help='Path to the script which emits landmines to stdout. The target '
           'is passed to this script via option -t. Note that an extra '
           'script can be specified via an env var EXTRA_LANDMINES_SCRIPT.')
  parser.add_option('-v', '--verbose', action='store_true',
      default=('LANDMINES_VERBOSE' in os.environ),
      help=('Emit some extra debugging information (default off). This option '
          'is also enabled by the presence of a LANDMINES_VERBOSE environment '
          'variable.'))

  options, args = parser.parse_args()

  if args:
    parser.error('Unknown arguments %s' % args)

  logging.basicConfig(
      level=logging.DEBUG if options.verbose else logging.ERROR)

  extra_script = os.environ.get('EXTRA_LANDMINES_SCRIPT')
  if extra_script:
    return options.landmine_scripts + [extra_script]
  else:
    return options.landmine_scripts


def main():
  landmine_scripts = process_options()

  if landmine_utils.builder() in ('dump_dependency_json', 'eclipse'):
    return 0

  gyp_environment.SetEnvironment()

  landmines = []
  for s in landmine_scripts:
    proc = subprocess.Popen([sys.executable, s], stdout=subprocess.PIPE)
    output, _ = proc.communicate()
    landmines.extend([('%s\n' % l.strip()) for l in output.splitlines()])
  clobber_if_necessary(landmines)

  return 0


if __name__ == '__main__':
  sys.exit(main())
