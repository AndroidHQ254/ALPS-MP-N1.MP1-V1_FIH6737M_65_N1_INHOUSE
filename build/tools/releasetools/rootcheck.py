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


"""
import os
import subprocess
import sys
import binascii
import string
import zipfile
import common
import hashlib
import shutil
import struct
import math
import multiprocessing
import traceback

ENCRYPT_KEY=15

crc32_table = [0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
 0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
 0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
 0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
 0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
 0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
 0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
 0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
  0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
  0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
  0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
  0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
  0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
  0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
  0x2d02ef8d ]

def encrypt(key,s):
  b=bytearray(str(s).encode("gbk"))
  n=len(b)
  c=bytearray(n*2)
  j=0
  for i in range(0,n):
    b1=b[i]
    b2=b1^key
    c1=b2%16
    c2=b2//16
    c1=c1+65
    c2=c2+65
    c[j]=c1
    c[j+1]=c2
    j=j+2
  return c.decode("gbk")

def fileCountIn(dir):
  return sum([len(files) for root,dirs,files in os.walk(dir)])

def unsigned32(n):
  return n&0xffffffff

def fileCRC32(file,crc32,buf):
  crc32^=0xffffffff
  for tmp in buf:
    #print ("%x %x %x %x" %(unsigned32(crc32),ord(tmp),((unsigned32(crc32))^ord(tmp))&0xff,crc32_table[(crc32^ord(tmp))&0xff]))
    crc32 = crc32_table[(crc32^ord(tmp))&0xff]^((crc32>>8)&0xffffff)
    #print ("%x" %(unsigned32(crc32)))
  return crc32^0xffffffff

class sysfileItem(object):
    def __init__(self, number, filename, filecrc=0, filemd5=0):
        self.number = number
        self.filename = filename
        self.filecrc = filecrc
        self.filemd5 = filemd5

multiprocess_lock = multiprocessing.Lock()

class myThread (multiprocessing.Process):
    def __init__(self,task_queue, out_file):
        multiprocessing.Process.__init__(self)
        self.task_queue = task_queue
        self.out_file = out_file
    def get_file_count(self, number, filename, crc32, filemd5, key=ENCRYPT_KEY):
        crc32=crc32&0xffffffff
        filepath=encrypt(key,filename)
        filecrc=encrypt(key,crc32)
        filemd5=encrypt(key,filemd5)
        return "%d\t%s\t%s\t%s\n" % (number, filepath, filecrc, filemd5)
    def run(self):
        while True:
            item = self.task_queue.get()
            if item is None:
                #print "process %s stop" % self.name
                self.task_queue.task_done()
                break
            target = item.filename
            number = item.number
            crc32 = 0
            filemd5 = 0
            if os.path.isfile(target) and not os.path.islink(target):
              mymd5=hashlib.md5()
              frb=open(target,'rb')
              #print "%s processing %s" % (self.name, target)
              while True:
                  tmp=frb.read(4*1024)
                  if tmp=='':
                      break
                  crc32 += fileCRC32(target,crc32,tmp)
                  mymd5.update(tmp)
              filemd5 = mymd5.hexdigest()
              frb.close()
            line = self.get_file_count(number, target, crc32, filemd5)
            multiprocess_lock.acquire()
            try:
              with open(self.out_file, 'a') as f:
                f.write(line)
            except:
                print traceback.format_exc()
            multiprocess_lock.release()
            self.task_queue.task_done()
        return

def check_image_lk(fd,lk,s_str):
  global ENCRYPT_KEY
  key=ENCRYPT_KEY
  if os.path.isfile(lk):
    crc32=0
    mymd5=hashlib.md5()
    frb=open(lk,'rb')
    while True:
      tmp=frb.read(4*1024)
      if tmp=='':
        break
      crc32 += fileCRC32(lk,crc32,tmp)
      mymd5.update(tmp)
  else:
    print s_str," is not exist,Please check it"
    return
  crc32=crc32&0xffffffff
  nsize=os.path.getsize(lk)
  filename=encrypt(key,s_str)
  fd.write(filename)
  fd.write("\t")
  name_size=encrypt(key,nsize)
  fd.write(name_size)
  fd.write("\t")
  name_crc=encrypt(key,crc32)
  fd.write(name_crc)
  fd.write("\t")
  print lk,nsize,mymd5.hexdigest()
  #fd.write(str(mymd5.hexdigest()))
  name_md=encrypt(key,mymd5.hexdigest())
  fd.write(name_md)
  fd.write("\n")

#define BOOT_MAGIC "ANDROID"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
#struct boot_img_hdr
#{
#    unsigned char magic[BOOT_MAGIC_SIZE];

#    unsigned kernel_size;  /* size in bytes */
#    unsigned kernel_addr;  /* physical load addr */
#    unsigned ramdisk_size; /* size in bytes */
#    unsigned ramdisk_addr; /* physical load addr */
#    unsigned second_size;  /* size in bytes */
#    unsigned second_addr;  /* physical load addr */
#    unsigned tags_addr;    /* physical addr for kernel tags */
#    unsigned page_size;    /* flash page size we assume */
#    unsigned unused[2];    /* future expansion: should be 0 */

#    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */
#    unsigned char cmdline[BOOT_ARGS_SIZE];#
#    unsigned id[8]; /* timestamp / checksum / sha1 / etc */
#};
#  If boot_img_hdr is changed, bootimg_header must be modified
#  Boot_img_hdr is in system/core/mkbootimg/bootimg.h

bootimg_header = "<8c 10I 16c 512c 8I"
def get_bootimage_phy_size(img):
  tgt_img = "%s_data.%s" % (".".join(img.split('.')[:-1]), img.split('.')[-1])
  shutil.copy(img, tgt_img)
  f=open(tgt_img,'r+b')
  header_size = struct.calcsize(bootimg_header)
  s = struct.Struct(bootimg_header)
  fields = s.unpack(f.read(header_size))
  page_size = fields[15]
  hdr_size = page_size
  kernel_size = int(math.ceil(float(fields[8]) / page_size)) * page_size
  ramdisk_size = int(math.ceil(float(fields[10]) / page_size)) * page_size
  second_size = int(math.ceil(float(fields[12]) / page_size)) * page_size
  img_len = kernel_size + ramdisk_size + second_size + hdr_size
  f.truncate(img_len);
  f.close()
  return tgt_img

def check_img_kernel_only(fd,img,s_str):
  tmp_bootimg = get_bootimage_phy_size(img)
  check_image_lk(fd,tmp_bootimg,s_str)
  if os.path.isfile(tmp_bootimg):
    os.remove(tmp_bootimg)

def cal_image_crc(path,lk,logo,recovery,bootimage,trustzone):
  s_lk="lk.bin"
  #s_preloader="preloder.bin"
  s_logo="logo.bin"
  s_recovery="recovery.img"
  s_bootimage="boot.img"
  s_trustzone="trustzone.bin"
  f_crc= open(path, "w")
  check_image_lk(f_crc,lk,s_lk)
  check_image_lk(f_crc,logo,s_logo)
  check_image_lk(f_crc,recovery,s_recovery)
  check_img_kernel_only(f_crc,bootimage,s_bootimage)

  if trustzone is not None:
    check_image_lk(f_crc,trustzone,s_trustzone)

  f_crc.close()

def cal_system_file_crc(file_count,in_dir):
  value=-1
  number=0
  fileLists = []

  for root, dirs, files in os.walk(in_dir):
    for file in files:
      fileLists.append(os.path.join(root, file))

  with open(file_count, "w") as f:
    f.write("%s\t%s\t%s\n" % (str(value), "file_number_in_system_dayu", str(len(fileLists))))

  fileQ = multiprocessing.JoinableQueue()
  num_processes = multiprocessing.cpu_count()
  print 'Creating %d processes for system file checksum calculation' % num_processes

  checksum_processes = [ myThread(fileQ, file_count)
                  for i in xrange(num_processes) ]
  for p in checksum_processes:
    p.start()

  for file in fileLists:
      if os.path.basename(file)=="file_count":
        continue
      fileQ.put(sysfileItem(number, file))
      number += 1

  for i in xrange(num_processes):
    fileQ.put(None)
  fileQ.join()
  print 'All system files checksum are generated'

def gen_zip_file(zipname,file_count,crc_count,vdoub_check):
  if(os.path.isfile(zipname)):
    print "recovery_rootcheck is already is system/data,build otapackage?"
    os.remove(zipname)
  f_zip=zipfile.ZipFile(zipname,"w",zipfile.ZIP_DEFLATED)
  if(os.path.isfile(file_count)):
    f_fc=open(file_count).read()
    common.ZipWriteStr(f_zip, "file_count", f_fc)
  else:
    print "Error",file_count,"is not exist"
  if(os.path.isfile(crc_count)):
    f_cc=open(crc_count).read()
    common.ZipWriteStr(f_zip, "crc_count", f_cc)
  else:
    print "Error",crc_count,"is not exist"
  if(os.path.isfile(vdoub_check)):
    f_dc=open(vdoub_check).read()
    common.ZipWriteStr(f_zip, "doub_check", f_dc)
  else:
    print "Error",vdoub_check,"is not exist"
  f_zip.close()

def gen_double_check(file_count,crc_count,vdoub_check):
  fo = open(vdoub_check, "w")
  crc32=0
  if os.path.isfile(file_count) and not os.path.islink(file_count):
    frb=open(file_count,'rb')
    while True:
      tmp=frb.read(4*1024)
      if tmp=='':
        break
      crc32 += fileCRC32(file_count,crc32,tmp)
    crc32=crc32&0xffffffff
    fo.write(str(crc32))
    fo.write("\t")
    frb.close()
  crc32=0
  if os.path.isfile(crc_count) and not os.path.islink(crc_count):
    frb=open(crc_count,'rb')
    while True:
      tmp=frb.read(4*1024)
      if tmp=='':
        break
      crc32 += fileCRC32(crc_count,crc32,tmp)
    crc32=crc32&0xffffffff
    fo.write(str(crc32))
    fo.write("\n")
    frb.close()
  fo.close()

def main(argv):
  print "crc and md5 generate start"
  lk=argv[1]
  #preloader=argv[2]
  recovery=argv[2]
  bootimage=argv[3]
  logo=argv[4]
  out_dir = argv[5]
  trustzone = argv[6]
  in_dir = argv[0]
  print lk
  print recovery
  print bootimage
  print logo
  print in_dir
  file_count=out_dir+"/file_count"
  crc_count=out_dir+"/crc_count"
  vdoub_check=out_dir+"/doub_check"
  zipname=out_dir+"/recovery_rootcheck"

  if not os.path.exists(out_dir):
    os.makedirs(out_dir)

  if not os.path.exists(trustzone):
    trustzone = None

  print "cal system file crc begin"
  cal_system_file_crc(file_count,in_dir)
  print "cal system file crc end"

  print "cal image crc begin"
  cal_image_crc(crc_count,lk,logo,recovery,bootimage,trustzone)
  print "cal image crc end"

  print "cal double crc check begin"
  gen_double_check(file_count,crc_count,vdoub_check)
  print "cal double crc check end"

  print "generate zipfile begin"
  gen_zip_file(zipname,file_count,crc_count,vdoub_check)
  print "generate zipfile end"

  print "delete temporary files begin"
  if(os.path.isfile(file_count)):
    os.remove(file_count)
  if(os.path.isfile(crc_count)):
    os.remove(crc_count)
  if(os.path.isfile(vdoub_check)):
    os.remove(vdoub_check)
  print "delete temporary files end"
  print "crc and md5 generate end"

if __name__ == '__main__':
  main(sys.argv[1:])
