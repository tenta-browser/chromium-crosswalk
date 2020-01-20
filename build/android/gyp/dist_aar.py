#!/usr/bin/env python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Creates an Android .aar file."""

import argparse
import collections
import os
import posixpath
import shutil
import sys
import tempfile
import zipfile

from filter_zip import CreatePathTransform
from util import build_utils


_ANDROID_BUILD_DIR = os.path.dirname(os.path.dirname(__file__))


def _RenumberRTxt(lines):
  """Eliminates duplicates and renumbers R.txt lines"""
  all_resources = collections.defaultdict(dict)
  # Parse lines
  for line in lines:
    jtype, rtype, name, id = line.strip().split(' ', 3)
    all_resources[(jtype, rtype)][name] = id
  # Renumber simple int types
  for tid, typ in enumerate(sorted(all_resources.keys()), start=1):
    _, rtype = typ
    if rtype == 'styleable':
      continue
    resources = all_resources[typ]
    for rid, name in enumerate(sorted(resources.keys())):
      resources[name] = hex((0x7f00 + tid) << 16 | rid)            
  # Renumber int[] styleables
  #  - lookup their attribute references in 'int styleable' 
  #    (a trie would help greatly here)
  #  - resolve the attribute references' id from 'int attr'
  styl_resources = all_resources[('int[]', 'styleable')]
  styl_attr_items = list(all_resources[('int', 'styleable')].items())
  attr_resources = all_resources[('int', 'attr')]
  for name in styl_resources:
    name_ = name + '_'
    attrs = []
    for anme, anr in styl_attr_items:
      if anme.startswith(name_):
        anr = int(anr)
        anme = anme[len(name_):]
        attrs.append((anr, anme))
    aids = []                
    for _, aname in sorted(attrs):
      aid = attr_resources[aname]
      aids.append(aid)
    styl_resources[name] = '{ ' + ', '.join(aids) + ' }'
  # Generate renumbered lines
  relines = []
  for typ in sorted(all_resources.keys()):
    jtype, rtype = typ
    resources = all_resources[typ]
    for name in sorted(resources.keys()):
      reline = '{} {} {} {}\n'.format(jtype, rtype, name, resources[name])
      relines.append(reline)
  return relines


def _MergeRTxt(r_paths, include_globs, renumber):
  """Merging the given R.txt files and returns them as a string."""
  all_lines = set()
  for r_path in r_paths:
    if include_globs and not build_utils.MatchesGlob(r_path, include_globs):
      continue
    with open(r_path) as f:
      all_lines.update(f.readlines())
  if renumber:
    all_lines = _RenumberRTxt(all_lines)
  return ''.join(sorted(all_lines))


def _MergeProguardConfigs(proguard_configs):
  """Merging the given proguard config files and returns them as a string."""
  ret = []
  for config in proguard_configs:
    ret.append('# FROM: {}'.format(config))
    with open(config) as f:
      ret.append(f.read())
  return '\n'.join(ret)


def _AddResources(aar_zip, resource_zips, include_globs):
  """Adds all resource zips to the given aar_zip.

  Ensures all res/values/* files have unique names by prefixing them.
  """
  for i, path in enumerate(resource_zips):
    if include_globs and not build_utils.MatchesGlob(path, include_globs):
      continue
    with zipfile.ZipFile(path) as res_zip:
      for info in res_zip.infolist():
        data = res_zip.read(info)
        dirname, basename = posixpath.split(info.filename)
        if 'values' in dirname:
          root, ext = os.path.splitext(basename)
          basename = '{}_{}{}'.format(root, i, ext)
          info.filename = posixpath.join(dirname, basename)
        info.filename = posixpath.join('res', info.filename)
        aar_zip.writestr(info, data)


