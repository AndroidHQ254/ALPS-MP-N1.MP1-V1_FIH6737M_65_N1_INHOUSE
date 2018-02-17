#!/usr/bin/env python
#
# Copyright (C) 2011 The Android Open Source Project
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

"""
Build image output_image_file from input_directory and properties_file.

Usage:  build_image input_directory properties_file output_image_file

"""
import os
import os.path
import re
import subprocess
import sys
import commands
import common
import shutil
import sparse_img
import tempfile
import shutil

OPTIONS = common.OPTIONS

FIXED_SALT = "aee087a5be3b982978c923f566a94613496b417f2af592639bc80d141e34dfe7"
BLOCK_SIZE = 4096

def RunCommand(cmd):
  """Echo and run the given command.

  Args:
    cmd: the command represented as a list of strings.
  Returns:
    A tuple of the output and the exit code.
  """
  print "Running: ", " ".join(cmd)
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  output, _ = p.communicate()
  print "%s" % (output.rstrip(),)
  return (output, p.returncode)

def GetVerityFECSize(partition_size):
  cmd = "fec -s %d" % partition_size
  status, output = commands.getstatusoutput(cmd)
  if status:
    print output
    return False, 0
  return True, int(output)

def GetVerityTreeSize(partition_size):
  cmd = "build_verity_tree -s %d"
  cmd %= partition_size
  status, output = commands.getstatusoutput(cmd)
  if status:
    print output
    return False, 0
  return True, int(output)

def GetVerityMetadataSize(partition_size):
  cmd = "system/extras/verity/build_verity_metadata.py -s %d"
  cmd %= partition_size

  status, output = commands.getstatusoutput(cmd)
  if status:
    print output
    return False, 0
  return True, int(output)

def GetVeritySize(partition_size, fec_supported):
  success, verity_tree_size = GetVerityTreeSize(partition_size)
  if not success:
    return 0
  success, verity_metadata_size = GetVerityMetadataSize(partition_size)
  if not success:
    return 0
  verity_size = verity_tree_size + verity_metadata_size
  if fec_supported:
    success, fec_size = GetVerityFECSize(partition_size + verity_size)
    if not success:
      return 0
    return verity_size + fec_size
  return verity_size

def GetSimgSize(image_file):
  simg = sparse_img.SparseImage(image_file, build_map=False)
  return simg.blocksize * simg.total_blocks

def ZeroPadSimg(image_file, pad_size):
  blocks = pad_size // BLOCK_SIZE
  print("Padding %d blocks (%d bytes)" % (blocks, pad_size))
  simg = sparse_img.SparseImage(image_file, mode="r+b", build_map=False)
  simg.AppendFillChunk(0, blocks)

