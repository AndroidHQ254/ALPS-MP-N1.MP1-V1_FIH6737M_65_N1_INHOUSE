#!/usr/bin/env python
#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys

if sys.hexversion < 0x02070000:
  print >> sys.stderr, "Python 2.7 or newer is required."
  sys.exit(1)

import os
import zipfile
import common
import edify_generator

mtStageFile="/cache/recovery/last_mtupdate_stage"
part_dev_map = {
  "preloader"  : "/dev/block/mmcblk0boot0",
  "preloader2" : "/dev/block/mmcblk0boot1"
}

def AddOTA_Items(input_zip, output_zip, isFullOTA):
  common.ZipWriteStr(output_zip, "type.txt", str(isFullOTA))
  ota_scatter = input_zip.read("OTA/ota_scatter.txt")
  common.ZipWriteStr(output_zip, "scatter.txt", ota_scatter)

def SwitchActive(script, from_part, to_part):
  """switch current active partition."""
  script.AppendExtra(('switch_active("%(partition)s", "%(to_part)s");')
      % {'partition':from_part.replace("bootloader","lk"), 'to_part':to_part.replace("bootloader","lk")})

def SwitchStage(script, stage_str):
  script.AppendExtra('set_mtupdate_stage("%(stagefile)s", "%(stage_str)s");' % {'stagefile':mtStageFile, 'stage_str':stage_str})

def WriteRawImage2(script, partition, fn, info_dict, mapfn=None):
  """Write the given package file into the given MTD partition."""
  if partition in part_dev_map.keys():
    partition_type = "EMMC"
    if script.fstab:
      try:
        p = script.fstab["/boot"]
      except:
        print "%s not exists in fstab, try fstab of info_dict" % mount_point
        p = info_dict["fstab"]["/boot"]
      partition_type = common.PARTITION_TYPES[p.fs_type]
      args = {'device': p.device, 'fn': fn}

    if partition_type == "MTD":
      script.AppendExtra(
          'write_raw_image(package_extract_file("%(fn)s"), "/dev/preloader");'
          % args)
    else:
      script.AppendExtra(
        ('assert(package_extract_file("%(fn)s", "/tmp/%(fn)s"),\n'
         '       write_raw_image("/tmp/%(fn)s", "%(partition)s"),\n'
         '       delete("/tmp/%(fn)s"));')
        % {'partition': part_dev_map[partition], 'fn': fn})
  else:
    mount_point = "/"+partition
    if script.fstab:
      try:
        p = script.fstab[mount_point]
      except:
        print "%s not exists in fstab, try fstab of info_dict" % mount_point
        p = info_dict["fstab"][mount_point]
      partition_type = common.PARTITION_TYPES[p.fs_type]
      args = {'device': p.device, 'fn': fn}
      if partition_type == "MTD":
        script.AppendExtra(
            'write_raw_image(package_extract_file("%(fn)s"), "%(device)s");'
            % args)
      elif partition_type == "EMMC":
        if mapfn:
          args["map"] = mapfn
          script.AppendExtra(
              'package_extract_file("%(fn)s", "%(device)s", "%(map)s");' % args)
        else:
          script.AppendExtra(
              'package_extract_file("%(fn)s", "%(device)s");' % args)
      else:
        raise ValueError(
            "don't know how to write \"%s\" partitions" % p.fs_type)

