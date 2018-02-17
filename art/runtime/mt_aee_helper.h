#ifdef CHECK_JNI_HAVE_AEE_FEATURE
#include "aee.h"
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>
#include <string>
#include <stdio.h>
#include "runtime/art_method-inl.h"
#include <utils/CallStack.h>
#include <utils/String8.h>

namespace art {
#define CHECK_JNI_AEE_EXCEPTION(_msg, _className, _extraMsg)                            \
    do {                                                                                \
        std::string _aeeMsg("");                                                        \
        _aeeMsg = "\nCRDISPATCH_FILE: " + _className +                                 \
        "\nCRDISPATCH_KEY: JNI_CHECK_ERROR" + "\n" + _msg + _extraMsg;                  \
        aee_system_exception(_className.c_str(), NULL, DB_OPT_DEFAULT|DB_OPT_PROCESS_COREDUMP, _aeeMsg.c_str());    \
    } while (false)

inline std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

inline std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, elems);
  return elems;
}

inline std::string find_non_art_so(const std::string &bt) {
  std::vector<std::string> btVec = split(bt, '\n');
  for (unsigned int i = 0; i < btVec.size() ; i++) {
    std::string::size_type res = btVec[i].find("libart.so");
    if (res == std::string::npos) {
      return btVec[i];
    }
  }
  return nullptr;
}

inline void run_aee(ArtMethod* current_method, std::string aee_msg)
SHARED_REQUIRES(Locks::mutator_lock_) {
  std::string target_name;
  if (current_method == nullptr) {
    // parse backtrace for aee db file using
    // get [4] -> [0] ...                  [4]
    //           #01 pc 0000000000458b94  /system/lib64/libart.so
    LOG(INFO) << "current_method is null";
    android::CallStack stack;
    stack.update();
    std::string bt(stack.toString());
    std::string nonArtSo = find_non_art_so(bt);
    if (nonArtSo == nullptr) {
      LOG(ERROR) << "Can't find non art so!";
      return;
    }
    std::vector<std::string> soVec = split(nonArtSo, ' ');
    if (soVec.size() > 0) {
      std::string soPath(soVec[4]);
      std::vector<std::string> soNameVec = split(soPath, '/');
      target_name = soNameVec[soNameVec.size() - 1];
    }
  } else {
    std::string aee_name(PrettyMethod(current_method));
    std::vector<std::string> aee_nameV1 = split(aee_name, ' ');
    if (aee_nameV1.size() > 0) {
      std::string pkg_name(aee_nameV1[1]);
      std::vector<std::string> aee_nameV2 = split(pkg_name, '.');
      for (unsigned int i = 0; i < aee_nameV2.size() - 1 ; i++) {
        target_name += aee_nameV2[i];
        if (i + 1 != aee_nameV2.size() - 1) {
          target_name += "_";
        }
      }
      target_name += ".cpp";
    }
  }

  if ((unsigned int)target_name.size() > 0) {
    CHECK_JNI_AEE_EXCEPTION(aee_msg, target_name, "please refer to http://wiki.mediatek.inc/display/ART/JNI");
  }
}
}  // namespace art
#endif