def AdjustPartitionSizeForVerity(partition_size, fec_supported):
  """Modifies the provided partition size to account for the verity metadata.

  This information is used to size the created image appropriately.
  Args:
    partition_size: the size of the partition to be verified.
  Returns:
    The size of the partition adjusted for verity metadata.
  """
  key = "%d %d" % (partition_size, fec_supported)
  if key in AdjustPartitionSizeForVerity.results:
    return AdjustPartitionSizeForVerity.results[key]

  hi = partition_size
  if hi % BLOCK_SIZE != 0:
    hi = (hi // BLOCK_SIZE) * BLOCK_SIZE

  # verity tree and fec sizes depend on the partition size, which
  # means this estimate is always going to be unnecessarily small
  lo = partition_size - GetVeritySize(hi, fec_supported)
  result = lo

  # do a binary search for the optimal size
  while lo < hi:
    i = ((lo + hi) // (2 * BLOCK_SIZE)) * BLOCK_SIZE
    size = i + GetVeritySize(i, fec_supported)
    if size <= partition_size:
      if result < i:
        result = i
      lo = i + BLOCK_SIZE
    else:
      hi = i

  AdjustPartitionSizeForVerity.results[key] = result
  return result

AdjustPartitionSizeForVerity.results = {}

def BuildVerityFEC(sparse_image_path, verity_path, verity_fec_path):
  cmd = "fec -e %s %s %s" % (sparse_image_path, verity_path, verity_fec_path)
  print cmd
  status, output = commands.getstatusoutput(cmd)
  if status:
    print "Could not build FEC data! Error: %s" % output
    return False
  return True

def BuildVerityTree(sparse_image_path, verity_image_path, prop_dict):
  cmd = "build_verity_tree -A %s %s %s" % (
      FIXED_SALT, sparse_image_path, verity_image_path)
  print cmd
  status, output = commands.getstatusoutput(cmd)
  if status:
    print "Could not build verity tree! Error: %s" % output
    return False
  root, salt = output.split()
  prop_dict["verity_root_hash"] = root
  prop_dict["verity_salt"] = salt
  return True

def BuildVerityMetadata(image_size, verity_metadata_path, root_hash, salt,
                        block_device, signer_path, key):
  cmd_template = (
      "system/extras/verity/build_verity_metadata.py %s %s %s %s %s %s %s")
  cmd = cmd_template % (image_size, verity_metadata_path, root_hash, salt,
                        block_device, signer_path, key)
  print cmd
  status, output = commands.getstatusoutput(cmd)
  if status:
    print "Could not build verity metadata! Error: %s" % output
    return False
  return True

def Append2Simg(sparse_image_path, unsparse_image_path, error_message):
  """Appends the unsparse image to the given sparse image.

  Args:
    sparse_image_path: the path to the (sparse) image
    unsparse_image_path: the path to the (unsparse) image
  Returns:
    True on success, False on failure.
  """
  cmd = "append2simg %s %s"
  cmd %= (sparse_image_path, unsparse_image_path)
  print cmd
  status, output = commands.getstatusoutput(cmd)
  if status:
    print "%s: %s" % (error_message, output)
    return False
  return True

def Append(target, file_to_append, error_message):
  cmd = 'cat %s >> %s' % (file_to_append, target)
  print cmd
  status, output = commands.getstatusoutput(cmd)
  if status:
    print "%s: %s" % (error_message, output)
    return False
  return True

def BuildVerifiedImage(data_image_path, verity_image_path,
                       verity_metadata_path, verity_fec_path,
                       fec_supported):
  if not Append(verity_image_path, verity_metadata_path,
                "Could not append verity metadata!"):
    return False

  if fec_supported:
    # build FEC for the entire partition, including metadata
    if not BuildVerityFEC(data_image_path, verity_image_path,
                          verity_fec_path):
      return False

    if not Append(verity_image_path, verity_fec_path, "Could not append FEC!"):
      return False

  if not Append2Simg(data_image_path, verity_image_path,
                     "Could not append verity data!"):
    return False
  return True

def UnsparseImage(sparse_image_path, replace=True):
  img_dir = os.path.dirname(sparse_image_path)
  unsparse_image_path = "unsparse_" + os.path.basename(sparse_image_path)
  unsparse_image_path = os.path.join(img_dir, unsparse_image_path)
  if os.path.exists(unsparse_image_path):
    if replace:
      os.unlink(unsparse_image_path)
    else:
      return True, unsparse_image_path
  inflate_command = ["simg2img", sparse_image_path, unsparse_image_path]
  (_, exit_code) = RunCommand(inflate_command)
  if exit_code != 0:
    os.remove(unsparse_image_path)
    return False, None
  return True, unsparse_image_path

def MakeVerityEnabledImage(out_file, fec_supported, prop_dict):
  """Creates an image that is verifiable using dm-verity.

  Args:
    out_file: the location to write the verifiable image at
    prop_dict: a dictionary of properties required for image creation and
               verification
  Returns:
    True on success, False otherwise.
  """
  # get properties
  image_size = prop_dict["partition_size"]
  block_dev = prop_dict["verity_block_device"]
  signer_key = prop_dict["verity_key"] + ".pk8"
  if OPTIONS.verity_signer_path is not None:
    signer_path = OPTIONS.verity_signer_path + ' '
    signer_path += ' '.join(OPTIONS.verity_signer_args)
  else:
    signer_path = prop_dict["verity_signer_cmd"]

  # make a tempdir
  tempdir_name = tempfile.mkdtemp(suffix="_verity_images")

  # get partial image paths
  verity_image_path = os.path.join(tempdir_name, "verity.img")
  verity_metadata_path = os.path.join(tempdir_name, "verity_metadata.img")
  verity_fec_path = os.path.join(tempdir_name, "verity_fec.img")

  # build the verity tree and get the root hash and salt
  if not BuildVerityTree(out_file, verity_image_path, prop_dict):
    shutil.rmtree(tempdir_name, ignore_errors=True)
    return False

  # build the metadata blocks
  root_hash = prop_dict["verity_root_hash"]
  salt = prop_dict["verity_salt"]
  if not BuildVerityMetadata(image_size, verity_metadata_path, root_hash, salt,
                             block_dev, signer_path, signer_key):
    shutil.rmtree(tempdir_name, ignore_errors=True)
    return False

  # build the full verified image
  if not BuildVerifiedImage(out_file,
                            verity_image_path,
                            verity_metadata_path,
                            verity_fec_path,
                            fec_supported):
    shutil.rmtree(tempdir_name, ignore_errors=True)
    return False

  shutil.rmtree(tempdir_name, ignore_errors=True)
  return True

def ConvertBlockMapToBaseFs(block_map_file):
  fd, base_fs_file = tempfile.mkstemp(prefix="script_gen_",
                                      suffix=".base_fs")
  os.close(fd)

  convert_command = ["blk_alloc_to_base_fs", block_map_file, base_fs_file]
  (_, exit_code) = RunCommand(convert_command)
  if exit_code != 0:
    os.remove(base_fs_file)
    return None
  return base_fs_file

def BuildImage(in_dir, prop_dict, out_file, target_out=None):
  """Build an image to out_file from in_dir with property prop_dict.

  Args:
    in_dir: path of input directory.
    prop_dict: property dictionary.
    out_file: path of the output image file.
    target_out: path of the product out directory to read device specific FS config files.

  Returns:
    True iff the image is built successfully.
  """
  # system_root_image=true: build a system.img that combines the contents of
  # /system and the ramdisk, and can be mounted at the root of the file system.
  origin_in = in_dir
  fs_config = prop_dict.get("fs_config")
  base_fs_file = None
  if (prop_dict.get("system_root_image") == "true"
      and prop_dict["mount_point"] == "system"):
    in_dir = tempfile.mkdtemp()
    # Change the mount point to "/"
    prop_dict["mount_point"] = "/"
    if fs_config:
      # We need to merge the fs_config files of system and ramdisk.
      fd, merged_fs_config = tempfile.mkstemp(prefix="root_fs_config",
                                              suffix=".txt")
      os.close(fd)
      with open(merged_fs_config, "w") as fw:
        if "ramdisk_fs_config" in prop_dict:
          with open(prop_dict["ramdisk_fs_config"]) as fr:
            fw.writelines(fr.readlines())
        with open(fs_config) as fr:
          fw.writelines(fr.readlines())
      fs_config = merged_fs_config

  build_command = []
  fs_type = prop_dict.get("fs_type", "")
  mtftl_support = prop_dict.get("mtftl_support", "")
  run_fsck = False

  fs_spans_partition = True
  if fs_type.startswith("squash"):
    fs_spans_partition = False

  is_verity_partition = "verity_block_device" in prop_dict
  verity_supported = prop_dict.get("verity") == "true"
  verity_fec_supported = prop_dict.get("verity_fec") == "true"

  # Adjust the partition size to make room for the hashes if this is to be
  # verified.
  if verity_supported and is_verity_partition:
    if mtftl_support.startswith("yes"):
      partition_size = int(prop_dict.get("partition_size")[:-1])*1024*1024    ## ftl for partiton MB
    else:
      partition_size = int(prop_dict.get("partition_size"))

    adjusted_size = AdjustPartitionSizeForVerity(partition_size,
                                                 verity_fec_supported)
    if not adjusted_size:
      return False
    prop_dict["partition_size"] = str(adjusted_size)
    prop_dict["original_partition_size"] = str(partition_size)

  if fs_type.startswith("ext"):
    build_command = ["mkuserimg.sh"]
    if "extfs_sparse_flag" in prop_dict:
      build_command.append(prop_dict["extfs_sparse_flag"])
      run_fsck = True
    build_command.extend([in_dir, out_file, fs_type,
                          prop_dict["mount_point"]])
    build_command.append(prop_dict["partition_size"])
    if "journal_size" in prop_dict:
      build_command.extend(["-j", prop_dict["journal_size"]])
    if "timestamp" in prop_dict:
      build_command.extend(["-T", str(prop_dict["timestamp"])])
    if fs_config:
      build_command.extend(["-C", fs_config])
    if target_out:
      build_command.extend(["-D", target_out])
    if "block_list" in prop_dict:
      build_command.extend(["-B", prop_dict["block_list"]])
    if "base_fs_file" in prop_dict:
      base_fs_file = ConvertBlockMapToBaseFs(prop_dict["base_fs_file"])
      if base_fs_file is None:
        return False
      build_command.extend(["-d", base_fs_file])
    build_command.extend(["-L", prop_dict["mount_point"]])
    if "selinux_fc" in prop_dict:
      build_command.append(prop_dict["selinux_fc"])
  elif fs_type.startswith("squash"):
    build_command = ["mksquashfsimage.sh"]
    build_command.extend([in_dir, out_file])
    if "squashfs_sparse_flag" in prop_dict:
      build_command.extend([prop_dict["squashfs_sparse_flag"]])
    build_command.extend(["-m", prop_dict["mount_point"]])
    if target_out:
      build_command.extend(["-d", target_out])
    if fs_config:
      build_command.extend(["-C", fs_config])
    if "selinux_fc" in prop_dict:
      build_command.extend(["-c", prop_dict["selinux_fc"]])
    if "block_list" in prop_dict:
      build_command.extend(["-B", prop_dict["block_list"]])
    if "squashfs_compressor" in prop_dict:
      build_command.extend(["-z", prop_dict["squashfs_compressor"]])
    if "squashfs_compressor_opt" in prop_dict:
      build_command.extend(["-zo", prop_dict["squashfs_compressor_opt"]])
    if "squashfs_block_size" in prop_dict:
      build_command.extend(["-b", prop_dict["squashfs_block_size"]])
    if "squashfs_disable_4k_align" in prop_dict and prop_dict.get("squashfs_disable_4k_align") == "true":
      build_command.extend(["-a"])
  elif fs_type.startswith("f2fs"):
    build_command = ["mkf2fsuserimg.sh"]
    build_command.extend([out_file, prop_dict["partition_size"]])
  elif fs_type.startswith("ubifs"):
    image_filename = os.path.basename(out_file)
    image_dir = os.path.dirname(out_file)
    if image_filename.startswith("system"):
      prop_dict["tmp_out"] = image_dir + "/ubifs.android.img"
    elif image_filename.startswith("userdata"):
        prop_dict["tmp_out"] = image_dir + "/ubifs.usrdata.img"
    if (prop_dict["ini"].startswith(image_dir) == False):
        prop_dict["tmp_out"] = prop_dict["tmp_out"] + "." + str(os.getpid())
	prop_dict["ubi_ini"] = image_dir + "/" + os.path.basename(prop_dict["ini"] + "." + str(os.getpid()))
	with open(prop_dict["ubi_ini"], "wt") as fout:
    	    with open(prop_dict["ini"], "rt") as fin:
                for line in fin:
		    fout.write(line.replace('.img', '.img.' + str(os.getpid())))
    else:
	prop_dict["ubi_ini"] = image_dir + "/" + os.path.basename(prop_dict["ini"])
    print "tmp_out:" + prop_dict["tmp_out"] + ", ini:" + prop_dict["ubi_ini"]

    build_command = ["mkfs_ubifs"]
    if "ubifs_fixup_flag" in prop_dict:
      build_command.extend(prop_dict["ubifs_fixup_flag"])
    build_command.extend(["-F", "-S", prop_dict["selinux_fc"]])
    build_command.extend(["-r", in_dir])
    build_command.extend(["-o", prop_dict["tmp_out"]])
    build_command.extend(["-m", prop_dict["pagesize"]])
    build_command.extend(["-e", prop_dict["lebsize"]])
    build_command.extend(["-c", prop_dict["leb_cnt"], "-v"])
  else:
    build_command = ["mkyaffs2image", "-f"]
    if prop_dict.get("mkyaffs2_extra_flags", None):
      build_command.extend(prop_dict["mkyaffs2_extra_flags"].split())
    build_command.append(in_dir)
    build_command.append(out_file)
    if "selinux_fc" in prop_dict:
      build_command.append(prop_dict["selinux_fc"])
      build_command.append(prop_dict["mount_point"])

  if in_dir != origin_in:
    # Construct a staging directory of the root file system.
    ramdisk_dir = prop_dict.get("ramdisk_dir")
    if ramdisk_dir:
      shutil.rmtree(in_dir)
      shutil.copytree(ramdisk_dir, in_dir, symlinks=True)
    staging_system = os.path.join(in_dir, "system")
    shutil.rmtree(staging_system, ignore_errors=True)
    shutil.copytree(origin_in, staging_system, symlinks=True)

  reserved_blocks = prop_dict.get("has_ext4_reserved_blocks") == "true"
  ext4fs_output = None

  try:
    if reserved_blocks and fs_type.startswith("ext4"):
      (ext4fs_output, exit_code) = RunCommand(build_command)
    else:
      (_, exit_code) = RunCommand(build_command)
  finally:
    if in_dir != origin_in:
      # Clean up temporary directories and files.
      shutil.rmtree(in_dir, ignore_errors=True)
      if fs_config:
        os.remove(fs_config)
    if base_fs_file is not None:
      os.remove(base_fs_file)
  if exit_code != 0:
    return False

  # Bug: 21522719, 22023465
  # There are some reserved blocks on ext4 FS (lesser of 4096 blocks and 2%).
  # We need to deduct those blocks from the available space, since they are
  # not writable even with root privilege. It only affects devices using
  # file-based OTA and a kernel version of 3.10 or greater (currently just
  # sprout).
  if reserved_blocks and fs_type.startswith("ext4"):
    assert ext4fs_output is not None
    ext4fs_stats = re.compile(
        r'Created filesystem with .* (?P<used_blocks>[0-9]+)/'
        r'(?P<total_blocks>[0-9]+) blocks')
    m = ext4fs_stats.match(ext4fs_output.strip().split('\n')[-1])
    used_blocks = int(m.groupdict().get('used_blocks'))
    total_blocks = int(m.groupdict().get('total_blocks'))
    reserved_blocks = min(4096, int(total_blocks * 0.02))
    adjusted_blocks = total_blocks - reserved_blocks
    if used_blocks > adjusted_blocks:
      mount_point = prop_dict.get("mount_point")
      print("Error: Not enough room on %s (total: %d blocks, used: %d blocks, "
            "reserved: %d blocks, available: %d blocks)" % (
                mount_point, total_blocks, used_blocks, reserved_blocks,
                adjusted_blocks))
      return False

  if not fs_spans_partition:
    mount_point = prop_dict.get("mount_point")
    partition_size = int(prop_dict.get("partition_size"))
    image_size = GetSimgSize(out_file)
    if image_size > partition_size:
      print("Error: %s image size of %d is larger than partition size of "
            "%d" % (mount_point, image_size, partition_size))
      return False
    if verity_supported and is_verity_partition:
      ZeroPadSimg(out_file, partition_size - image_size)

  # create the verified image if this is to be verified
  if verity_supported and is_verity_partition:
    if not MakeVerityEnabledImage(out_file, verity_fec_supported, prop_dict):
      return False

  if fs_type.startswith("ext") and mtftl_support.startswith("yes"):
    image_filename = os.path.basename(out_file)
    image_dir = os.path.dirname(out_file)
    if image_filename.startswith("system"):
      prop_dict["tmp_out"] = image_dir + "/mtftl.android.img"
    elif image_filename.startswith("userdata"):
      prop_dict["tmp_out"] = image_dir + "/mtftl.usrdata.img"
    if (prop_dict["ini"].startswith(image_dir) == False):
        prop_dict["tmp_out"] = prop_dict["tmp_out"] + "." + str(os.getpid())
        prop_dict["ubi_ini"] = image_dir + "/" + os.path.basename(prop_dict["ini"] + "." + str(os.getpid()))
        with open(prop_dict["ubi_ini"], "wt") as fout:
            with open(prop_dict["ini"], "rt") as fin:
                for line in fin:
                    fout.write(line.replace('.img', '.img.' + str(os.getpid())))
    else:
        prop_dict["ubi_ini"] = image_dir + "/" + os.path.basename(prop_dict["ini"])
    print "tmp_out:" + prop_dict["tmp_out"] + ", ini:" + prop_dict["ubi_ini"]
    mkftl_command = ["mkftl"]
    mkftl_command.extend(["-m", prop_dict["pagesize"]])
    mkftl_command.extend(["-e", prop_dict["lebsize"]])
    mkftl_command.extend(["-c", prop_dict["leb_cnt"], "-v"])
    mkftl_command.extend(["-r", out_file])
    mkftl_command.extend(["-o", prop_dict["tmp_out"]])
    (_, exit_code) = RunCommand(mkftl_command)
    if exit_code != 0:
      return False

  if fs_type.startswith("ubifs") or mtftl_support.startswith("yes"):
    ubinize_command = ["ubinize"]
    ubinize_command.extend(["-o", out_file])
    ubinize_command.extend(["-m", prop_dict["pagesize"]])
    ubinize_command.extend(["-p", prop_dict["blocksize"]])
    ubinize_command.extend(["-O", prop_dict["vid_offset"]])
    ubinize_command.extend(["-v", prop_dict["ubi_ini"]])
    (_, exit_code) = RunCommand(ubinize_command)
    if exit_code != 0:
      return False
    if (prop_dict["ini"].startswith(image_dir) == False and os.path.exists(prop_dict["ubi_ini"]) == True):
      os.remove(prop_dict["ubi_ini"])
    if os.path.exists(prop_dict["tmp_out"]):
      os.remove(prop_dict["tmp_out"])

  if run_fsck and prop_dict.get("skip_fsck") != "true":
    success, unsparse_image = UnsparseImage(out_file, replace=False)
    if not success:
      return False

    # Run e2fsck on the inflated image file
    e2fsck_command = ["e2fsck", "-f", "-n", unsparse_image]
    (_, exit_code) = RunCommand(e2fsck_command)

    os.remove(unsparse_image)

  return exit_code == 0

def ParseNum(size_num):
  """Parse a human-friendly size string to size in bytes.

  Args:
    size_num: size string.
  """
  try:
    if size_num.endswith('k') or size_num.endswith('K'):
      num = int(size_num[:-1]) * 1024
    elif size_num.endswith('m') or size_num.endswith('M'):
      num = int(size_num[:-1]) * 1024 * 1024
    elif size_num.endswith('g') or size_num.endswith('G'):
      num = int(size_num[:-1]) * 1024 * 1024 * 1024
    else:
      num = int(size_num)
  except ValueError:
    num = -1

  return num

def ImagePropFromGlobalDict(glob_dict, mount_point):
  """Build an image property dictionary from the global dictionary.

  Args:
    glob_dict: the global dictionary from the build system.
    mount_point: such as "system", "data" etc.
  """
  d = {}

  if "build.prop" in glob_dict:
    bp = glob_dict["build.prop"]
    if "ro.build.date.utc" in bp:
      d["timestamp"] = bp["ro.build.date.utc"]

  def copy_prop(src_p, dest_p):
    if src_p in glob_dict:
      d[dest_p] = str(glob_dict[src_p])

  common_props = (
      "extfs_sparse_flag",
      "squashfs_sparse_flag",
      "mkyaffs2_extra_flags",
      "selinux_fc",
      "skip_fsck",
      "verity",
      "verity_key",
      "verity_signer_cmd",
      "verity_fec"
      )
  for p in common_props:
    copy_prop(p, p)

  d["mount_point"] = mount_point
  if mount_point == "system":
    copy_prop("fs_type", "fs_type")
    # Copy the generic sysetem fs type first, override with specific one if
    # available.
    copy_prop("system_fs_type", "fs_type")
    copy_prop("system_size", "partition_size")
    copy_prop("system_journal_size", "journal_size")
    copy_prop("system_verity_block_device", "verity_block_device")
    copy_prop("system_root_image", "system_root_image")
    copy_prop("ramdisk_dir", "ramdisk_dir")
    copy_prop("ramdisk_fs_config", "ramdisk_fs_config")
    copy_prop("has_ext4_reserved_blocks", "has_ext4_reserved_blocks")
    copy_prop("system_squashfs_compressor", "squashfs_compressor")
    copy_prop("system_squashfs_compressor_opt", "squashfs_compressor_opt")
    copy_prop("system_squashfs_block_size", "squashfs_block_size")
    copy_prop("system_squashfs_disable_4k_align", "squashfs_disable_4k_align")
    copy_prop("system_base_fs_file", "base_fs_file")
    copy_prop("mtftl_support", "mtftl_support")
  elif mount_point == "system_other":
    # We inherit the selinux policies of /system since we contain some of its files.
    d["mount_point"] = "system"
    copy_prop("fs_type", "fs_type")
    copy_prop("system_fs_type", "fs_type")
    copy_prop("system_size", "partition_size")
    copy_prop("system_journal_size", "journal_size")
    copy_prop("system_verity_block_device", "verity_block_device")
    copy_prop("has_ext4_reserved_blocks", "has_ext4_reserved_blocks")
    copy_prop("system_squashfs_compressor", "squashfs_compressor")
    copy_prop("system_squashfs_compressor_opt", "squashfs_compressor_opt")
    copy_prop("system_squashfs_block_size", "squashfs_block_size")
    copy_prop("system_base_fs_file", "base_fs_file")
  elif mount_point == "data":
    # Copy the generic fs type first, override with specific one if available.
    copy_prop("fs_type", "fs_type")
    copy_prop("userdata_fs_type", "fs_type")
    copy_prop("userdata_size", "partition_size")
    copy_prop("mtftl_support", "mtftl_support")
    # Workaround: CTS vm-tests-tf runs out of inode on a small /data partition
    # TODO: Pass the min. inodes value from a Makefile option
    if "partition_size" in d and ParseNum(d["partition_size"]) < (16384 * 4 * 4096):
      # default inodes < 16384 (partition size < 256M)
      d["extfs_inodes"] = str(16384)
  elif mount_point == "cache":
    copy_prop("cache_fs_type", "fs_type")
    copy_prop("cache_size", "partition_size")
  elif mount_point == "vendor":
    copy_prop("vendor_fs_type", "fs_type")
    copy_prop("vendor_size", "partition_size")
    copy_prop("vendor_journal_size", "journal_size")
    copy_prop("vendor_verity_block_device", "verity_block_device")
    copy_prop("has_ext4_reserved_blocks", "has_ext4_reserved_blocks")
    copy_prop("vendor_squashfs_compressor", "squashfs_compressor")
    copy_prop("vendor_squashfs_compressor_opt", "squashfs_compressor_opt")
    copy_prop("vendor_squashfs_block_size", "squashfs_block_size")
    copy_prop("vendor_squashfs_disable_4k_align", "squashfs_disable_4k_align")
    copy_prop("vendor_base_fs_file", "base_fs_file")
  elif mount_point == "oem":
    copy_prop("fs_type", "fs_type")
    copy_prop("oem_size", "partition_size")
    copy_prop("oem_journal_size", "journal_size")
    copy_prop("has_ext4_reserved_blocks", "has_ext4_reserved_blocks")
  elif mount_point == "custom":
    copy_prop("fs_type", "fs_type")
    copy_prop("custom_size", "partition_size")
    copy_prop("custom_verity_block_device", "verity_block_device")

  if "fs_type" in d:
    if d["fs_type"] == "ubifs":
      copy_prop("pagesize", "pagesize")
      copy_prop("vid_offset", "vid_offset")
      copy_prop("lebsize", "lebsize");
      copy_prop("blocksize", "blocksize");
      if mount_point == "system":
        copy_prop("system_cnt", "leb_cnt");
        copy_prop("system_ini", "ini");
      elif mount_point == "data":
        copy_prop("userdata_cnt", "leb_cnt");
        copy_prop("userdata_ini", "ini");
      copy_prop("ubifs_fixup_flag", "ubifs_fixup_flag");
      copy_prop("blocksize", "blocksize");
	
  if "mtftl_support" in d:
    if d["mtftl_support"] == "yes":
      copy_prop("pagesize", "pagesize")
      copy_prop("vid_offset", "vid_offset")
      copy_prop("lebsize", "lebsize");
      copy_prop("blocksize", "blocksize");
      if mount_point == "system":
        copy_prop("system_cnt", "leb_cnt");
        copy_prop("system_ini", "ini");
      elif mount_point == "data":
        copy_prop("userdata_cnt", "leb_cnt");
        copy_prop("userdata_ini", "ini");
      copy_prop("ubifs_fixup_flag", "ubifs_fixup_flag");

  return d


def LoadGlobalDict(filename):
  """Load "name=value" pairs from filename"""
  d = {}
  f = open(filename)
  for line in f:
    line = line.strip()
    if not line or line.startswith("#"):
      continue
    k, v = line.split("=", 1)
    d[k] = v
  f.close()
  return d


def main(argv):
  if len(argv) != 4:
    print __doc__
    sys.exit(1)

  in_dir = argv[0]
  glob_dict_file = argv[1]
  out_file = argv[2]
  target_out = argv[3]

  glob_dict = LoadGlobalDict(glob_dict_file)
  if "mount_point" in glob_dict:
    # The caller knows the mount point and provides a dictionay needed by
    # BuildImage().
    image_properties = glob_dict
  else:
    image_filename = os.path.basename(out_file)
    mount_point = ""
    if image_filename == "system.img":
      mount_point = "system"
    elif image_filename == "system.img.fixup":
      mount_point = "system"
    elif image_filename == "system_other.img":
      mount_point = "system_other"
    elif image_filename == "userdata.img":
      mount_point = "data"
    elif image_filename == "cache.img":
      mount_point = "cache"
    elif image_filename == "vendor.img":
      mount_point = "vendor"
    elif image_filename == "oem.img":
      mount_point = "oem"
    elif image_filename == "custom.img":
      mount_point = "custom"
    else:
      print >> sys.stderr, "error: unknown image file name ", image_filename
      exit(1)

    image_properties = ImagePropFromGlobalDict(glob_dict, mount_point)

  if not BuildImage(in_dir, image_properties, out_file, target_out):
    print >> sys.stderr, "error: failed to build %s from %s" % (out_file,
                                                                in_dir)
    exit(1)


if __name__ == '__main__':
  main(sys.argv[1:])
