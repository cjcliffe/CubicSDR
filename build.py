#!/usr/bin/env python3
# Copyright (c) Charles J. Cliffe
# SPDX-License-Identifier: GPL-2.0+
"""Build CubicSDR from this checkout: dependencies, configure, build, package."""
import argparse
import glob
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile

ROOT = os.path.dirname(os.path.abspath(__file__))

WX_VERSION = "3.2.1"
WX_URL = "https://github.com/wxWidgets/wxWidgets/releases/download/v%s/wxWidgets-%s.tar.bz2"
SOAPY_TAG = "soapy-sdr-0.8.1"
SOAPY_URL = "https://github.com/pothosware/SoapySDR.git"
LIQUID_URL = "https://github.com/jgaeddert/liquid-dsp.git"
HAMLIB_VERSION = "4.4"
HAMLIB_URL = "https://github.com/Hamlib/Hamlib/releases/download/4.4/hamlib-%s-4.4.zip"
LINUXDEPLOY_URL = "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"


def log(text):
    print(text, flush=True)


def run(cmd, cwd=None, env=None, dry_run=False):
    log("+ %s" % " ".join(cmd))
    if dry_run:
        return
    merged = dict(os.environ)
    if env:
        merged.update(env)
    subprocess.run(cmd, cwd=cwd or ROOT, env=merged, check=True)


def require(prog, dry_run=False):
    if shutil.which(prog) is None:
        if dry_run:
            log("would require tool '%s'" % prog)
        else:
            sys.exit("error: required tool '%s' not found on PATH" % prog)


def download(url, dest, dry_run=False, retries=3):
    if os.path.exists(dest):
        log("have %s; skipping download" % dest)
        return
    log("+ download %s" % url)
    if dry_run:
        return
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    for attempt in range(retries):
        try:
            urllib.request.urlretrieve(url, dest + ".part")
            os.replace(dest + ".part", dest)
            return
        except Exception as error:
            log("download attempt %d failed: %s" % (attempt + 1, error))
    sys.exit("error: download failed after %d attempts: %s" % (retries, url))


