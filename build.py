#!/usr/bin/env python3
# Copyright (c) Charles J. Cliffe
# SPDX-License-Identifier: GPL-2.0+
"""Build CubicSDR from this checkout: dependencies, configure, build, package.

Layout: third-party sources live under build/deps/src, everything installs
into the build/deps prefix, and stamp files under build/deps/stamps record
completed steps so reruns skip finished work. Pass --dry-run to preview the
full command sequence without running anything.
"""
import argparse
import glob
import hashlib
import os
import platform
import re
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
    """Print a build step immediately; unbuffered so CI logs stay ordered."""
    print(text, flush=True)


def run(cmd, cwd=None, env=None, dry_run=False):
    """Echo then execute a command; extra env entries merge over os.environ."""
    log("+ %s" % " ".join(cmd))
    if dry_run:
        return
    merged = dict(os.environ)
    if env:
        merged.update(env)
    subprocess.run(cmd, cwd=cwd or ROOT, env=merged, check=True)


def require(prog, dry_run=False):
    """Abort unless prog is on PATH; in dry-run just note that it is needed."""
    if shutil.which(prog) is None:
        if dry_run:
            log("would require tool '%s'" % prog)
        else:
            sys.exit("error: required tool '%s' not found on PATH" % prog)


def download(url, dest, dry_run=False, retries=3):
    """Fetch url to dest, resuming-safe via a .part file; skip if dest exists."""
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
    """Locate a Visual Studio tool via vswhere; newest match wins."""
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
    """Path to lib.exe for building import libraries from .def files."""
    return vs_file(("VC", "Tools", "MSVC", "*", "bin", "Hostx64", "x64",
                    "lib.exe"), "lib.exe")


def msbuild_exe(dry_run=False):
    """Path to msbuild.exe, or its bare name when only previewing."""
    if dry_run:
        return "msbuild"
    return vs_file(("MSBuild", "Current", "Bin", "amd64", "msbuild.exe"),
                   "msbuild.exe")


def devenv_exe(dry_run=False):
    """Path to devenv.com, used only for one-off .sln upgrades."""
    if dry_run:
        return "devenv"
    return vs_file(("Common7", "IDE", "devenv.com"), "devenv")


def msbuild_sln(sln, config, arch, dry_run=False, upgrade=False):
    """Build a vendored Visual Studio solution in place."""
    if upgrade:
        run([devenv_exe(dry_run), "/Upgrade", sln], dry_run=dry_run)
    run([msbuild_exe(dry_run), sln, "/p:Configuration=%s" % config,
         "/p:Platform=%s" % arch], dry_run=dry_run)


def unzip_staged(tarball, dest, dry_run=False):
    """Extract a zip, collapsing a single top-level directory when present."""
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
    """Configure, build, and install one CMake project into the prefix."""
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


# Every SoapySDR module this script knows how to build. netsdr is excluded
# from the default set because it hangs device enumeration; uhd is excluded
# because few users have the hardware and its build is very long.
SUPPORTED_MODULES = ["audio", "remote", "redpitaya", "netsdr",
                     "rtlsdr", "airspy", "airspyhf", "hackrf",
                     "limesuite", "bladerf", "pluto", "uhd", "sdrplay"]
BOOST_VERSION = "1.84.0"
BOOST_MSVC = "14.3"
UHD_TAG = "v4.6.0.0"
XML2_VERSION = "2.13.5"
DEFAULT_OPT_OUT = {"netsdr",  # hangs device enumeration
                   "uhd"}  # rare hardware, very long build; opt in explicitly
DEFAULT_MODULES = [name for name in SUPPORTED_MODULES
                   if name not in DEFAULT_OPT_OUT]
# Module key to Soapy source repo and CMake project name. Shared by the
# Windows and Linux module builders; per-module configure flags live in
# each builder's local libs table.
SOAPY_REPOS = {
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
    "sdrplay": "https://github.com/pothosware/SoapySDRPlay3.git",
}
SOAPY_NAMES = {
    "audio": "SoapyAudio", "remote": "SoapyRemote",
    "redpitaya": "SoapyRedPitaya", "netsdr": "SoapyNetSDR",
    "rtlsdr": "SoapyRTLSDR", "airspy": "SoapyAirspy",
    "airspyhf": "SoapyAirspyHF", "hackrf": "SoapyHackRF",
    "bladerf": "SoapyBladeRF", "pluto": "SoapyPlutoSDR",
    "uhd": "SoapyUHD", "sdrplay": "SoapySDRPlay3",
}


def selected_modules(spec):
    """Resolve a --modules value to an ordered module key list."""
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
    """Find a built file under the prefix, preferring installed copies.

    Glob results sort build-tree paths last so an installed library wins
    over an intermediate object of the same name.
    """
    matches = sorted(glob.glob(os.path.join(prefix, "**", pattern),
                                recursive=True),
                     key=lambda path: ("build" in path.split(os.sep), path))
    if not matches:
        sys.exit("error: %s not found under %s" % (pattern, prefix))
    return matches[0]


def header_dir(prefix, header, dry_run=False):
    """Directory containing a built header under the prefix."""
    if dry_run:
        return os.path.join(prefix, "include")
    return os.path.dirname(find_prefix_file(prefix, header))


def stage_runtime_windows(prefix, out, dry_run=False):
    """Copy prefix DLLs beside the exe and SoapySDR modules into modules/.

    Modules that are no longer selected are pruned so a deselected module
    cannot linger and load on the next launch.
    """
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
    """Copy each library's sibling DLLs into the prefix bin directory."""
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
    """Clone, build, and install one SoapySDR module into the prefix.

    On Linux the installed module gets an $ORIGIN-relative rpath so it
    finds libSoapySDR and the device libraries two dirs up (usr/lib),
    which holds both for the prefix layout (lib/SoapySDR/modules*/) and
    the AppDir layout (bin/modules/).
    """
    stamp = "mod-" + name.lower()
    key = stamp_key(extra or [])
    if stamp_valid(prefix, stamp, key, adopt=not dry_run):
        log("%s already built; skipping" % name)
        return
    if os.path.exists(stamp_path(prefix, stamp)):
        log("%s flags changed; rebuilding" % name)
    src = os.path.join(prefix, "src", name)
    git_clone(url, src, dry_run)
    flags = ["-DCMAKE_PREFIX_PATH=%s" % prefix] + (extra or [])
    if platform.system() == "Linux" and arch is None:
        # Plain -D passes $ORIGIN through verbatim (no shell involved).
        flags += ["-DCMAKE_INSTALL_RPATH=$ORIGIN/../../lib",
                  "-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON"]
    cmake_build(src, os.path.join(prefix, "build", name), config, jobs,
                prefix, arch, flags, dry_run)
    mark_done(prefix, stamp, dry_run, key)


def stamp_path(prefix, name):
    """Stamp file recording that a named build step completed."""
    return os.path.join(prefix, "stamps", name + ".done")


def stamp_key(parts):
    """Short hash of configure inputs; a flag edit must invalidate the stamp."""
    return hashlib.md5(repr(parts).encode()).hexdigest()[:12]