def main(args):
  args = build_utils.ExpandFileArgs(args)
  parser = argparse.ArgumentParser()
  build_utils.AddDepfileOption(parser)
  parser.add_argument('--output', required=True, help='Path to output aar.')
  parser.add_argument('--jars', required=True, help='GN list of jar inputs.')
  parser.add_argument('--dependencies-res-zips', required=True,
                      help='GN list of resource zips')
  parser.add_argument('--r-text-files', required=True,
                      help='GN list of R.txt files to merge')
  parser.add_argument('--r-text-renumber', action='store_true', dest='r_text_renumber',
                      help='Enables R.txt deduplication and renumbering')
  parser.add_argument('--proguard-configs', required=True,
                      help='GN list of ProGuard flag files to merge.')
  parser.add_argument('--assets', required=True,
                      help='GN list of asset files')
  parser.add_argument(
      '--android-manifest',
      help='Path to AndroidManifest.xml to include.',
      default=os.path.join(_ANDROID_BUILD_DIR, 'AndroidManifest.xml'))
  parser.add_argument('--native-libraries', default='',
                      help='GN list of native libraries. If non-empty then '
                      'ABI must be specified.')
  parser.add_argument('--abi',
                      help='ABI (e.g. armeabi-v7a) for native libraries.')
  parser.add_argument(
      '--jar-excluded-globs',
      help='GN-list of globs for paths to exclude in jar.')
  parser.add_argument(
      '--jar-included-globs',
      help='GN-list of globs for paths to include in jar.')
  parser.add_argument(
      '--resource-included-globs',
      help='GN-list of globs for paths to include in R.txt and resources zips.')
  parser.set_defaults(r_text_renumber=False)

  options = parser.parse_args(args)

  if options.native_libraries and not options.abi:
    parser.error('You must provide --abi if you have native libs')

  options.jars = build_utils.ParseGnList(options.jars)
  options.dependencies_res_zips = build_utils.ParseGnList(
      options.dependencies_res_zips)
  options.r_text_files = build_utils.ParseGnList(options.r_text_files)
  options.proguard_configs = build_utils.ParseGnList(options.proguard_configs)
  options.assets = build_utils.ParseGnList(options.assets)
  options.native_libraries = build_utils.ParseGnList(options.native_libraries)
  options.jar_excluded_globs = build_utils.ParseGnList(
      options.jar_excluded_globs)
  options.jar_included_globs = build_utils.ParseGnList(
      options.jar_included_globs)
  options.resource_included_globs = build_utils.ParseGnList(
      options.resource_included_globs)

  with tempfile.NamedTemporaryFile(delete=False) as staging_file:
    try:
      with zipfile.ZipFile(staging_file.name, 'w', compression=zipfile.ZIP_DEFLATED) as z:
        build_utils.AddToZipHermetic(
            z, 'AndroidManifest.xml', src_path=options.android_manifest)

        path_transform = CreatePathTransform(options.jar_excluded_globs,
                                             options.jar_included_globs, [])
        with tempfile.NamedTemporaryFile() as jar_file:
          build_utils.MergeZips(
              jar_file.name, options.jars, path_transform=path_transform)
          build_utils.AddToZipHermetic(z, 'classes.jar', src_path=jar_file.name)

        build_utils.AddToZipHermetic(
            z,
            'R.txt',
            data=_MergeRTxt(options.r_text_files,
                            options.resource_included_globs,
                            options.r_text_renumber))
        build_utils.AddToZipHermetic(z, 'public.txt', data='')

        if options.proguard_configs:
          build_utils.AddToZipHermetic(
              z, 'proguard.txt',
              data=_MergeProguardConfigs(options.proguard_configs))

        _AddResources(z, options.dependencies_res_zips,
                      options.resource_included_globs)

        for native_library in options.native_libraries:
          libname = os.path.basename(native_library)
          build_utils.AddToZipHermetic(
              z, os.path.join('jni', options.abi, libname),
              src_path=native_library)

        assets = []
        for asset in options.assets:
          ext_path, int_path = asset.split(':', 2)
          assets.append(ext_path)
          build_utils.AddToZipHermetic(z, 'assets/' + int_path, src_path=ext_path)
    except:
      os.unlink(staging_file.name)
      raise
    shutil.move(staging_file.name, options.output)

  if options.depfile:
    all_inputs = (options.jars + options.dependencies_res_zips +
                  options.r_text_files + options.proguard_configs + 
                  options.native_libraries + assets)
    build_utils.WriteDepfile(options.depfile, options.output, all_inputs,
                             add_pydeps=False)


if __name__ == '__main__':
  main(sys.argv[1:])