def AddOTAImage_Items(input_zip, output_zip, info_dict, script):
  try:
    output = input_zip.read("OTA/ota_update_list.txt")
  except:
    print "update_img_list not found"
    return

  storage_type="EMMC"
  td_pair = common.GetTypeAndDevice("/boot", info_dict)
  if not td_pair:
    return
  storage_type = td_pair[0]

  isBackupImgExist = 0
  isFirstRun = 0
  part_list = []

  general_img_list = []
  loader_img_list = []
  for line in output.split("\n"):
    if not line: continue
    columns = line.split()
    try:
      img_read = input_zip.read("IMAGES/%s" % columns[0])
    except:
      print "read image %s fail, remove from update list" % columns[0]
      continue
    common.ZipWriteStr(output_zip, columns[0], img_read)
    if len(columns) == 2:
      general_img_list.append(columns[:2])
    elif len(columns) == 3:
      loader_img_list.append(columns[:3])
    else:
      print "incorrect format in ota_update_list.txt"
      return

  script.AppendExtra('show_mtupdate_stage("%s");' % mtStageFile)

  for img_name, mount_point in general_img_list:
    if general_img_list.index([img_name, mount_point]) == 0:
      script.AppendExtra('ifelse (\nless_than_int(get_mtupdate_stage("%s"), "1") ,\n(' % mtStageFile)
      script.AppendExtra('ui_print("start to update general image");');
    WriteRawImage2(script, mount_point, img_name, info_dict)
  if len(general_img_list) > 0:
    SwitchStage(script, "1")
    script.AppendExtra('),\nui_print("general images are already updated");\n);')

  if len(loader_img_list) > 0:
    for img_name, mount_point, backup_mount_point in loader_img_list:
      if loader_img_list.index([img_name, mount_point, backup_mount_point]) == 0:
        script.AppendExtra('ifelse (\nless_than_int(get_mtupdate_stage("%s"), "3") ,\n(' % mtStageFile)
        script.AppendExtra('if less_than_int(get_mtupdate_stage("%s"), "2") then\n' % mtStageFile)
        script.AppendExtra('ui_print("start to update alt loader image");');
      WriteRawImage2(script, backup_mount_point, img_name, info_dict)
    SwitchStage(script, "2")
    script.AppendExtra('endif;\n')
    for img_name, mount_point, backup_mount_point in loader_img_list:
      SwitchActive(script, mount_point, backup_mount_point)
    SwitchStage(script, "3")
    script.AppendExtra('),\nui_print("alt loder images are already updated");\n);')

    for img_name, mount_point, backup_mount_point in loader_img_list:
      if loader_img_list.index([img_name, mount_point, backup_mount_point]) == 0:
        script.AppendExtra('ifelse (\nless_than_int(get_mtupdate_stage("%s"), "5") ,\n(' % mtStageFile)
        script.AppendExtra('if less_than_int(get_mtupdate_stage("%s"), "4") then\n' % mtStageFile)
        script.AppendExtra('ui_print("start to update main loader image");');
      WriteRawImage2(script, mount_point, img_name, info_dict)
    SwitchStage(script, "4")
    script.AppendExtra('endif;\n')
    for img_name, mount_point, backup_mount_point in loader_img_list:
      SwitchActive(script, backup_mount_point, mount_point)
    script.AppendExtra('),\nui_print("main loader images are already updated");\n);')

  script.AppendExtra('delete("%s");' % mtStageFile)

def FullOTA_InstallEnd(self):
  input_zip = self.input_zip
  input_version= self.input_version
  output_zip = self.output_zip
  script = self.script
  input_tmp = self.input_tmp
  metadata = self.metadata
  info_dict = self.info_dict

  # add OTA information
  AddOTA_Items(input_zip, output_zip, 1)

  # add extra images to upgrade
  AddOTAImage_Items(input_zip, output_zip, info_dict, script)

def IncrementalOTA_InstallEnd(self):
  script = self.script
  source_zip = self.source_zip
  source_version = self.source_version
  target_zip = self.target_zip
  target_version = self.target_version
  output_zip = self.output_zip
  script = self.script
  metadata = self.metadata
  info_dict= self.info_dict

  tgt_info_dict = common.LoadInfoDict(target_zip)

  # add OTA information
  AddOTA_Items(target_zip, output_zip, 0)

  # add extra images to upgrade
  AddOTAImage_Items(target_zip, output_zip, tgt_info_dict, script)