def stamp_valid(prefix, name, key=None, adopt=True):
    """True when a step can be skipped: stamp exists and, if keyed, matches.

    Pre-keying "ok" stamps are adopted once by writing the current key, so
    existing checkouts keep skipping until flags actually change.
    """
    path = stamp_path(prefix, name)
    if not os.path.exists(path):
        return False
    if key is None:
        return True
    with open(path) as handle:
        seen = handle.read().strip()
    if seen == key:
        return True
    if seen == "ok" and adopt:
        with open(path, "w") as handle:
            handle.write("%s\n" % key)
        return True
    return False


def mark_done(prefix, name, dry_run=False, key=None):
    """Record a completed step; never writes anything during a dry run."""
    if dry_run:
        return
    os.makedirs(os.path.dirname(stamp_path(prefix, name)), exist_ok=True)
    with open(stamp_path(prefix, name), "w") as handle:
        handle.write("%s\n" % (key or "ok"))


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Build CubicSDR from this checkout.")
    parser.add_argument("--platform", default="auto",
                        choices=["auto", "windows", "linux", "macos"])
    parser.add_argument("--config", default="Release", choices=["Release", "Debug"])
    parser.add_argument("--arch", default="x64", choices=["x64", "Win32"])
    parser.add_argument("--macos-arch", default="native",
                        choices=["native", "x86_64", "arm64"],
                        help="macOS binary architecture; anything but native "
                        "cross-compiles via -arch flags")
    parser.add_argument("--jobs", default=str(min(os.cpu_count() or 2, 8)),
                        help="parallel build jobs (default: CPU count, max 8)")
    parser.add_argument("--deps-prefix", default=None)
    parser.add_argument("--skip-deps", action="store_true")
    parser.add_argument("--modules", default="default",
                        help="'default', 'all', 'none', or comma-separated module list")
    parser.add_argument("--skip-app", action="store_true",
                        help="build and stage modules only; skip the CubicSDR app")
    parser.add_argument("--build-deps", action="store_true",
                        help="build all dependencies from source instead of using system packages (Linux)")
    parser.add_argument("--package", default="none",
                        choices=["none", "deb", "nsis", "dmg", "appimage"])
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args(argv)


def default_prefix():
    """Dependency prefix; overridable with --deps-prefix."""
    return os.path.join(ROOT, "build", "deps")


def git_clone(url, dest, dry_run=False, branch=None):
    """Shallow-clone url once; reuse the existing checkout on later runs."""
    if os.path.isdir(dest):
        log("%s exists; reusing" % dest)
        return
    cmd = ["git", "clone", "--depth", "1"]
    if branch:
        cmd += ["-b", branch]
    run(cmd + [url, dest], dry_run=dry_run)


def wx_source(prefix, dry_run=False):
    """Download and unpack the pinned wxWidgets release under src/."""
    dest = os.path.join(prefix, "src", "wxWidgets-%s" % WX_VERSION)
    if not os.path.isdir(dest):
        tarball = os.path.join(prefix, "src", "wxWidgets-%s.tar.bz2" % WX_VERSION)
        download(WX_URL % (WX_VERSION, WX_VERSION), tarball, dry_run)
        if not dry_run:
            with tarfile.open(tarball) as archive:
                archive.extractall(os.path.join(prefix, "src"))
    return dest


def build_wx_linux(prefix, jobs, dry_run=False):
    """Build static monolithic wxWidgets for Linux into the prefix."""
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


def scrub_wx_config(src, dry_run=False):
    """Replace surviving @EGREP@ placeholders in generated wx-config.

    wx 3.2.1's configure can leave @EGREP@ unsubstituted (seen on the
    macOS CI runners for both native and cross builds); every wx-config
    query then fails, including the ones CMake's FindwxWidgets relies on.
    Repairing the generated scripts covers every cause uniformly.
    """
    configs = sorted(glob.glob(os.path.join(src, "lib", "wx", "config", "*")))
    if dry_run:
        log("+ scrub unsubstituted placeholders from generated wx-config")
        return
    for path in configs:
        if not os.path.isfile(path):
            continue
        with open(path) as handle:
            text = handle.read()
        fixed = text.replace("@EGREP@", "grep -E")
        if fixed == text:
            continue
        log("wx-config: substituting unexpanded @EGREP@ in %s"
            % os.path.basename(path))
        with open(path, "w") as handle:
            handle.write(fixed)
        leftovers = sorted(set(re.findall(r"@[A-Z_]+@", fixed)))
        if leftovers:
            log("warning: %s still has placeholders: %s"
                % (os.path.basename(path), ", ".join(leftovers)))


def remove_shared_liquid(prefix, dry_run=False):
    """Drop shared liquid-dsp libraries from a macOS prefix.

    liquid-dsp installs its shared library even with --disable-shared;
    the app links whatever FindLiquid finds, and a bare-install-name
    dylib breaks fixup_bundle at packaging time. Static only.
    """
    for stale in glob.glob(os.path.join(prefix, "lib", "libliquid*.dylib")):
        log("removing shared %s; macOS links liquid-dsp statically"
            % os.path.basename(stale))
        if not dry_run:
            os.remove(stale)


def build_wx_macos(prefix, jobs, dry_run=False, env=None, host=None):
    """Build static monolithic wxWidgets for macOS into the prefix.

    Always exports a working CPP: wx appends its deployment-target flag
    to whatever CPP it finds, so an unset CPP becomes bare flags and
    kills the bundled-library sub-configures. host is the autoconf
    --host triplet for cross-compiles.
    """
    src = wx_source(prefix, dry_run)
    if os.path.exists(stamp_path(prefix, "wx")):
        log("wxWidgets already built; skipping")
        return os.path.join(prefix, "wx", "bin", "wx-config")
    env = dict(env or {})
    cpp = "gcc -E"
    if host is not None:
        # $CPP never sees $CFLAGS, so the -arch flag rides along here.
        cpp += " -arch %s" % host.split("-")[0]
    env.setdefault("CPP", cpp)
    cmd = ["./configure", "--with-opengl", "--with-libjpeg", "--disable-shared",
           "--enable-monolithic", "--with-libtiff", "--with-libpng",
           "--with-zlib", "--disable-sdltest", "--enable-unicode",
           "--enable-display", "--enable-propgrid", "--disable-webview",
           "--disable-webviewwebkit",
           "--prefix=%s" % os.path.join(prefix, "wx")]
    if host is not None:
        cmd += ["--host=%s" % host]
    run(["sh", "autogen.sh"], cwd=src, dry_run=dry_run, env=env)
    run(cmd, cwd=src, dry_run=dry_run, env=env)
    scrub_wx_config(src, dry_run)
    run(["make", "-j%s" % jobs], cwd=src, dry_run=dry_run, env=env)
    run(["make", "install"], cwd=src, dry_run=dry_run, env=env)
    mark_done(prefix, "wx", dry_run)
    return os.path.join(prefix, "wx", "bin", "wx-config")


def build_wx_windows(prefix, config, arch, jobs, dry_run=False):
    """Build static wxWidgets for Windows via its CMake build into the prefix."""
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