def vs_file(parts, what):
    vswhere = os.path.join(os.environ.get("ProgramFiles(x86)", ""),
                           "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if os.path.exists(vswhere):
        root = subprocess.run(
            [vswhere, "-latest", "-property", "installationPath"],
            capture_output=True, text=True).stdout.strip()
        if root:
            matches = sorted(glob.glob(os.path.join(root, *parts)))
            if matches:
                return matches[-1]
    sys.exit("error: %s not found; install the MSVC C++ workload or "
             "run from a Visual Studio Developer shell" % what)


def msvc_lib_exe():
    return vs_file(("VC", "Tools", "MSVC", "*", "bin", "Hostx64", "x64",
                    "lib.exe"), "lib.exe")


def msbuild_exe(dry_run=False):
    if dry_run:
        return "msbuild"
    return vs_file(("MSBuild", "Current", "Bin", "amd64", "msbuild.exe"),
                   "msbuild.exe")


def devenv_exe(dry_run=False):
    if dry_run:
        return "devenv"
    return vs_file(("Common7", "IDE", "devenv.com"), "devenv")


def msbuild_sln(sln, config, arch, dry_run=False, upgrade=False):
    if upgrade:
        run([devenv_exe(dry_run), "/Upgrade", sln], dry_run=dry_run)
    run([msbuild_exe(dry_run), sln, "/p:Configuration=%s" % config,
         "/p:Platform=%s" % arch], dry_run=dry_run)


def unzip_staged(tarball, dest, dry_run=False):
    if os.path.isdir(dest):
        return
    if dry_run:
        log("+ extract %s" % tarball)
        return
    stage = dest + ".stage"
    shutil.rmtree(stage, ignore_errors=True)
    with zipfile.ZipFile(tarball) as archive:
        archive.extractall(stage)
    entries = os.listdir(stage)
    if len(entries) == 1 and os.path.isdir(os.path.join(stage, entries[0])):
        shutil.move(os.path.join(stage, entries[0]), dest)
    else:
        shutil.move(stage, dest)
    shutil.rmtree(stage, ignore_errors=True)


def cmake_build(src, build, config, jobs, prefix, arch=None, extra=None,
                dry_run=False):
    cmd = ["cmake", "-S", src, "-B", build,
           "-DCMAKE_BUILD_TYPE=%s" % config,
           "-DCMAKE_INSTALL_PREFIX=%s" % prefix] + (extra or [])
    if arch:
        cmd += ["-G", "Visual Studio 17 2022", "-A", arch]
    run(cmd, dry_run=dry_run)
    cmd = ["cmake", "--build", build]
    if arch:
        cmd += ["--config", config]
    cmd += ["--parallel", str(jobs)] if arch else ["-j", str(jobs)]
    run(cmd, dry_run=dry_run)
    cmd = ["cmake", "--install", build]
    if arch:
        cmd += ["--config", config]
    run(cmd, dry_run=dry_run)


SUPPORTED_MODULES = ["audio", "remote", "redpitaya", "netsdr",
                     "rtlsdr", "airspy", "airspyhf", "hackrf",
                     "limesuite", "bladerf", "pluto", "uhd"]
BOOST_VERSION = "1.84.0"
BOOST_MSVC = "14.3"
UHD_TAG = "v4.6.0.0"
XML2_VERSION = "2.13.5"
DEFAULT_MODULES = [name for name in SUPPORTED_MODULES if name != "netsdr"]


def selected_modules(spec):
    if spec == "default":
        return list(DEFAULT_MODULES)
    if spec == "all":
        return list(SUPPORTED_MODULES)
    if spec == "none":
        return []
    selected = spec.split(",")
    for name in selected:
        if name not in SUPPORTED_MODULES:
            sys.exit("error: unknown module '%s' (supported: %s)" % (
                name, ", ".join(SUPPORTED_MODULES)))
    return selected


def find_prefix_file(prefix, pattern):
    matches = sorted(glob.glob(os.path.join(prefix, "**", pattern),
                                recursive=True),
                     key=lambda path: ("build" in path.split(os.sep), path))
    if not matches:
        sys.exit("error: %s not found under %s" % (pattern, prefix))
    return matches[0]


def header_dir(prefix, header, dry_run=False):
    if dry_run:
        return os.path.join(prefix, "include")
    return os.path.dirname(find_prefix_file(prefix, header))


def stage_runtime_windows(prefix, out, dry_run=False):
    if dry_run:
        log("+ stage runtime DLLs and modules to %s" % out)
        return
    os.makedirs(out, exist_ok=True)
    for dll in glob.glob(os.path.join(prefix, "bin", "*.dll")):
        shutil.copy(dll, out)
    for mods in glob.glob(os.path.join(prefix, "lib", "SoapySDR", "modules*")):
        dest = os.path.join(out, "modules")
        os.makedirs(dest, exist_ok=True)
        wanted = set()
        for dll in glob.glob(os.path.join(mods, "*.dll")):
            wanted.add(os.path.basename(dll))
            shutil.copy(dll, dest)
        for stale in glob.glob(os.path.join(dest, "*.dll")):
            if os.path.basename(stale) not in wanted:
                log("removing deselected module %s" % stale)
                if not dry_run:
                    os.remove(stale)
    log("staged runtime to %s" % out)


def stage_dlls(libs, prefix, dry_run=False):
    dest = os.path.join(prefix, "bin")
    if dry_run:
        log("+ stage dependency DLLs to %s" % dest)
        return
    os.makedirs(dest, exist_ok=True)
    for lib in libs:
        for dll in glob.glob(os.path.join(os.path.dirname(lib), "*.dll")):
            target = os.path.join(dest, os.path.basename(dll))
            if os.path.abspath(dll) != os.path.abspath(target):
                shutil.copy(dll, target)


def soapy_module(name, url, prefix, config, jobs, arch, dry_run=False,
                 extra=None):
    if os.path.exists(stamp_path(prefix, "mod-" + name.lower())):
        log("%s already built; skipping" % name)
        return
    src = os.path.join(prefix, "src", name)
    git_clone(url, src, dry_run)
    cmake_build(src, os.path.join(prefix, "build", name), config, jobs,
                prefix, arch,
                ["-DCMAKE_PREFIX_PATH=%s" % prefix] + (extra or []), dry_run)
    mark_done(prefix, "mod-" + name.lower(), dry_run)


def stamp_path(prefix, name):
    return os.path.join(prefix, "stamps", name + ".done")


def mark_done(prefix, name, dry_run=False):
    if dry_run:
        return
    os.makedirs(os.path.dirname(stamp_path(prefix, name)), exist_ok=True)
    with open(stamp_path(prefix, name), "w") as handle:
        handle.write("ok\n")


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Build CubicSDR from this checkout.")
    parser.add_argument("--platform", default="auto",
                        choices=["auto", "windows", "linux", "macos"])
    parser.add_argument("--config", default="Release", choices=["Release", "Debug"])
    parser.add_argument("--arch", default="x64", choices=["x64", "Win32"])
    parser.add_argument("--jobs", default=str(os.cpu_count() or 2))
    parser.add_argument("--deps-prefix", default=None)
    parser.add_argument("--skip-deps", action="store_true")
    parser.add_argument("--modules", default="default",
                        help="'default', 'all', 'none', or comma-separated module list")
    parser.add_argument("--package", default="none",
                        choices=["none", "deb", "nsis", "dmg", "appimage"])
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args(argv)


def default_prefix():
    return os.path.join(ROOT, "build", "deps")


def git_clone(url, dest, dry_run=False, branch=None):
    if os.path.isdir(dest):
        log("%s exists; reusing" % dest)
        return
    cmd = ["git", "clone", "--depth", "1"]
    if branch:
        cmd += ["-b", branch]
    run(cmd + [url, dest], dry_run=dry_run)


def wx_source(prefix, dry_run=False):
    dest = os.path.join(prefix, "src", "wxWidgets-%s" % WX_VERSION)
    if not os.path.isdir(dest):
        tarball = os.path.join(prefix, "src", "wxWidgets-%s.tar.bz2" % WX_VERSION)
        download(WX_URL % (WX_VERSION, WX_VERSION), tarball, dry_run)
        if not dry_run:
            with tarfile.open(tarball) as archive:
                archive.extractall(os.path.join(prefix, "src"))
    return dest


def build_wx_linux(prefix, jobs, dry_run=False):
    src = wx_source(prefix, dry_run)
    if os.path.exists(stamp_path(prefix, "wx")):
        log("wxWidgets already built; skipping")
        return os.path.join(prefix, "wx", "bin", "wx-config")
    run(["sh", "autogen.sh"], cwd=src, dry_run=dry_run)
    run(["./configure", "--with-opengl", "--disable-glcanvasegl",
         "--disable-shared", "--enable-monolithic", "--with-libjpeg",
         "--with-libtiff", "--with-libpng", "--with-zlib",
         "--disable-sdltest", "--enable-unicode", "--enable-display",
         "--enable-propgrid", "--disable-webview", "--disable-webviewwebkit",
         "--prefix=%s" % os.path.join(prefix, "wx")], cwd=src, dry_run=dry_run)
    run(["make", "-j%s" % jobs], cwd=src, dry_run=dry_run)
    run(["make", "install"], cwd=src, dry_run=dry_run)
    mark_done(prefix, "wx", dry_run)
    return os.path.join(prefix, "wx", "bin", "wx-config")


def build_wx_macos(prefix, jobs, dry_run=False):
    src = wx_source(prefix, dry_run)
    if os.path.exists(stamp_path(prefix, "wx")):
        log("wxWidgets already built; skipping")
        return os.path.join(prefix, "wx", "bin", "wx-config")
    run(["sh", "autogen.sh"], cwd=src, dry_run=dry_run)
    run(["./configure", "--with-opengl", "--with-libjpeg", "--disable-shared",
         "--enable-monolithic", "--with-libtiff", "--with-libpng",
         "--with-zlib", "--disable-sdltest", "--enable-unicode",
         "--enable-display", "--enable-propgrid", "--disable-webview",
         "--disable-webviewwebkit",
         "--prefix=%s" % os.path.join(prefix, "wx")], cwd=src, dry_run=dry_run)
    run(["make", "-j%s" % jobs], cwd=src, dry_run=dry_run)
    run(["make", "install"], cwd=src, dry_run=dry_run)
    mark_done(prefix, "wx", dry_run)
    return os.path.join(prefix, "wx", "bin", "wx-config")


def build_wx_windows(prefix, config, arch, jobs, dry_run=False):
    if os.path.exists(stamp_path(prefix, "wx")):
        log("wxWidgets already built; skipping")
        return os.path.join(prefix, "wx")
    tarball = os.path.join(prefix, "src", "wxWidgets-%s.zip" % WX_VERSION)
    download("https://github.com/wxWidgets/wxWidgets/releases/download/v%s/wxWidgets-%s.zip" % (WX_VERSION, WX_VERSION), tarball, dry_run)
    src = os.path.join(prefix, "src", "wxWidgets-%s" % WX_VERSION)
    if not dry_run:
        unzip_staged(tarball, src, dry_run)
    build = os.path.join(src, "cmake-build")
    run(["cmake", "-S", src, "-B", build, "-G", "Visual Studio 17 2022",
         "-A", arch, "-DwxBUILD_SHARED=OFF",
         "-DCMAKE_INSTALL_PREFIX=%s" % os.path.join(prefix, "wx")],
        dry_run=dry_run)
    run(["cmake", "--build", build, "--config", config, "--parallel", str(jobs)],
        dry_run=dry_run)
    run(["cmake", "--install", build, "--config", config], dry_run=dry_run)
    mark_done(prefix, "wx", dry_run)
    return os.path.join(prefix, "wx")


def build_liquid(prefix, jobs, dry_run=False):
    src = os.path.join(prefix, "src", "liquid-dsp")
    if os.path.exists(stamp_path(prefix, "liquid")):
        log("liquid-dsp already built; skipping")
        return
    git_clone(LIQUID_URL, src, dry_run)
    run(["sh", "bootstrap.sh"], cwd=src, dry_run=dry_run)
    run(["./configure", "--prefix=%s" % prefix], cwd=src, dry_run=dry_run)
    run(["make", "-j%s" % jobs], cwd=src, dry_run=dry_run)
    run(["make", "install"], cwd=src, dry_run=dry_run)
    mark_done(prefix, "liquid", dry_run)


def soapy_build_dir(prefix):
    return os.path.join(prefix, "src", "SoapySDR", "build")


def build_soapy(prefix, config, jobs, arch=None, dry_run=False):
    src = os.path.join(prefix, "src", "SoapySDR")
    if os.path.exists(stamp_path(prefix, "soapy")):
        log("SoapySDR already built; skipping")
        return
    git_clone(SOAPY_URL, src, dry_run, branch=SOAPY_TAG)
    cmd = ["cmake", "-S", src, "-B", soapy_build_dir(prefix),
           "-DCMAKE_BUILD_TYPE=%s" % config,
           "-DCMAKE_INSTALL_PREFIX=%s" % prefix]
    if arch:
        cmd += ["-G", "Visual Studio 17 2022", "-A", arch]
    run(cmd, dry_run=dry_run)
    if arch:
        run(["cmake", "--build", soapy_build_dir(prefix), "--config", config,
             "--parallel", str(jobs)], dry_run=dry_run)
        run(["cmake", "--install", soapy_build_dir(prefix), "--config", config],
            dry_run=dry_run)
    else:
        run(["cmake", "--build", soapy_build_dir(prefix), "-j", str(jobs)],
            dry_run=dry_run)
        run(["cmake", "--install", soapy_build_dir(prefix)], dry_run=dry_run)
    mark_done(prefix, "soapy", dry_run)


def find_soapysdr_cmake(prefix, dry_run=False):
    matches = glob.glob(os.path.join(prefix, "**", "SoapySDRConfig.cmake"), recursive=True)
    if matches:
        return os.path.dirname(matches[0])
    if dry_run:
        return os.path.join(prefix, "lib", "cmake", "SoapySDR")
    sys.exit("error: SoapySDRConfig.cmake not found under %s" % prefix)


def fetch_hamlib_windows(prefix, arch, dry_run=False):
    if os.path.exists(stamp_path(prefix, "hamlib")):
        log("hamlib already fetched; skipping")
    else:
        tag = "w64" if arch == "x64" else "w32"
        dest = os.path.join(prefix, "src", "hamlib-%s-%s" % (tag, HAMLIB_VERSION))
        if not os.path.isdir(dest):
            download(HAMLIB_URL % tag, dest + ".zip", dry_run)
            if not dry_run:
                with zipfile.ZipFile(dest + ".zip") as archive:
                    archive.extractall(os.path.join(prefix, "src"))
        machine = "X64" if arch == "x64" else "X86"
        run([msvc_lib_exe() if not dry_run else "lib",
             "/def:libhamlib-4.def", "/MACHINE:%s" % machine],
            cwd=os.path.join(dest, "lib", "msvc"), dry_run=dry_run)
        if not dry_run:
            rig = os.path.join(dest, "include", "hamlib", "rig.h")
            with open(rig) as handle:
                text = handle.read()
            with open(rig, "w") as handle:
                handle.write(text.replace("#include <sys/time.h>", "// # include <sys/time.h>"))
        mark_done(prefix, "hamlib", dry_run)
    base = os.path.join(prefix, "src", "hamlib-%s-%s" % ("w64" if arch == "x64" else "w32", HAMLIB_VERSION))
    return os.path.join(base, "lib", "msvc", "libhamlib-4.lib"), os.path.join(base, "include")


def fetch_vc_redist(prefix, arch, dry_run=False):
    name = "vc_redist.%s.exe" % arch
    dest = os.path.join(prefix, name)
    if not os.path.exists(dest):
        download("https://aka.ms/vs/17/release/%s" % name, dest, dry_run)
    return dest


PTHREADS_VERSION = "3.0.3.1"
PTHREADS_URL = "https://github.com/GerHobbelt/pthread-win32/archive/refs/tags/v%s.zip"
FFTW_VERSION = "3.3.5"


def build_libusb_windows(prefix, config, arch, dry_run=False):
    if os.path.exists(stamp_path(prefix, "libusb")):
        log("libusb already built; skipping")
    else:
        src = os.path.join(prefix, "src", "libusb")
        git_clone("https://github.com/libusb/libusb", src, dry_run)
        msbuild_sln(os.path.join(src, "msvc", "libusb.sln"), config, arch,
                    dry_run)
        mark_done(prefix, "libusb", dry_run)
    src = os.path.join(prefix, "src", "libusb")
    if dry_run:
        lib = os.path.join(src, "build", "libusb-1.0.lib")
    else:
        lib = find_prefix_file(os.path.join(src, "build"), "libusb-1.0.lib")
    return os.path.join(src, "libusb"), lib


def build_pthreads_windows(prefix, config, arch, dry_run=False):
    dest = os.path.join(prefix, "src", "pthread-win32-%s" % PTHREADS_VERSION)
    if os.path.exists(stamp_path(prefix, "pthreads")):
        log("pthreads already built; skipping")
    else:
        download(PTHREADS_URL % PTHREADS_VERSION, dest + ".zip", dry_run)
        if not dry_run:
            unzip_staged(dest + ".zip", dest, dry_run)
        msbuild_sln(os.path.join(dest, "windows", "VS2019", "pthread.2019.sln"),
                    config, arch, dry_run, upgrade=True)
        mark_done(prefix, "pthreads", dry_run)
    bit = "64bit-x64" if arch == "x64" else "32bit-x86"
    lib = os.path.join(dest, "windows", "VS2019", "bin",
                       "%s-Unicode-%s" % (config, bit), "pthread.lib")
    config_file = os.path.join(prefix, "cmake", "PThreads4WConfig.cmake")
    if not dry_run:
        os.makedirs(os.path.dirname(config_file), exist_ok=True)
        with open(config_file, "w") as handle:
            handle.write("if(NOT TARGET PThreads4W::PThreads4W)\n"
                         "  add_library(PThreads4W::PThreads4W UNKNOWN IMPORTED)\n"
                         "  set_target_properties(PThreads4W::PThreads4W PROPERTIES\n"
                         "    IMPORTED_LOCATION \"%s\"\n"
                         "    INTERFACE_INCLUDE_DIRECTORIES \"%s\")\n"
                         "endif()\n" % (lib.replace("\\", "/"), dest.replace("\\", "/")))
    return dest, lib


def stage_libusb_windows(prefix, arch, dry_run=False):
    usb_src = os.path.join(prefix, "src", "libusb")
    dest = os.path.join(prefix, "libusb")
    if os.path.exists(stamp_path(prefix, "libusb-stage")):
        log("libusb already staged; skipping")
    else:
        if not dry_run:
            inc = os.path.join(dest, "include", "libusb-1.0")
            os.makedirs(inc, exist_ok=True)
            shutil.copy(os.path.join(usb_src, "libusb", "libusb.h"), inc)
            sub = "MS64" if arch == "x64" else "MS32"
            libdir = os.path.join(dest, sub, "dll")
            os.makedirs(libdir, exist_ok=True)
            for pattern in ("libusb-1.0.lib", "libusb-1.0.dll"):
                for found in glob.glob(os.path.join(usb_src, "build", "**", pattern),
                                       recursive=True):
                    shutil.copy(found, libdir)
                    break
        mark_done(prefix, "libusb-stage", dry_run)
    return dest


def install_boost_windows(prefix, arch, dry_run=False):
    dest = os.path.join(prefix, "boost")
    if os.path.exists(stamp_path(prefix, "boost")):
        log("boost already installed; skipping")
    else:
        bit = "64" if arch == "x64" else "32"
        installer = os.path.join(
            prefix, "src",
            "boost_%s-msvc-%s-%s.exe"
            % (BOOST_VERSION.replace(".", "_"), BOOST_MSVC, bit))
        download("https://downloads.sourceforge.net/project/boost/boost-binaries/"
                 "%s/boost_%s-msvc-%s-%s.exe"
                 % (BOOST_VERSION, BOOST_VERSION.replace(".", "_"),
                    BOOST_MSVC, bit),
                 installer, dry_run)
        run([installer, "/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART",
             "/DIR=%s" % dest], dry_run=dry_run)
        mark_done(prefix, "boost", dry_run)
    return dest


def patch_uhd_windows(src, dry_run=False):
    target = os.path.join(src, "host", "lib", "transport", "libusb1_base.cpp")
    marker = "#include <winsock2.h>  // CubicSDR build: struct timeval on MSVC"
    if dry_run:
        log("+ patch %s for MSVC timeval" % target)
        return
    with open(target, "r", newline="") as handle:
        text = handle.read()
    if marker not in text:
        log("patching %s for MSVC timeval" % target)
        text = text.replace('#include "libusb1_base.hpp"',
                            marker + '\n#include "libusb1_base.hpp"', 1)
        with open(target, "w", newline="") as handle:
            handle.write(text)


def ensure_mako(dry_run=False):
    if dry_run:
        log("+ ensure python mako module")
        return
    check = subprocess.run([sys.executable, "-c", "import mako"],
                           capture_output=True)
    if check.returncode != 0:
        run([sys.executable, "-m", "pip", "install", "--user", "mako"],
            dry_run=dry_run)


def fetch_fftw_windows(prefix, arch, dry_run=False):
    tag = "dll64" if arch == "x64" else "dll32"
    dest = os.path.join(prefix, "src", "fftw-%s-%s" % (FFTW_VERSION, tag))
    if os.path.exists(stamp_path(prefix, "fftw")):
        log("fftw already fetched; skipping")
    else:
        download("https://fftw.org/pub/fftw/fftw-%s-%s.zip" % (FFTW_VERSION, tag),
                 dest + ".zip", dry_run)
        if not dry_run:
            unzip_staged(dest + ".zip", dest, dry_run)
        machine = "X64" if arch == "x64" else "X86"
        run([msvc_lib_exe() if not dry_run else "lib", "/def:libfftw3f-3.def",
             "/MACHINE:%s" % machine], cwd=dest, dry_run=dry_run)
        mark_done(prefix, "fftw", dry_run)
    return dest, os.path.join(dest, "libfftw3f-3.lib")


def build_clib_windows(name, url, prefix, config, arch, jobs, extra,
                       subdir="", branch=None, dry_run=False):
    if os.path.exists(stamp_path(prefix, "lib-" + name)):
        log("%s already built; skipping" % name)
        return
    src = os.path.join(prefix, "src", name)
    git_clone(url, src, dry_run, branch=branch)
    cmake_build(os.path.join(src, subdir) if subdir else src,
                os.path.join(prefix, "build", name), config, jobs, prefix,
                arch, extra, dry_run)
    mark_done(prefix, "lib-" + name, dry_run)


def build_modules_windows(prefix, config, arch, jobs, selected, dry_run=False):
    selected = [key for key in SUPPORTED_MODULES if key in set(selected)]
    if not selected:
        return
    need_usb = set(selected) & {"rtlsdr", "airspy", "airspyhf", "hackrf",
                               "limesuite", "bladerf", "pluto", "uhd"}
    usb_inc = usb_lib = pt_inc = pt_lib = ""
    if need_usb:
        usb_inc, usb_lib = build_libusb_windows(prefix, config, arch, dry_run)
        pt_inc, pt_lib = build_pthreads_windows(prefix, config, arch, dry_run)
        stage_dlls([usb_lib, pt_lib], prefix, dry_run)
    pthread_defs = ["-DTHREADS_PTHREADS_LIBRARY:PATH=%s" % pt_lib,
                    "-DTHREADS_PTHREADS_INCLUDE_DIR:PATH=%s" % pt_inc]
    pthread_win32_defs = \
        ["-DTHREADS_PTHREADS_WIN32_LIBRARY:PATH=%s" % pt_lib,
         "-DTHREADS_PTHREADS_INCLUDE_DIR:PATH=%s" % pt_inc]
    usb_defs = ["-DLIBUSB_INCLUDE_DIRS:PATH=%s" % usb_inc,
                "-DLIBUSB_LIBRARIES:PATH=%s" % usb_lib]
    usb_single_defs = ["-DLIBUSB_INCLUDE_DIR:PATH=%s" % usb_inc,
                       "-DLIBUSB_LIBRARIES:PATH=%s" % usb_lib]
    if "hackrf" in selected:
        fftw_inc, fftw_lib = fetch_fftw_windows(prefix, arch, dry_run)
        stage_dlls([fftw_lib], prefix, dry_run)
        build_clib_windows(
            "hackrf", "https://github.com/greatscottgadgets/hackrf",
            prefix, config, arch, jobs,
            ["-DFFTW_LIBRARIES=%s" % fftw_lib,
             "-DFFTW_INCLUDES=%s" % fftw_inc,
             "-DCMAKE_PREFIX_PATH=%s" % prefix] + usb_single_defs,
            subdir="host", dry_run=dry_run)
    if "rtlsdr" in selected:
        build_clib_windows(
            "librtlsdr", "https://github.com/rtlsdrblog/rtl-sdr-blog",
            prefix, config, arch, jobs, pthread_defs + usb_defs,
            dry_run=dry_run)
    if "airspy" in selected:
        build_clib_windows(
            "airspyone_host", "https://github.com/airspy/airspyone_host",
            prefix, config, arch, jobs, pthread_win32_defs + usb_single_defs,
            dry_run=dry_run)
    if "airspyhf" in selected:
        build_clib_windows(
            "airspyhf", "https://github.com/airspy/airspyhf",
            prefix, config, arch, jobs, pthread_win32_defs + usb_single_defs,
            dry_run=dry_run)
    if "limesuite" in selected:
        build_clib_windows(
            "LimeSuite", "https://github.com/myriadrf/LimeSuite",
            prefix, config, arch, jobs,
            ["-DCMAKE_PREFIX_PATH=%s" % prefix,
             "-DLIBUSB_INCLUDE_DIRS:PATH=%s" % usb_inc,
             "-DLIBUSB_LIBRARIES:PATH=%s" % usb_lib,
             "-DLIBUSB_INCLUDE_DIR:PATH=%s" % usb_inc,
             "-DLIBUSB_LIBRARY:PATH=%s" % usb_lib],
            dry_run=dry_run)
    if "pluto" in selected:
        build_clib_windows(
            "zstd", "https://github.com/facebook/zstd",
            prefix, config, arch, jobs,
            ["-DZSTD_BUILD_PROGRAMS=OFF", "-DZSTD_BUILD_TESTS=OFF"],
            subdir="build/cmake", dry_run=dry_run)
        build_clib_windows(
            "libxml2", "https://github.com/GNOME/libxml2",
            prefix, config, arch, jobs,
            ["-DLIBXML2_WITH_ICONV=OFF", "-DLIBXML2_WITH_ZLIB=OFF",
             "-DLIBXML2_WITH_LZMA=OFF", "-DLIBXML2_WITH_PYTHON=OFF",
             "-DLIBXML2_WITH_PROGRAMS=OFF"],
            dry_run=dry_run)
        build_clib_windows(
            "libiio", "https://github.com/analogdevicesinc/libiio",
            prefix, config, arch, jobs,
            ["-DCMAKE_PREFIX_PATH=%s" % prefix,
             "-DLIBUSB_INCLUDE_DIR:PATH=%s" % usb_inc,
             "-DLIBUSB_LIBRARIES:PATH=%s" % usb_lib,
             "-DWITH_IIOD=OFF"],
            branch="v0.26", dry_run=dry_run)
        build_clib_windows(
            "libad9361", "https://github.com/analogdevicesinc/libad9361-iio",
            prefix, config, arch, jobs,
            ["-DCMAKE_PREFIX_PATH=%s" % prefix,
             "-DLIBIIO_INCLUDEDIR:PATH=%s" % os.path.join(prefix, "include"),
             "-DLIBIIO_LIBRARIES:PATH=%s" % os.path.join(prefix, "lib", "libiio.lib"),
             "-DBUILD_TESTS=OFF"],
            dry_run=dry_run)
    if "uhd" in selected:
        boost_root = install_boost_windows(prefix, arch, dry_run)
        boost_libdir = os.path.join(
            boost_root, "lib64-msvc-%s" % BOOST_MSVC).replace("\\", "/")
        boost_libpath = "/LIBPATH:%s" % boost_libdir
        ensure_mako(dry_run)
        git_clone("https://github.com/EttusResearch/uhd",
                  os.path.join(prefix, "src", "uhd"), dry_run, branch=UHD_TAG)
        patch_uhd_windows(os.path.join(prefix, "src", "uhd"), dry_run)
        build_clib_windows(
            "uhd", "https://github.com/EttusResearch/uhd",
            prefix, config, arch, jobs,
            ["-DBOOST_ROOT:PATH=%s" % boost_root,
             "-DBOOST_LIBRARYDIR:PATH=%s" % os.path.join(boost_root, "lib64-msvc-%s" % BOOST_MSVC),
             "-DLIBUSB_INCLUDE_DIRS:PATH=%s" % usb_inc,
             "-DLIBUSB_LIBRARIES:PATH=%s" % usb_lib,
             "-DCMAKE_SHARED_LINKER_FLAGS=%s" % boost_libpath,
             "-DCMAKE_EXE_LINKER_FLAGS=%s" % boost_libpath,
             "-DUHD_IMAGES_DOWNLOAD=ON",
             "-DENABLE_PYTHON_API=OFF", "-DENABLE_EXAMPLES=OFF",
             "-DENABLE_TESTS=OFF"],
            subdir="host", branch=UHD_TAG, dry_run=dry_run)
    if "bladerf" in selected:
        usb_path = stage_libusb_windows(prefix, arch, dry_run)
        build_clib_windows(
            "bladeRF", "https://github.com/Nuand/bladeRF",
            prefix, config, arch, jobs,
            pthread_win32_defs + usb_single_defs +
            ["-DLIBUSB_PATH:PATH=%s" % usb_path,
             "-DTREAT_WARNINGS_AS_ERRORS=OFF"],
            subdir="host", dry_run=dry_run)
        for dll in glob.glob(os.path.join(prefix, "lib", "bladeRF*.dll")):
            log("+ stage %s to prefix bin" % os.path.basename(dll))
            if not dry_run:
                shutil.copy(dll, os.path.join(prefix, "bin"))
    libs = {
        "rtlsdr": (["-DRTLSDR_LIBRARY:PATH=%s" % find_prefix_file(prefix, "rtlsdr.lib"),
                    "-DRTLSDR_INCLUDE_DIR:PATH=%s" % header_dir(prefix, "rtl-sdr.h", dry_run)]
                   if not dry_run else
                   ["-DRTLSDR_LIBRARY:PATH=rtlsdr.lib",
                    "-DRTLSDR_INCLUDE_DIR:PATH=%s" % header_dir(prefix, "rtl-sdr.h", dry_run)]),
        "airspy": (["-DLIBAIRSPY_LIBRARIES:PATH=%s" % find_prefix_file(prefix, "airspy.lib"),
                    "-DLIBAIRSPY_INCLUDE_DIRS:PATH=%s" % header_dir(prefix, "airspy.h", dry_run)]
                   if not dry_run else
                   ["-DLIBAIRSPY_LIBRARIES:PATH=airspy.lib",
                    "-DLIBAIRSPY_INCLUDE_DIRS:PATH=%s" % header_dir(prefix, "airspy.h", dry_run)]),
        "airspyhf": (["-DLIBAIRSPYHF_LIBRARIES:PATH=%s" % find_prefix_file(prefix, "airspyhf.lib"),
                      "-DLIBAIRSPYHF_INCLUDE_DIRS:PATH=%s" % header_dir(prefix, "airspyhf.h", dry_run)]
                     if not dry_run else
                     ["-DLIBAIRSPYHF_LIBRARIES:PATH=airspyhf.lib",
                      "-DLIBAIRSPYHF_INCLUDE_DIRS:PATH=%s" % header_dir(prefix, "airspyhf.h", dry_run)]),
        "hackrf": (["-DLIBHACKRF_LIBRARY:PATH=%s" % find_prefix_file(prefix, "hackrf.lib"),
                    "-DLIBHACKRF_INCLUDE_DIR:PATH=%s" % header_dir(prefix, "hackrf.h", dry_run)]
                   if not dry_run else
                   ["-DLIBHACKRF_LIBRARY:PATH=hackrf.lib",
                    "-DLIBHACKRF_INCLUDE_DIR:PATH=%s" % header_dir(prefix, "hackrf.h", dry_run)]),
    }
    if "uhd" in selected:
        libs["uhd"] = ["-DBOOST_ROOT:PATH=%s" % boost_root,
                       "-DBOOST_LIBRARYDIR:PATH=%s" % boost_libdir.replace("/", "\\"),
                       "-DCMAKE_SHARED_LINKER_FLAGS=%s" % boost_libpath,
                       "-DCMAKE_MODULE_LINKER_FLAGS=%s" % boost_libpath,
                       "-DCMAKE_EXE_LINKER_FLAGS=%s" % boost_libpath]
    repos = {
        "audio": "https://github.com/pothosware/SoapyAudio.git",
        "remote": "https://github.com/pothosware/SoapyRemote.git",
        "redpitaya": "https://github.com/pothosware/SoapyRedPitaya.git",
        "netsdr": "https://github.com/pothosware/SoapyNetSDR.git",
        "rtlsdr": "https://github.com/pothosware/SoapyRTLSDR.git",
        "airspy": "https://github.com/pothosware/SoapyAirspy.git",
        "airspyhf": "https://github.com/pothosware/SoapyAirspyHF.git",
        "hackrf": "https://github.com/pothosware/SoapyHackRF.git",
        "bladerf": "https://github.com/pothosware/SoapyBladeRF.git",
        "pluto": "https://github.com/pothosware/SoapyPlutoSDR.git",
        "uhd": "https://github.com/pothosware/SoapyUHD.git",
    }
    names = {
        "audio": "SoapyAudio", "remote": "SoapyRemote",
        "redpitaya": "SoapyRedPitaya", "netsdr": "SoapyNetSDR",
        "rtlsdr": "SoapyRTLSDR", "airspy": "SoapyAirspy",
        "airspyhf": "SoapyAirspyHF", "hackrf": "SoapyHackRF",
        "bladerf": "SoapyBladeRF", "pluto": "SoapyPlutoSDR",
        "uhd": "SoapyUHD",
    }
    for key in selected:
        if key not in repos:
            continue
        soapy_module(names[key], repos[key], prefix, config, jobs, arch,
                     dry_run, extra=libs.get(key))


def sudo_apt(packages, dry_run=False):
    euid = getattr(os, "geteuid", lambda: 0)()
    prefix = ["sudo", "apt-get"] if euid != 0 else ["apt-get"]
    run(prefix + ["update"], dry_run=dry_run)
    run(prefix + ["install", "-y"] + packages, dry_run=dry_run)


def build_linux(args, dry_run=False):
    for prog in ["cmake", "git", "make"]:
        require(prog, dry_run)
    prefix = os.path.abspath(args.deps_prefix or default_prefix())
    if not args.skip_deps:
        sudo_apt(["build-essential", "cmake", "git", "autoconf", "automake",
                  "libtool", "pkg-config", "libpulse-dev", "libgtk-3-dev",
                  "freeglut3-dev", "libhamlib-dev"], dry_run)
        wx_config = build_wx_linux(prefix, args.jobs, dry_run)
        build_liquid(prefix, args.jobs, dry_run)
        build_soapy(prefix, args.config, args.jobs, dry_run=dry_run)
    else:
        wx_config = os.path.join(prefix, "wx", "bin", "wx-config")
    if not dry_run and not os.path.exists(wx_config):
        sys.exit("error: %s missing; run without --skip-deps first" % wx_config)
    preset = "linux-deb" if args.package == "deb" else "linux"
    run(["cmake", "--preset", preset, "-DCMAKE_BUILD_TYPE=%s" % args.config],
        dry_run=dry_run,
        env={"CUBICSDR_WXCONFIG": wx_config, "CMAKE_PREFIX_PATH": prefix})
    run(["cmake", "--build", "--preset", preset, "-j", str(args.jobs)], dry_run=dry_run)
    if args.package == "deb":
        run(["cpack", "-G", "DEB"], cwd=os.path.join(ROOT, "build", preset),
            dry_run=dry_run)


def build_windows(args, dry_run=False):
    for prog in ["cmake", "git"]:
        require(prog, dry_run)
    prefix = os.path.abspath(args.deps_prefix or default_prefix())
    extra = []
    if not args.skip_deps:
        wx_root = build_wx_windows(prefix, args.config, args.arch, args.jobs, dry_run)
        build_soapy(prefix, args.config, args.jobs, arch=args.arch, dry_run=dry_run)
        hamlib_lib, hamlib_include = fetch_hamlib_windows(prefix, args.arch, dry_run)
    else:
        wx_root = os.path.join(prefix, "wx")
        hamlib_lib = ""
        hamlib_include = ""
    if args.package == "nsis":
        require("makensis", dry_run)
        redist = fetch_vc_redist(prefix, args.arch, dry_run)
        extra += ["-DBUILD_INSTALLER=ON", "-DBUNDLE_SOAPY_MODS=ON",
                  "-DSOAPY_SDR_ROOT=%s" % prefix,
                  "-DBUNDLE_MSVC_REDIST=%s" % redist]
    if hamlib_lib:
        extra += ["-DHAMLIB_LIBRARY=%s" % hamlib_lib,
                  "-DHAMLIB_INCLUDE_DIR=%s" % hamlib_include]
    build_modules_windows(prefix, args.config, args.arch, args.jobs,
                          selected_modules(args.modules), args.dry_run)
    preset = "windows-x86" if args.arch == "Win32" else "windows"
    run(["cmake", "--preset", preset] + extra, dry_run=dry_run,
        env={"CUBICSDR_WXWIN": wx_root, "CUBICSDR_SOAPY": prefix})
    run(["cmake", "--build", "--preset", preset, "--config", args.config,
         "-j", str(args.jobs)], dry_run=dry_run)
    out = os.path.join(ROOT, "build", preset,
                       "x64" if args.arch == "x64" else "x86")
    stage_runtime_windows(prefix, out, args.dry_run)
    if args.package == "nsis":
        run(["cpack", "-G", "NSIS", "-C", args.config],
            cwd=os.path.join(ROOT, "build", preset), dry_run=dry_run)


def build_macos(args, dry_run=False):
    for prog in ["brew", "cmake", "git", "make"]:
        require(prog, dry_run)
    prefix = os.path.abspath(args.deps_prefix or default_prefix())
    if not args.skip_deps:
        run(["brew", "install", "automake", "cmake", "git", "autoconf",
             "libtool", "pkg-config", "wget", "libusb", "fftw", "hamlib"],
            dry_run=dry_run)
        wx_config = build_wx_macos(prefix, args.jobs, dry_run)
        build_liquid(prefix, args.jobs, dry_run)
        build_soapy(prefix, args.config, args.jobs, dry_run=dry_run)
    else:
        wx_config = os.path.join(prefix, "wx", "bin", "wx-config")
    if not dry_run and not os.path.exists(wx_config):
        sys.exit("error: %s missing; run without --skip-deps first" % wx_config)
    soapy_cmake = find_soapysdr_cmake(prefix, dry_run)
    run(["cmake", "--preset", "macos", "-DCMAKE_BUILD_TYPE=%s" % args.config],
        dry_run=dry_run,
        env={"CUBICSDR_MACOS_PREFIX": prefix,
             "CUBICSDR_WXCONFIG": wx_config,
             "CUBICSDR_SOAPY_CMAKE": soapy_cmake,
             "CMAKE_PREFIX_PATH": prefix})
    run(["cmake", "--build", "--preset", "macos", "-j", str(args.jobs)],
        dry_run=dry_run)
    if args.package == "dmg":
        run(["cpack", "-G", "DragNDrop"], cwd=os.path.join(ROOT, "build", "macos"),
            dry_run=dry_run)


def build_appimage(args, dry_run=False):
    require("cmake", dry_run)
    prefix = os.path.abspath(args.deps_prefix or default_prefix())
    if not args.skip_deps:
        build_linux(args, dry_run)
    appdir = os.path.join(ROOT, "build", "AppDir")
    if not dry_run and os.path.isdir(appdir):
        shutil.rmtree(appdir)
    log("installing to %s" % appdir if not dry_run else "would install to %s" % appdir)
    if not dry_run:
        os.makedirs(os.path.join(appdir, "usr"), exist_ok=True)
    run(["cmake", "--install", os.path.join(ROOT, "build", "linux"),
         "--prefix", os.path.join(appdir, "usr")], dry_run=dry_run)
    tool = os.path.join(prefix, "linuxdeploy-x86_64.AppImage")
    if not os.path.exists(tool):
        download(LINUXDEPLOY_URL, tool, dry_run)
    if not dry_run:
        os.chmod(tool, 0o755)
    run([tool, "--appimage-extract-and-run", "--appdir", appdir,
         "--desktop-file", os.path.join(ROOT, "build", "linux", "CubicSDR.desktop"),
         "--icon-file", os.path.join(ROOT, "src", "CubicSDR.png"),
         "--output", "appimage"], cwd=os.path.join(ROOT, "build"), dry_run=dry_run,
        env={"APPIMAGE_EXTRACT_AND_RUN": "1"})


def clean(target, dry_run=False):
    path = os.path.join(ROOT, "build")
    log("would remove %s" % path if dry_run else "removing %s" % path)
    if not dry_run:
        shutil.rmtree(path, ignore_errors=True)


def main(argv=None):
    args = parse_args(argv or sys.argv[1:])
    target = args.platform
    if target == "auto":
        target = {"Windows": "windows", "Linux": "linux",
                  "Darwin": "macos"}.get(platform.system())
        if target is None:
            sys.exit("error: unsupported host; pass --platform explicitly")
    pairs = {"deb": "linux", "nsis": "windows", "dmg": "macos", "appimage": "linux"}
    if args.package != "none" and pairs[args.package] != target:
        sys.exit("error: --package %s needs --platform %s"
                 % (args.package, pairs[args.package]))
    if args.clean:
        clean(target, args.dry_run)
        return 0
    log("building CubicSDR for %s (%s)" % (target, args.config))
    if target == "windows":
        build_windows(args, args.dry_run)
    elif target == "linux":
        if args.package == "appimage":
            build_appimage(args, args.dry_run)
        else:
            build_linux(args, args.dry_run)
    elif target == "macos":
        build_macos(args, args.dry_run)
    log("done")
    return 0


if __name__ == "__main__":
    sys.exit(main())
