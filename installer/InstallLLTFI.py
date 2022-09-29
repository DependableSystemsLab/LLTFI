import sys, os
import subprocess
if sys.version_info >= (3,):
    import urllib.request as urllib2
    import urllib.parse as urlparse
else:
    import urllib2
    import urlparse
import argparse

LLTFIROOTDIRECTORY = "."

def Touch(path):
    with open(path, 'a'):
        os.utime(path, None)

def python3PrintParse(version):
  return version.split()[1]

def python3Parse(version):
  return version.split()[1].split('.')[:2]

python3Msg = "Error: Python 3 (python3) not found on path" + \
       "       Pease ensure python3 is installed and is available on the path"  + \
       "       The latest version of Python3 can be downloaded from:"  + \
       "       https://www.python.org/downloads/"
       
def CmakePrintParse(version):
  return version.split()[2]

def CmakeParse(version):
  return version.split()[2].split('.')[:2]

cmakeMsg = "\tCmake 3.15+ can be downloaded from:\n\thttp://www.cmake.org/cmake/resources/software.html" 

def ninjaPrintParse(version):
  return version.split()[0]

def ninjaParse(version):
  return version.split()[0].split('.')[:2]

ninjaMsg = "\tNinja 1.10+ can be downloaded from:\n\thttps://ninja-build.org/"

       
def checkDep(name, execName, versionArg, printParseFunc, parseFunc, minVersion, msg):
  try:
    which = subprocess.check_output(['which', execName])
    print("Success: " + name +  " Found at: " + str(which.strip()).lstrip("b\'").rstrip("\'"))
    version = str(subprocess.check_output([execName, versionArg], stderr=subprocess.STDOUT).strip())
    version = version.lstrip("b'").rstrip('\'').replace('\\n',' ')
    print("v", version)
    try:
      printVersion = str(printParseFunc(version))
      #print("pv", printVersion)
      version = parseFunc(str(version).strip())
      #print("cv", version)
      properVersion = True

      if int(version[0]) < minVersion[0]:
          #if name != "Protobuf":
          properVersion = False
          #else:
          #installProtobuf = True
      elif (int(version[0]) == minVersion[0]) and (int(version[1]) < minVersion[1]):
          #if name != "Protobuf":
          properVersion = False
          #else:
              #installProtobuf = True
      if properVersion:
        print("Success: " + name + "(" + printVersion + ") is at or above version " + ".".join([str(x) for x in minVersion]))
        return True
      else:
        print("Error: " + name + "(" + printVersion + ") is below version " + ".".join([str(x) for x in minVersion]))
        print(msg)
        return False
    except:
      print("Warning, " + name + " detected on path, but unable to parse version info.")
      print("Please ensure that " + name + " is at least of version: " + '.'.join([str(x) for x in minVersion]))
      return True   
  except(subprocess.CalledProcessError):
    print("Error: " + name + " (" + execName + ") not found on path")
    print("       Pease ensure " + name + " is installed and is available on the path")
    print(msg)
    return False

def checkDependencies():
    hasAll = True
    hasAll = checkDep("Python 3", "python3", "--version", python3PrintParse, python3Parse, [3,8], python3Msg) and hasAll
    hasAll = checkDep("Cmake","cmake","--version", CmakePrintParse, CmakeParse, [3,15], cmakeMsg) and hasAll
    hasAll = checkDep("Ninja","ninja","--version", ninjaPrintParse, ninjaParse, [1,10], ninjaMsg) and hasAll
    
    try:
        import pkg_resources
        versionInfo = pkg_resources.get_distribution("pyyaml").version
        if versionInfo[0] != '5' or versionInfo[2] != '4' or versionInfo[4] != '1':
            print("Incorrect PyYaml version. Please install v5.4.1")
            sys.exit(-1)
    except:
        print("PyYaml missing. Installing PyYaml..")
        os.system("pip3 install pyyaml===5.4.1")

    try:
        import tensorflow as tf
        versionInfo = tf.__version__
        if versionInfo[0] != '2':
            print("Incorrect tensorflow version. Please install v2.0 or greater")
            sys.exit(-1)
    except ImportError:
        print("Tensorflow missing. Installing Tensorflow..")
        os.system("pip install tensorflow")

    try:
        import tf2onnx as tfonnx
    except ImportError:
        print("tf2onnx missing. Installing tf2onnx..")
        os.system("pip install tf2onnx")

    return hasAll
    