def build_liquid(prefix, jobs, dry_run=False, env=None, host=None):
    """Autotools-build liquid-dsp into the prefix (no distro package exists).

    Static only: nothing dlopens liquid-dsp, so a shared library would
    just be another runtime dependency for packaged builds to carry.
    """
    flags = ["--disable-shared", "--enable-static"]
    stamp = "lib-liquid"
    key = stamp_key(flags)
    if stamp_valid(prefix, stamp, key, adopt=not dry_run):
        log("liquid-dsp already built; skipping")
        return
    if os.path.exists(stamp_path(prefix, stamp)):
        log("liquid-dsp flags changed; rebuilding")
    if os.path.exists(stamp_path(prefix, "liquid")):
        log("liquid-dsp was shared; rebuilding static")
        if not dry_run:
            for stale in glob.glob(os.path.join(prefix, "lib", "libliquid.so*")):
                os.remove(stale)
            os.remove(stamp_path(prefix, "liquid"))
    src = os.path.join(prefix, "src", "liquid-dsp")
    git_clone(LIQUID_URL, src, dry_run)
    cmd = ["./configure", "--prefix=%s" % prefix] + flags
    if host is not None:
        cmd += ["--host=%s" % host]
    run(["sh", "bootstrap.sh"], cwd=src, dry_run=dry_run, env=env)
    run(cmd, cwd=src, dry_run=dry_run, env=env)
    run(["make", "-j%s" % jobs], cwd=src, dry_run=dry_run, env=env)
    run(["make", "install"], cwd=src, dry_run=dry_run, env=env)
    mark_done(prefix, stamp, dry_run, key)


def soapy_build_dir(prefix):
    """Out-of-source build directory for SoapySDR itself."""
    return os.path.join(prefix, "src", "SoapySDR", "build")


def build_soapy(prefix, config, jobs, arch=None, dry_run=False, env=None):
    """Build pinned SoapySDR into the prefix; MSVC when arch is given."""
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
    run(cmd, dry_run=dry_run, env=env)
    if arch:
        run(["cmake", "--build", soapy_build_dir(prefix), "--config", config,
             "--parallel", str(jobs)], dry_run=dry_run, env=env)
        run(["cmake", "--install", soapy_build_dir(prefix), "--config", config],
            dry_run=dry_run, env=env)
    else:
        run(["cmake", "--build", soapy_build_dir(prefix), "-j", str(jobs)],
            dry_run=dry_run, env=env)
        run(["cmake", "--install", soapy_build_dir(prefix)], dry_run=dry_run,
            env=env)
    mark_done(prefix, "soapy", dry_run)


def find_soapysdr_cmake(prefix, dry_run=False):
    """Locate the installed SoapySDRConfig.cmake for the macOS preset."""
    matches = glob.glob(os.path.join(prefix, "**", "SoapySDRConfig.cmake"), recursive=True)
    if matches:
        return os.path.dirname(matches[0])
    if dry_run:
        return os.path.join(prefix, "lib", "cmake", "SoapySDR")
    sys.exit("error: SoapySDRConfig.cmake not found under %s" % prefix)


def fetch_hamlib_windows(prefix, arch, dry_run=False):
    """Unpack the Hamlib Windows archive and build its MSVC import library.

    Also comments out the POSIX-only sys/time.h include in rig.h, which
    MSVC cannot provide.
    """
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


def ensure_nsis(dry_run=False):
    """Make makensis available, falling back to the default install location.

    The NSIS installer does not add itself to PATH, so check the standard
    Program Files locations before reporting the tool missing.
    """
    if shutil.which("makensis") is not None:
        return
    for base in (os.environ.get("ProgramFiles(x86)", ""),
                 os.environ.get("ProgramFiles", "")):
        candidate = os.path.join(base, "NSIS")
        if os.path.isfile(os.path.join(candidate, "makensis.exe")):
            log("+ using NSIS at %s" % candidate)
            if not dry_run:
                os.environ["PATH"] = candidate + os.pathsep + os.environ.get("PATH", "")
            return
    if dry_run:
        log("would require tool 'makensis'")
    else:
        sys.exit("error: required tool 'makensis' not found on PATH")


def fetch_vc_redist(prefix, arch, dry_run=False):
    """Download the matching VC++ redistributable for the NSIS installer."""
    name = "vc_redist.%s.exe" % arch
    dest = os.path.join(prefix, name)
    if not os.path.exists(dest):
        download("https://aka.ms/vs/17/release/%s" % name, dest, dry_run)
    return dest


PTHREADS_VERSION = "3.0.3.1"
PTHREADS_URL = "https://github.com/GerHobbelt/pthread-win32/archive/refs/tags/v%s.zip"
FFTW_VERSION = "3.3.5"


def build_libusb_windows(prefix, config, arch, dry_run=False):
    """Build libusb from its vendored MSVC solution; return headers and lib."""
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
    """Build pthread-win32 and generate a PThreads4W config shim for CMake.

    Upstream find modules expect PThreads4W target names that the old
    pthread-win32 package does not ship, hence the generated shim.
    """
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
    """Lay out libusb headers and binaries in the shape finders expect."""
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
    """Run the prebuilt Boost installer into the prefix (UHD needs it).

    Boost binary installers are per-MSVC-version, so BOOST_MSVC must match
    the compiler; see boost_lib_dir for locating the matching lib dir.
    """
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


def find_sdrplay_api(arch, dry_run=False):
    """Locate the vendor SDRplay API 3.x install, or None when absent.

    The API cannot be redistributed, so it is never downloaded: the
    registry key its installer writes is honored, with the default
    install path as fallback. Both the header and the arch-matching DLL
    must exist for the install to count.
    """
    default = os.path.join("C:\\", "Program Files", "SDRplay", "API")
    if dry_run:
        return default
    api_dir = default
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                            "SOFTWARE\\SDRplay\\Service\\API") as key:
            api_dir, _ = winreg.QueryValueEx(key, "Install_Dir")
    except OSError:
        pass
    subdir = "x64" if arch == "x64" else "x86"
    if (os.path.isfile(os.path.join(api_dir, "inc", "sdrplay_api.h"))
            and os.path.isfile(os.path.join(api_dir, subdir, "sdrplay_api.dll"))):
        return api_dir
    return None


def patch_uhd_windows(src, dry_run=False):
    """Provide struct timeval for UHD's USB transport on MSVC.

    Current libusb headers no longer define struct timeval on Windows, so
    winsock2.h (which does) is included first. Idempotent via the marker.
    """
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


def boost_lib_dir(boost_root, arch, dry_run=False):
    """Resolve the Boost library dir, preferring the pinned MSVC version.

    The prebuilt layout is lib{32,64}-msvc-<ver>; fall back to the newest
    match so a newer installer keeps working, and to the constructed
    default when previewing or when nothing is installed yet.
    """
    tag = "64" if arch == "x64" else "32"
    if not dry_run:
        matches = sorted(glob.glob(
            os.path.join(boost_root, "lib%s-msvc-*" % tag)))
        for path in matches:
            if os.path.basename(path) == "lib%s-msvc-%s" % (tag, BOOST_MSVC):
                return path
        if matches:
            return matches[-1]
    return os.path.join(boost_root, "lib%s-msvc-%s" % (tag, BOOST_MSVC))


def patch_limesuite_rpath(src, dry_run=False):
    """Let our -D install rpath survive the LimeSuite build.

    LimeSuite unconditionally SETs CMAKE_INSTALL_RPATH to the install
    prefix lib dir, discarding the -D value soapy_module passes. Wrap it
    so an explicitly provided rpath (the $ORIGIN one) is honored, leaving
    default behavior untouched. Idempotent via the marker comment.
    """
    target = os.path.join(src, "CMakeLists.txt")
    marker = "# CubicSDR build: honor a pre-set install rpath"
    needle = 'SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")'
    if dry_run:
        log("+ patch %s to honor pre-set install rpath" % target)
        return
    with open(target, "r", newline="") as handle:
        text = handle.read()
    if marker in text or needle not in text:
        return
    log("patching %s to honor pre-set install rpath" % target)
    text = text.replace(
        needle,
        marker + '\nif(NOT CMAKE_INSTALL_RPATH)\n' + needle +
        '\nendif()', 1)
    with open(target, "w", newline="") as handle:
        handle.write(text)


def ensure_mako(dry_run=False):
    """UHD code generation needs Python mako; install it when missing."""
    if dry_run:
        log("+ ensure python mako module")
        return
    check = subprocess.run([sys.executable, "-c", "import mako"],
                           capture_output=True)
    if check.returncode != 0:
        run([sys.executable, "-m", "pip", "install", "--user", "mako"],
            dry_run=dry_run)


def fetch_fftw_windows(prefix, arch, dry_run=False):
    """Fetch prebuilt FFTW and create its MSVC import library from the .def."""
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
    """Build one C library dependency; the stamp key covers flags and source.

    Reconfigures on every run (cheap) but recompiles only when the stamp
    is missing or the flags changed since the last build.
    """
    stamp = "lib-" + name
    key = stamp_key([extra or [], subdir, branch])
    if stamp_valid(prefix, stamp, key, adopt=not dry_run):
        log("%s already built; skipping" % name)
        return
    if os.path.exists(stamp_path(prefix, stamp)):
        log("%s flags changed; rebuilding" % name)
    src = os.path.join(prefix, "src", name)
    git_clone(url, src, dry_run, branch=branch)
    cmake_build(os.path.join(src, subdir) if subdir else src,
                os.path.join(prefix, "build", name), config, jobs, prefix,
                arch, extra, dry_run)
    mark_done(prefix, stamp, dry_run, key)