def downloadSource():
      # LLTFI
      os.system('git clone https://github.com/DependableSystemsLab/LLTFI.git')
 
      # LLVM
      os.system('git clone https://github.com/llvm/llvm-project.git')
      os.chdir('llvm-project')
      os.system('git checkout 9778ec057cf4')
      os.chdir(os.path.join('..'))

      # onnx-mlir
      os.system('git clone --recursive https://github.com/DependableSystemsLab/onnx-mlir-lltfi.git')
      os.chdir('onnx-mlir-lltfi')
      os.system('git checkout LLTFI')
      os.chdir(os.path.join('..'))
      os.rename('onnx-mlir-lltfi','onnx-mlir')


def CheckDirExists(dir):
  FullPath = os.path.abspath(dir)
  if (os.path.exists(FullPath)):
    if (os.path.isdir(FullPath)):
      print("%s directory exists." % (dir))
      return True
  else:
    print("%s directory missing" % (dir))
  return False

def buildSource():
    # LLVM
    if CheckDirExists('llvm-project'):
        os.chdir("llvm-project")
        if (not CheckDirExists('build')): 
            os.mkdir("build")
        os.chdir("build")
        if (not os.path.exists("CMAKESUCCESS")):
            print("Running cmake for LLVM:")
            p = subprocess.call(["cmake", "-G", "Ninja", "../llvm", "-DLLVM_ENABLE_PROJECTS='clang;mlir'","-DLLVM_BUILD_TESTS=ON", "-DLLVM_TARGETS_TO_BUILD='host'", "-DLLVM_ENABLE_ASSERTIONS=ON", "-DLLVM_ENABLE_RTTI=ON"])
            p1 = subprocess.call(["cmake", "--build", ".", "--target", "clang", "check-mlir", "mlir-translate", "opt", "llc", "lli", "llvm-dis", "llvm-link", "-j1" ])
            if p != 0 or p1 != 0:
                sys.exit(p)
            Touch("CMAKESUCCESS")

        if (not os.path.exists("MAKESUCCESS")):
            print("Running make for LLVM")
            p = subprocess.call(["ninja", "install", "-j1"])
            if p != 0:
                sys.exit(p)
            Touch("MAKESUCCESS")
        os.chdir(os.path.join('../..'))


    else:
        print("LLVM source code missing. Run the installer script without -nD option")

    # ONNX-MLIR 
    if CheckDirExists('onnx-mlir'):
        cwd = os.getcwd()
        os.environ['MLIR_DIR'] = cwd + '/llvm-project/build/lib/cmake/mlir'
        os.chdir("onnx-mlir")
        if (not CheckDirExists('build')):
            os.mkdir("build")
        os.chdir("build")
        if (not os.path.exists("CMAKESUCCESS")):
            print("Running cmake for ONNX-MLIR:")
            p = subprocess.call(["cmake", "-G", "Ninja", "-DCMAKE_CXX_COMPILER=/usr/bin/c++", "-DMLIR_DIR=${MLIR_DIR}", ".."])
            p1 = subprocess.call(["cmake", "--build", ".", "-j1" ])
            if p != 0 or p1 != 0:
                sys.exit(p)
            Touch("CMAKESUCCESS")

        if (not os.path.exists("MAKESUCCESS")):
            print("Running make for ONNX-MLIR")
            p = subprocess.call(["ninja", "install", "-j1"])
            if p != 0:
                sys.exit(p)
            Touch("MAKESUCCESS")
        os.chdir(os.path.join('../..'))

    else:
        print("ONNX-MLIR source missing. Run the installer script without -nD option")


    # LLTFI
    if CheckDirExists('LLTFI'):
        os.chdir("LLTFI")
        if (not os.path.exists("BUILDSUCCESS")):
            print("Building LLTFI:")
            p = os.system('./setup -LLFI_BUILD_ROOT $(pwd)/build -LLVM_SRC_ROOT $(pwd)/../llvm-project -LLVM_DST_ROOT $(pwd)/../llvm-project/build')
            if p != 0:
                sys.exit(p)
            Touch("BUILDSUCCESS")
        os.chdir(os.path.join('..'))

    else:
        print("LLTFI source missing. Run the installer script without -nD option")

    cwd = os.getcwd()
    os.environ['LLFI_BUILD_ROOT'] = cwd + '/LLTFI/build'