def build_modules_windows(prefix, config, arch, jobs, selected, dry_run=False):
    """Build the selected C libraries and SoapySDR modules for Windows.

    Shared C libraries (libusb, pthreads, FFTW, device libraries) are
    built first so each Soapy module can point at them; pure modules are
    handled by the repos/names tables at the end.
    """
    selected = [key for key in SUPPORTED_MODULES if key in set(selected)]
    sdrplay_api = find_sdrplay_api(arch, dry_run) if "sdrplay" in selected else None
    if "sdrplay" in selected and sdrplay_api is None:
        log("SDRplay API not found; skipping sdrplay module "
            "(install SDRplay API 3.x to enable it)")
        selected = [key for key in selected if key != "sdrplay"]
    if not selected:
        return
    # Only pay for libusb/pthreads when a selected module links against them.
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
        boost_libdir = boost_lib_dir(boost_root, arch, dry_run).replace("\\", "/")
        boost_libpath = "/LIBPATH:%s" % boost_libdir
        ensure_mako(dry_run)
        git_clone("https://github.com/EttusResearch/uhd",
                  os.path.join(prefix, "src", "uhd"), dry_run, branch=UHD_TAG)
        patch_uhd_windows(os.path.join(prefix, "src", "uhd"), dry_run)
        build_clib_windows(
            "uhd", "https://github.com/EttusResearch/uhd",
            prefix, config, arch, jobs,
            ["-DBOOST_ROOT:PATH=%s" % boost_root,
             "-DBOOST_LIBRARYDIR:PATH=%s" % boost_lib_dir(boost_root, arch, dry_run),
             # USB transport (B100/B200/USRP1) needs libusb; the /LIBPATH
             # entries let MSVC auto-linking find the Boost binary libs,
             # which UHD's own Boost detection leaves off the link line.
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
        # bladeRF installs its DLL beside its lib; mirror it to prefix bin
        # so the generic runtime staging picks it up.
        for dll in glob.glob(os.path.join(prefix, "lib", "bladeRF*.dll")):
            log("+ stage %s to prefix bin" % os.path.basename(dll))
            if not dry_run:
                shutil.copy(dll, os.path.join(prefix, "bin"))
    if "sdrplay" in selected:
        # The vendor runtime DLL lives outside the prefix; stage it to
        # prefix bin so it lands beside the exe with everything else.
        api_dll = os.path.join(sdrplay_api,
                               "x64" if arch == "x64" else "x86",
                               "sdrplay_api.dll")
        log("+ stage %s to prefix bin" % os.path.basename(api_dll))
        if not dry_run:
            os.makedirs(os.path.join(prefix, "bin"), exist_ok=True)
            shutil.copy(api_dll, os.path.join(prefix, "bin"))
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
        # SoapyUHD relies on the same Boost auto-linking as UHD itself, so
        # it needs the same BOOST_ROOT/LIBPATH hints to configure and link.
        libs["uhd"] = ["-DBOOST_ROOT:PATH=%s" % boost_root,
                       "-DBOOST_LIBRARYDIR:PATH=%s" % boost_libdir.replace("/", "\\"),
                       "-DCMAKE_SHARED_LINKER_FLAGS=%s" % boost_libpath,
                       "-DCMAKE_MODULE_LINKER_FLAGS=%s" % boost_libpath,
                       "-DCMAKE_EXE_LINKER_FLAGS=%s" % boost_libpath]
    # Pure Soapy modules: CMake project name and repo per module key (see
    # SOAPY_REPOS/SOAPY_NAMES), with optional per-module flags from libs.
    for key in selected:
        if key not in SOAPY_REPOS:
            continue
        soapy_module(SOAPY_NAMES[key], SOAPY_REPOS[key], prefix, config, jobs, arch,
                     dry_run, extra=libs.get(key))


def sys_header(candidates):
    """First existing header path from a list of absolute candidates."""
    for candidate in candidates:
        if os.path.isfile(candidate):
            return candidate
    return None


def first_glob(patterns):
    """First match across glob patterns, or None when nothing matches."""
    for pattern in patterns:
        matches = sorted(glob.glob(pattern))
        if matches:
            return matches[0]
    return None


def build_libusb_linux(prefix, jobs, from_source, dry_run=False):
    """Provide libusb on Linux: system package or autotools source build."""
    header = sys_header(["/usr/include/libusb-1.0/libusb.h",
                         "/usr/local/include/libusb-1.0/libusb.h"])
    if header and not from_source:
        log("using system libusb")
        lib = sys_lib(["/usr/lib/x86_64-linux-gnu/libusb-1.0.so",
                       "/usr/lib/aarch64-linux-gnu/libusb-1.0.so",
                       "/usr/local/lib/libusb-1.0.so",
                       "/usr/lib/libusb-1.0.so"])
        return os.path.dirname(os.path.dirname(header)), lib
    if os.path.exists(stamp_path(prefix, "lib-libusb")):
        log("libusb already built; skipping")
    else:
        src = os.path.join(prefix, "src", "libusb")
        git_clone("https://github.com/libusb/libusb", src, dry_run)
        run(["sh", "autogen.sh"], cwd=src, dry_run=dry_run)
        run(["./configure", "--prefix=%s" % prefix], cwd=src, dry_run=dry_run)
        run(["make", "-j%s" % jobs], cwd=src, dry_run=dry_run)
        run(["make", "install"], cwd=src, dry_run=dry_run)
        mark_done(prefix, "lib-libusb", dry_run)
    return (os.path.join(prefix, "include"),
            os.path.join(prefix, "lib", "libusb-1.0.so"))


def build_fftw_linux(prefix, jobs, from_source, dry_run=False):
    """Provide single-precision FFTW: system package or source build."""
    header = sys_header(["/usr/include/fftw3.h",
                         "/usr/local/include/fftw3.h"])
    if header and not from_source:
        log("using system FFTW")
        return os.path.dirname(header), sys_lib([
            "/usr/lib/x86_64-linux-gnu/libfftw3f.so",
            "/usr/lib/aarch64-linux-gnu/libfftw3f.so",
            "/usr/local/lib/libfftw3f.so", "/usr/lib/libfftw3f.so"])
    dest = os.path.join(prefix, "src", "fftw-%s" % FFTW_VERSION)
    if os.path.exists(stamp_path(prefix, "lib-fftw")):
        log("fftw already built; skipping")
    else:
        download("https://fftw.org/pub/fftw/fftw-%s.tar.gz" % FFTW_VERSION,
                 dest + ".tar.gz", dry_run)
        if not dry_run:
            with tarfile.open(dest + ".tar.gz") as archive:
                archive.extractall(os.path.join(prefix, "src"))
        run(["./configure", "--enable-float", "--enable-shared",
             "--prefix=%s" % prefix], cwd=dest, dry_run=dry_run)
        run(["make", "-j%s" % jobs], cwd=dest, dry_run=dry_run)
        run(["make", "install"], cwd=dest, dry_run=dry_run)
        mark_done(prefix, "lib-fftw", dry_run)
    return os.path.join(prefix, "include"), os.path.join(prefix, "lib", "libfftw3f.so")


def normalize_libusb_headers(usb_inc, prefix, dry_run=False):
    """Mirror libusb headers under prefix/include in both layouts.

    Projects disagree on <libusb.h> vs <libusb-1.0/libusb.h>; the prefix
    copy provides both so every finder is satisfied. Returns the prefix
    include dir for all LIBUSB_*_INCLUDE_* flags.
    """
    dest_top = os.path.join(prefix, "include")
    subdir = os.path.join(usb_inc, "libusb-1.0", "libusb.h")
    bare_src = os.path.join(usb_inc, "libusb.h")
    if not os.path.isfile(bare_src):
        # Synthesize the bare header from the subdir one; libusb.h is
        # self-contained and does not depend on its location.
        bare_src = subdir
    pairs = [(subdir, os.path.join(dest_top, "libusb-1.0", "libusb.h")),
             (bare_src, os.path.join(dest_top, "libusb.h"))]
    if dry_run:
        log("+ mirror libusb headers into %s" % dest_top)
        return dest_top
    for src, dest in pairs:
        if os.path.isfile(src) and not os.path.isfile(dest):
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            shutil.copy(src, dest)
    return dest_top


def find_sdrplay_api_linux(dry_run=False):
    """Locate a Linux SDRplay API install by its header, or None."""
    if dry_run:
        return "/usr/include"
    if sys_header(["/usr/include/sdrplay_api.h",
                   "/usr/local/include/sdrplay_api.h"]):
        return True
    return None


def build_modules_linux(prefix, config, jobs, selected, from_source,
                        dry_run=False):
    """Build the selected C libraries and SoapySDR modules for Linux.

    Generic dependencies (libusb, FFTW, libxml2, zstd) prefer system
    packages unless from_source forces source builds; device libraries
    always build from source. Mirrors build_modules_windows.
    """
    selected = [key for key in SUPPORTED_MODULES if key in set(selected)]
    if "sdrplay" in selected and find_sdrplay_api_linux(dry_run) is None:
        log("SDRplay API not found; skipping sdrplay module "
            "(install the SDRplay API to enable it)")
        selected = [key for key in selected if key != "sdrplay"]
    if not selected:
        return
    need_usb = set(selected) & {"rtlsdr", "airspy", "airspyhf", "hackrf",
                               "limesuite", "bladerf", "pluto", "uhd"}
    if need_usb:
        usb_inc, usb_lib = build_libusb_linux(prefix, jobs, from_source, dry_run)
        usb_inc = normalize_libusb_headers(usb_inc, prefix, dry_run)
    else:
        usb_inc = usb_lib = ""
    usb_defs = ["-DLIBUSB_INCLUDE_DIRS:PATH=%s" % usb_inc,
                "-DLIBUSB_LIBRARIES:PATH=%s" % usb_lib]
    usb_single_defs = ["-DLIBUSB_INCLUDE_DIR:PATH=%s" % usb_inc,
                       "-DLIBUSB_LIBRARIES:PATH=%s" % usb_lib]
    if "hackrf" in selected:
        fftw_inc, fftw_lib = build_fftw_linux(prefix, jobs, from_source, dry_run)
        build_clib_windows(
            "hackrf", "https://github.com/greatscottgadgets/hackrf",
            prefix, config, None, jobs,
            ["-DFFTW_LIBRARIES=%s" % fftw_lib,
             "-DFFTW_INCLUDES=%s" % fftw_inc,
             # udev rules target /etc and need root; the AppImage/CI
             # install must stay inside the prefix.
             "-DINSTALL_UDEV_RULES=OFF",
             "-DCMAKE_PREFIX_PATH=%s" % prefix] + usb_single_defs,
            subdir="host", dry_run=dry_run)
    if "rtlsdr" in selected:
        build_clib_windows(
            "librtlsdr", "https://github.com/rtlsdrblog/rtl-sdr-blog",
            prefix, config, None, jobs,
            # Detach the DVB kernel driver so the dongle opens even when
            # dvb_usb_rtl28xxu has claimed it (Debian builds enable this).
            ["-DCMAKE_PREFIX_PATH=%s" % prefix,
             "-DDETACH_KERNEL_DRIVER=ON"] + usb_defs,
            dry_run=dry_run)
    if "airspy" in selected:
        build_clib_windows(
            "airspyone_host", "https://github.com/airspy/airspyone_host",
            prefix, config, None, jobs,
            ["-DCMAKE_PREFIX_PATH=%s" % prefix] + usb_single_defs,
            dry_run=dry_run)
    if "airspyhf" in selected:
        build_clib_windows(
            "airspyhf", "https://github.com/airspy/airspyhf",
            prefix, config, None, jobs,
            ["-DCMAKE_PREFIX_PATH=%s" % prefix] + usb_single_defs,
            dry_run=dry_run)
    if "limesuite" in selected:
        git_clone("https://github.com/myriadrf/LimeSuite",
                  os.path.join(prefix, "src", "LimeSuite"), dry_run)
        patch_limesuite_rpath(os.path.join(prefix, "src", "LimeSuite"), dry_run)
        build_clib_windows(
            "LimeSuite", "https://github.com/myriadrf/LimeSuite",
            prefix, config, None, jobs,
            ["-DCMAKE_PREFIX_PATH=%s" % prefix,
             # This build produces both prefix/lib/*.so ($ORIGIN) and the
             # Soapy module two dirs deeper ($ORIGIN/../../lib).
             "-DCMAKE_INSTALL_RPATH=$ORIGIN;$ORIGIN/../../lib",
             "-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON",
             "-DLIBUSB_INCLUDE_DIRS:PATH=%s" % usb_inc,
             "-DLIBUSB_LIBRARIES:PATH=%s" % usb_lib,
             "-DLIBUSB_INCLUDE_DIR:PATH=%s" % usb_inc,
             "-DLIBUSB_LIBRARY:PATH=%s" % usb_lib],
            dry_run=dry_run)
    if "pluto" in selected:
        zstd_inc = sys_header(["/usr/include/zstd.h",
                               "/usr/local/include/zstd.h"])
        if zstd_inc and not from_source:
            log("using system zstd")
        else:
            build_clib_windows(
                "zstd", "https://github.com/facebook/zstd",
                prefix, config, None, jobs,
                ["-DZSTD_BUILD_PROGRAMS=OFF", "-DZSTD_BUILD_TESTS=OFF"],
                subdir="build/cmake", dry_run=dry_run)
        xml_inc = sys_header(["/usr/include/libxml2/libxml/parser.h",
                              "/usr/local/include/libxml2/libxml/parser.h"])
        if xml_inc and not from_source:
            log("using system libxml2")
        else:
            build_clib_windows(
                "libxml2", "https://github.com/GNOME/libxml2",
                prefix, config, None, jobs,
                ["-DLIBXML2_WITH_ICONV=OFF", "-DLIBXML2_WITH_ZLIB=OFF",
                 "-DLIBXML2_WITH_LZMA=OFF", "-DLIBXML2_WITH_PYTHON=OFF",
                 "-DLIBXML2_WITH_PROGRAMS=OFF"],
                dry_run=dry_run)
        build_clib_windows(
            "libiio", "https://github.com/analogdevicesinc/libiio",
            prefix, config, None, jobs,
            ["-DCMAKE_PREFIX_PATH=%s" % prefix,
             "-DLIBUSB_INCLUDE_DIR:PATH=%s" % usb_inc,
             "-DLIBUSB_LIBRARIES:PATH=%s" % usb_lib,
             # udev rule targets /lib and needs root; keep install in-prefix.
             "-DINSTALL_UDEV_RULE=OFF",
             # DNS-SD discovery needs Avahi; USB Pluto use does not.
             "-DHAVE_DNS_SD=OFF",
             "-DWITH_IIOD=OFF"],
            branch="v0.26", dry_run=dry_run)
        build_clib_windows(
            "libad9361", "https://github.com/analogdevicesinc/libad9361-iio",
            prefix, config, None, jobs,
            ["-DCMAKE_PREFIX_PATH=%s" % prefix,
             "-DLIBIIO_INCLUDEDIR:PATH=%s" % os.path.join(prefix, "include"),
             "-DLIBIIO_LIBRARIES:PATH=%s" % os.path.join(prefix, "lib", "libiio.so"),
             "-DBUILD_TESTS=OFF"],
            dry_run=dry_run)
    if "uhd" in selected:
        ensure_mako(dry_run)
        git_clone("https://github.com/EttusResearch/uhd",
                  os.path.join(prefix, "src", "uhd"), dry_run, branch=UHD_TAG)
        build_clib_windows(
            "uhd", "https://github.com/EttusResearch/uhd",
            prefix, config, None, jobs,
            ["-DLIBUSB_INCLUDE_DIRS:PATH=%s" % usb_inc,
             "-DLIBUSB_LIBRARIES:PATH=%s" % usb_lib,
             "-DUHD_IMAGES_DOWNLOAD=ON",
             "-DENABLE_PYTHON_API=OFF", "-DENABLE_EXAMPLES=OFF",
             "-DENABLE_TESTS=OFF"],
            subdir="host", branch=UHD_TAG, dry_run=dry_run)
    if "bladerf" in selected:
        build_clib_windows(
            "bladeRF", "https://github.com/Nuand/bladeRF",
            prefix, config, None, jobs,
            ["-DCMAKE_PREFIX_PATH=%s" % prefix] + usb_single_defs +
            [# udev rules and shell completions target /etc and /usr/share
             # and need root; keep the install inside the prefix.
             "-DINSTALL_UDEV_RULES=OFF",
             "-DINSTALL_SHELL_COMPLETIONS=OFF",
             "-DTREAT_WARNINGS_AS_ERRORS=OFF"],
            subdir="host", dry_run=dry_run)
    libs = {
        "rtlsdr": (["-DRTLSDR_LIBRARY:PATH=%s" % find_prefix_file(prefix, "librtlsdr.so"),
                    "-DRTLSDR_INCLUDE_DIR:PATH=%s" % header_dir(prefix, "rtl-sdr.h", dry_run)]
                   if not dry_run else
                   ["-DRTLSDR_LIBRARY:PATH=librtlsdr.so",
                    "-DRTLSDR_INCLUDE_DIR:PATH=%s" % header_dir(prefix, "rtl-sdr.h", dry_run)]),
        "airspy": (["-DLIBAIRSPY_LIBRARIES:PATH=%s" % find_prefix_file(prefix, "libairspy.so"),
                    "-DLIBAIRSPY_INCLUDE_DIRS:PATH=%s" % header_dir(prefix, "airspy.h", dry_run)]
                   if not dry_run else
                   ["-DLIBAIRSPY_LIBRARIES:PATH=libairspy.so",
                    "-DLIBAIRSPY_INCLUDE_DIRS:PATH=%s" % header_dir(prefix, "airspy.h", dry_run)]),
        "airspyhf": (["-DLIBAIRSPYHF_LIBRARIES:PATH=%s" % find_prefix_file(prefix, "libairspyhf.so"),
                      "-DLIBAIRSPYHF_INCLUDE_DIRS:PATH=%s" % header_dir(prefix, "airspyhf.h", dry_run)]
                     if not dry_run else
                     ["-DLIBAIRSPYHF_LIBRARIES:PATH=libairspyhf.so",
                      "-DLIBAIRSPYHF_INCLUDE_DIRS:PATH=%s" % header_dir(prefix, "airspyhf.h", dry_run)]),
        "hackrf": (["-DLIBHACKRF_LIBRARY:PATH=%s" % find_prefix_file(prefix, "libhackrf.so"),
                    "-DLIBHACKRF_INCLUDE_DIR:PATH=%s" % header_dir(prefix, "hackrf.h", dry_run)]
                   if not dry_run else
                   ["-DLIBHACKRF_LIBRARY:PATH=libhackrf.so",
                    "-DLIBHACKRF_INCLUDE_DIR:PATH=%s" % header_dir(prefix, "hackrf.h", dry_run)]),
    }
    if "audio" in selected:
        # SoapyAudio backends: PulseAudio is the desktop default; ALSA and
        # JACK cover non-Pulse and pro-audio setups. OSS has no link
        # library on Linux, so it stays off.
        if not dry_run:
            alsa_inc = sys_header(["/usr/include/alsa/asoundlib.h",
                                   "/usr/local/include/alsa/asoundlib.h"])
            if alsa_inc is None:
                sys.exit("error: alsa/asoundlib.h not found; install ALSA development files (e.g. libasound2-dev)")
            jack_inc = sys_header(["/usr/include/jack/jack.h",
                                   "/usr/local/include/jack/jack.h"])
            if jack_inc is None:
                sys.exit("error: jack/jack.h not found; install JACK development files (e.g. libjack-jackd2-dev)")
            jack_lib = first_glob(["/usr/lib/x86_64-linux-gnu/libjack.so",
                                   "/usr/lib/aarch64-linux-gnu/libjack.so",
                                   "/usr/local/lib/libjack.so",
                                   "/usr/lib/libjack.so"])
            if jack_lib is None:
                sys.exit("error: libjack.so not found; install JACK development files (e.g. libjack-jackd2-dev)")
            jack_inc = os.path.dirname(jack_inc)
        else:
            jack_inc = "/usr/include"
            jack_lib = "libjack.so"
        libs["audio"] = ["-DUSE_AUDIO_PULSE=ON",
                         "-DUSE_AUDIO_ALSA=ON",
                         "-DUSE_AUDIO_JACK=ON",
                         "-DJACK_INCLUDE_DIRS:PATH=%s" % jack_inc,
                         "-DJACK_LIBRARIES:PATH=%s" % jack_lib]
    for key in selected:
        if key not in SOAPY_REPOS:
            continue
        soapy_module(SOAPY_NAMES[key], SOAPY_REPOS[key], prefix, config, jobs, None,
                     dry_run, extra=libs.get(key))


def system_wx_config():
    """wx-config on PATH, else the versioned config script distros ship."""
    found = shutil.which("wx-config")
    if found:
        return found
    return first_glob(["/usr/lib/*/wx/config/*",
                       "/usr/local/lib/*/wx/config/*"])


def system_soapy_found():
    """True when a distro SoapySDR is usable: tool or CMake config present.

    The utility binary lives in a separate package from the dev files, so
    either one counts as evidence of an installed SoapySDR.
    """
    if shutil.which("SoapySDRUtil") is not None:
        return True
    return first_glob(["/usr/lib/*/cmake/SoapySDR/SoapySDRConfig*.cmake",
                       "/usr/share/cmake/SoapySDR/SoapySDRConfig*.cmake",
                       "/usr/local/lib/*/cmake/SoapySDR/SoapySDRConfig*.cmake",
                       "/usr/local/share/cmake/SoapySDR/SoapySDRConfig*.cmake"]) is not None


def build_linux(args, dry_run=False, preset=None):
    """Full Linux build: deps, Soapy modules, then the app preset.

    System wxWidgets/SoapySDR are preferred because building wx from
    source dominates the build time; liquid-dsp is always built since no
    distro packages it. Modules install into the prefix; the AppImage
    flow stages them into the AppDir instead (see build_appimage).
    """
    for prog in ["cmake", "git", "make"]:
        require(prog, dry_run)
    prefix = os.path.abspath(args.deps_prefix or default_prefix())
    if not args.skip_deps:
        if args.build_deps:
            wx_config = build_wx_linux(prefix, args.jobs, dry_run)
            build_liquid(prefix, args.jobs, dry_run)
            build_soapy(prefix, args.config, args.jobs, dry_run=dry_run)
        else:
            if dry_run:
                wx_config = "wx-config"
                log("would use system wxWidgets and SoapySDR")
            else:
                wx_config = system_wx_config()
                missing = []
                if wx_config is None:
                    missing.append("wxWidgets (libwxgtk3.x-gtk3-dev)")
                if not system_soapy_found():
                    missing.append("SoapySDR (libsoapysdr-dev)")
                if missing:
                    sys.exit("error: system %s not found; install the dev "
                             "packages (see the apt list in "
                             ".circleci/config.yml) or pass --build-deps"
                             % " and ".join(missing))
                log("using system wxWidgets and SoapySDR")
            build_liquid(prefix, args.jobs, dry_run)
        build_modules_linux(prefix, args.config, args.jobs,
                            selected_modules(args.modules), args.build_deps,
                            dry_run)
    else:
        wx_config = (system_wx_config() if not dry_run else "wx-config") or \
            os.path.join(prefix, "wx", "bin", "wx-config")
    if not dry_run and shutil.which(wx_config) is None \
            and not os.path.exists(wx_config):
        sys.exit("error: %s missing; run without --skip-deps first" % wx_config)
    if preset is None:
        preset = "linux-deb" if args.package == "deb" else "linux"
    run(["cmake", "--preset", preset, "-DCMAKE_BUILD_TYPE=%s" % args.config],
        dry_run=dry_run,
        env={"CUBICSDR_WXCONFIG": wx_config, "CMAKE_PREFIX_PATH": prefix})
    run(["cmake", "--build", "--preset", preset, "-j", str(args.jobs)], dry_run=dry_run)
    if args.package == "deb":
        run(["cpack", "-G", "DEB"], cwd=os.path.join(ROOT, "build", preset),
            dry_run=dry_run)


def build_windows(args, dry_run=False):
    """Full Windows build: deps, modules, app preset, runtime staging.

    --package nsis additionally configures the installer flags and runs
    CPack; --skip-app stops after module staging for fast module iteration.
    """
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
        ensure_nsis(dry_run)
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
    if not args.skip_app:
        run(["cmake", "--preset", preset] + extra, dry_run=dry_run,
            env={"CUBICSDR_WXWIN": wx_root, "CUBICSDR_SOAPY": prefix})
        run(["cmake", "--build", "--preset", preset, "--config", args.config,
             "-j", str(args.jobs)], dry_run=dry_run)
    out = os.path.join(ROOT, "build", preset,
                       "x64" if args.arch == "x64" else "x86")
    stage_runtime_windows(prefix, out, args.dry_run)
    if args.package == "nsis" and not args.skip_app:
        run(["cpack", "-G", "NSIS", "-C", args.config],
            cwd=os.path.join(ROOT, "build", preset), dry_run=dry_run)


def macos_build_flags(target):
    """Cross-compile support for macOS builds.

    Returns (env, host): extra -arch environment (empty for native builds,
    so local builds behave exactly as before) and the autoconf --host
    triplet (None for native builds).
    """
    native = platform.machine().replace("aarch64", "arm64")
    want = "native" if target in (None, "native") else \
        target.replace("aarch64", "arm64")
    if want == "native" or want == native:
        return {}, None
    arch = "-arch %s" % want
    env = {}
    for var in ("CFLAGS", "CXXFLAGS", "OBJCFLAGS", "OBJCXXFLAGS", "LDFLAGS"):
        existing = os.environ.get(var, "")
        env[var] = ("%s %s" % (existing, arch)).strip()
    return env, "%s-apple-darwin" % want


def build_macos(args, dry_run=False):
    """Full macOS build: Homebrew deps, source wx/liquid/Soapy, app preset."""
    # brew comes first; cmake arrives via brew, so it is required later.
    for prog in ["brew", "git", "make"]:
        require(prog, dry_run)
    prefix = os.path.abspath(args.deps_prefix or default_prefix())
    arch_env, host = macos_build_flags(args.macos_arch)
    if not args.skip_deps:
        run(["brew", "install", "automake", "cmake", "git", "autoconf",
             "libtool", "pkg-config", "wget", "libusb", "fftw", "hamlib"],
            dry_run=dry_run)
        require("cmake", dry_run)
        wx_config = build_wx_macos(prefix, args.jobs, dry_run, arch_env,
                                   host=host)
        build_liquid(prefix, args.jobs, dry_run, arch_env, host=host)
        build_soapy(prefix, args.config, args.jobs, dry_run=dry_run,
                    env=arch_env)
    else:
        require("cmake", dry_run)
        wx_config = os.path.join(prefix, "wx", "bin", "wx-config")
    remove_shared_liquid(prefix, dry_run)
    if not dry_run and not os.path.exists(wx_config):
        sys.exit("error: %s missing; run without --skip-deps first" % wx_config)
    soapy_cmake = find_soapysdr_cmake(prefix, dry_run)
    app_env = {"CUBICSDR_MACOS_PREFIX": prefix,
               "CUBICSDR_WXCONFIG": wx_config,
               "CUBICSDR_SOAPY_CMAKE": soapy_cmake,
               "CMAKE_PREFIX_PATH": prefix}
    app_env.update(arch_env)
    run(["cmake", "--preset", "macos", "-DCMAKE_BUILD_TYPE=%s" % args.config],
        dry_run=dry_run, env=app_env)
    run(["cmake", "--build", "--preset", "macos", "-j", str(args.jobs)],
        dry_run=dry_run, env=arch_env)
    if args.package == "dmg":
        run(["cpack", "-G", "DragNDrop"], cwd=os.path.join(ROOT, "build", "macos"),
            dry_run=dry_run)


def build_appimage(args, dry_run=False):
    """Build the Linux app, then bundle an AppDir and run linuxdeploy.

    Built with the linux-appimage preset so the app scans exe-relative
    modules/. Prefix libraries are staged into the AppDir and exported
    on LD_LIBRARY_PATH so from-source deps (SoapySDR, liquid-dsp)
    resolve during linuxdeploy's dependency pass.
    """
    require("cmake", dry_run)
    prefix = os.path.abspath(args.deps_prefix or default_prefix())
    preset = "linux-appimage"
    if not args.skip_deps:
        build_linux(args, dry_run, preset=preset)
    # Preset binary dirs are build/<preset name>; the appimage build never
    # shares the plain linux build tree.
    build_dir = os.path.join(ROOT, "build", preset)
    appdir = os.path.join(ROOT, "build", "AppDir")
    if not dry_run and os.path.isdir(appdir):
        shutil.rmtree(appdir)
    log("installing to %s" % appdir if not dry_run else "would install to %s" % appdir)
    if not dry_run:
        os.makedirs(os.path.join(appdir, "usr"), exist_ok=True)
    run(["cmake", "--install", build_dir,
         "--prefix", os.path.join(appdir, "usr")], dry_run=dry_run)
    # The desktop template carries an absolute Icon path for normal
    # installs; linuxdeploy needs the bare icon name matching --icon-file.
    staged_desktop = os.path.join(appdir, "usr", "share", "applications",
                                  "CubicSDR.desktop")
    if dry_run:
        log("+ point staged desktop file Icon at the bundled icon")
    else:
        with open(staged_desktop) as handle:
            desktop = handle.read()
        desktop = re.sub(r"(?m)^Icon=.*$", "Icon=CubicSDR", desktop)
        with open(staged_desktop, "w", newline="") as handle:
            handle.write(desktop)
    if dry_run:
        log("+ stage prefix libraries into %s"
            % os.path.join(appdir, "usr", "lib"))
        log("+ stage SoapySDR modules into %s"
            % os.path.join(appdir, "usr", "bin", "modules"))
        log("+ mirror GL fonts into %s"
            % os.path.join(appdir, "usr", "bin", "fonts"))
    else:
        libdir = os.path.join(appdir, "usr", "lib")
        os.makedirs(libdir, exist_ok=True)
        for lib in glob.glob(os.path.join(prefix, "lib", "*.so*")):
            if os.path.isfile(lib):
                shutil.copy(lib, libdir)
        # SoapyAudio links system audio libraries that linuxdeploy cannot
        # see (dlopened module); stage them so the module always loads.
        for pattern in ("libasound.so*", "libjack.so*"):
            for sysdir in ("/usr/lib/x86_64-linux-gnu",
                           "/usr/lib/aarch64-linux-gnu", "/usr/lib"):
                for syslib in glob.glob(os.path.join(sysdir, pattern)):
                    if os.path.isfile(syslib):
                        shutil.copy(syslib, libdir)
        # Exe-relative modules/ is what the bundled-mods build scans.
        moddir = os.path.join(appdir, "usr", "bin", "modules")
        os.makedirs(moddir, exist_ok=True)
        for mods in glob.glob(os.path.join(prefix, "lib", "SoapySDR", "modules*")):
            for mod in glob.glob(os.path.join(mods, "*.so")):
                shutil.copy(mod, moddir)
        # The GL font loader falls back to exe-relative fonts/; mirror the
        # installed fonts there as on Windows.
        fontdir = os.path.join(appdir, "usr", "bin", "fonts")
        os.makedirs(fontdir, exist_ok=True)
        for font in glob.glob(os.path.join(appdir, "usr", "share",
                                            "cubicsdr", "fonts", "*")):
            if os.path.isfile(font):
                shutil.copy(font, fontdir)
    tool = os.path.join(prefix, "linuxdeploy-x86_64.AppImage")
    if not os.path.exists(tool):
        download(LINUXDEPLOY_URL, tool, dry_run)
    if not dry_run:
        os.chmod(tool, 0o755)
    run([tool, "--appimage-extract-and-run", "--appdir", appdir,
         "--desktop-file", os.path.join(build_dir, "CubicSDR.desktop"),
         "--icon-file", os.path.join(ROOT, "src", "CubicSDR.png"),
         "--output", "appimage"], cwd=os.path.join(ROOT, "build"), dry_run=dry_run,
        env={"APPIMAGE_EXTRACT_AND_RUN": "1",
             "LD_LIBRARY_PATH": os.path.join(prefix, "lib")})


def clean(target, dry_run=False):
    """Remove the whole build tree, including the dependency prefix."""
    path = os.path.join(ROOT, "build")
    log("would remove %s" % path if dry_run else "removing %s" % path)
    if not dry_run:
        shutil.rmtree(path, ignore_errors=True)


def main(argv=None):
    """Parse args, validate the platform/package pairing, and dispatch."""
    args = parse_args(argv or sys.argv[1:])
    target = args.platform
    if target == "auto":
        target = {"Windows": "windows", "Linux": "linux",
                  "Darwin": "macos"}.get(platform.system())
        if target is None:
            sys.exit("error: unsupported host; pass --platform explicitly")
    if args.macos_arch != "native" and target != "macos":
        sys.exit("error: --macos-arch is macOS-only")
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