def DownloadFile(url, destinationDirectory, desc=None):
    u = urllib2.urlopen(url)

    scheme, netloc, path, query, fragment = urlparse.urlsplit(url)
    filename = os.path.basename(path)
    if not filename:
        filename = 'downloaded.file'
    if desc:
        filename = os.path.join(desc, filename)

    with open(os.path.join(destinationDirectory, filename), 'wb') as f:
        meta = u.info()
        meta_func = meta.getheaders if hasattr(meta, 'getheaders') else meta.get_all
        meta_length = meta_func("Content-Length")
        file_size = None
        if meta_length:
            file_size = int(meta_length[0])
        print("Downloading: {0} Bytes: {1}".format(url, file_size))

        file_size_dl = 0
        block_sz = 8192
        while True:
            buffer = u.read(block_sz)
            if not buffer:
                break

            file_size_dl += len(buffer)
            f.write(buffer)

            status = "{0:16}".format(file_size_dl)
            if file_size:
                status += "   [{0:6.2f}%]".format(file_size_dl * 100 / file_size)
            status += chr(13)
            print(status, end="")
        print()

    return filename

def downloadAndInstallProtobuf():
    DownloadFile("https://github.com/protocolbuffers/protobuf/releases/download/v3.17.2/protobuf-all-3.17.2.zip", ".")
    os.system("unzip protobuf-all-3.17.2.zip")
    os.chdir("protobuf-3.17.2")
    os.system("./configure")
    os.system("make -j2")
    os.system("make check -j2")
    os.system("make install")
    os.system("ldconfig")
    os.chdir(os.path.join('../..'))


def runTests():
  LLFI_BUILD_DIR = os.path.dirname(os.path.realpath(__file__))
  subprocess.call(["python3", LLFI_BUILD_DIR + "/LLTFI/build/test_suite/SCRIPTS/llfi_test", "--all", "--threads", "2", "--verbose"])

parser = argparse.ArgumentParser(
    description=("Installer for UBC DependableSystemsLab's LLTFI"),
    epilog="More information available at www.github.com/DependableSystemsLab/LLTFI",
    usage='%(prog)s [options]')
parser.add_argument("-v", "--version", action="version", version="LLTFI Installer v0.1, September 23, 2022")
parser.add_argument("-sDC", "--skipDependencyCheck", action='store_true', help="Skip Dependency Checking")
parser.add_argument("-nPb", "--noProtobuf", action='store_true', help="Do not download and install Protobuf")
parser.add_argument("-nD", "--noDownload", action='store_true', help="Do not download any files")
parser.add_argument("-nB", "--noBuild", action='store_true', help="Do not perform installation, only download")
parser.add_argument("-rT", "--runTests", action='store_true', help="Run all regression tests for LLTFI after installation")


if __name__ == "__main__":
    args = parser.parse_args(sys.argv[1:])
    if not args.skipDependencyCheck:
        print("Checking LLTFI Pre-Requisites and Dependencies")
        deps = checkDependencies()
        if not deps:
            print("Some LLTFI Pre-Requisites are missing!")
            print("Please see Errors above, and install the missing dependencies")
            print("Exiting Installer...")
            sys.exit(-1)
    if not args.noProtobuf:
       downloadAndInstallProtobuf()
    print("Installing LLTFI to: " + os.path.abspath(LLTFIROOTDIRECTORY))
    if not args.noDownload:
        downloadSource()
    if not args.noBuild:
        buildSource()
    if args.runTests:
        runTests()
