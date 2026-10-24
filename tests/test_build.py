# Copyright (c) Charles J. Cliffe
# SPDX-License-Identifier: GPL-2.0+
"""Regression tests for build.py's dependency and module planning.

These run anywhere with a stock Python 3: they exercise pure planning
helpers and filesystem helpers against temporary directories, never the
network or a compiler. Run with: python -m unittest discover -s tests
"""
import os
import platform
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import build


class SelectedModulesTests(unittest.TestCase):
    def test_default_excludes_opt_outs(self):
        selected = build.selected_modules("default")
        self.assertIn("rtlsdr", selected)
        self.assertIn("audio", selected)
        self.assertNotIn("netsdr", selected)
        self.assertNotIn("uhd", selected)

    def test_all_includes_everything(self):
        self.assertEqual(set(build.selected_modules("all")),
                         set(build.SUPPORTED_MODULES))

    def test_none_is_empty(self):
        self.assertEqual(build.selected_modules("none"), [])

    def test_unknown_module_fails(self):
        with self.assertRaises(SystemExit):
            build.selected_modules("nope")


class MacosBuildFlagsTests(unittest.TestCase):
    def test_native_has_no_host(self):
        _, host = build.macos_build_flags("native")
        self.assertIsNone(host)

    def test_arch_is_always_pinned(self):
        for spec in ("native", "x86_64", "arm64"):
            env, _ = build.macos_build_flags(spec)
            self.assertTrue(env["CMAKE_OSX_ARCHITECTURES"])

    def test_cross_adds_arch_flags_and_host(self):
        # Exercise whichever arch is not native here so the cross path
        # runs on any machine.
        native = platform.machine().replace("aarch64", "arm64")
        target = "x86_64" if native != "x86_64" else "arm64"
        env, host = build.macos_build_flags(target)
        self.assertEqual(host, "%s-apple-darwin" % target)
        for var in ("CFLAGS", "CXXFLAGS", "OBJCFLAGS", "OBJCXXFLAGS",
                    "LDFLAGS"):
            self.assertIn("-arch %s" % target, env[var])


class NormalizeLiquidMacosTests(unittest.TestCase):
    def test_renames_archive_and_drops_shared(self):
        with tempfile.TemporaryDirectory() as prefix:
            lib = os.path.join(prefix, "lib")
            os.makedirs(lib)
            for name in ("libliquid.ar", "libliquid.dylib",
                         "libliquid.1.dylib", "libother.dylib"):
                with open(os.path.join(lib, name), "w") as handle:
                    handle.write("x")
            build.normalize_liquid_macos(prefix)
            remaining = sorted(os.listdir(lib))
        self.assertEqual(remaining, ["libliquid.a", "libother.dylib"])

    def test_dry_run_changes_nothing(self):
        with tempfile.TemporaryDirectory() as prefix:
            lib = os.path.join(prefix, "lib")
            os.makedirs(lib)
            before = ["libliquid.ar", "libliquid.dylib"]
            for name in before:
                with open(os.path.join(lib, name), "w") as handle:
                    handle.write("x")
            build.normalize_liquid_macos(prefix, dry_run=True)
            self.assertEqual(sorted(os.listdir(lib)), sorted(before))


class ScrubWxConfigTests(unittest.TestCase):
    def test_replaces_egrep_placeholder(self):
        with tempfile.TemporaryDirectory() as src:
            confdir = os.path.join(src, "lib", "wx", "config")
            os.makedirs(confdir)
            script = os.path.join(confdir, "osx-test")
            with open(script, "w") as handle:
                handle.write('EGREP="@EGREP@"\nOTHER="@FOO@"\n')
            build.scrub_wx_config(src)
            with open(script) as handle:
                text = handle.read()
        self.assertIn('EGREP="grep -E"', text)
        self.assertIn("@FOO@", text)


class MacosModulesDryRunTests(unittest.TestCase):
    def test_default_modules_plan_runs(self):
        with tempfile.TemporaryDirectory() as prefix:
            build.build_modules_macos(
                prefix, "Release", 2,
                build.selected_modules("default"), dry_run=True)

    def test_sdrplay_is_skipped(self):
        with tempfile.TemporaryDirectory() as prefix:
            build.build_modules_macos(
                prefix, "Release", 2, ["sdrplay", "audio"], dry_run=True)

    def test_darwin_modules_carry_prefix_rpath(self):
        from unittest import mock
        import io
        from contextlib import redirect_stdout
        with tempfile.TemporaryDirectory() as prefix:
            with mock.patch.object(build.platform, "system",
                                   return_value="Darwin"):
                buf = io.StringIO()
                with redirect_stdout(buf):
                    build.soapy_module(
                        "SoapyAudio",
                        "https://github.com/pothosware/SoapyAudio",
                        prefix, "Release", 2, None, True)
                self.assertIn("-DCMAKE_INSTALL_RPATH=", buf.getvalue())
                self.assertIn(os.path.join(prefix, "lib"), buf.getvalue())

    def test_libiio_disables_osx_framework(self):
        import io
        from contextlib import redirect_stdout
        with tempfile.TemporaryDirectory() as prefix:
            buf = io.StringIO()
            with redirect_stdout(buf):
                build.build_modules_macos(
                    prefix, "Release", 2, ["pluto"], dry_run=True)
            self.assertIn("-DOSX_FRAMEWORK=OFF", buf.getvalue())
            self.assertIn("-DOSX_PACKAGE=OFF", buf.getvalue())

    def test_cross_build_uses_source_deps(self):
        import io
        from contextlib import redirect_stdout
        with tempfile.TemporaryDirectory() as prefix:
            buf = io.StringIO()
            with redirect_stdout(buf):
                build.build_modules_macos(
                    prefix, "Release", 2,
                    ["rtlsdr", "hackrf", "bladerf", "pluto"],
                    dry_run=True, host="x86_64-apple-darwin",
                    env={"CMAKE_OSX_ARCHITECTURES": "x86_64"})
            out = buf.getvalue()
            self.assertIn("--host=x86_64-apple-darwin", out)
            self.assertIn("libusb-1.0.27.tar.bz2", out)
            self.assertIn("fftw-", out)
            self.assertIn("-DLIBUSB_SKIP_VERSION_CHECK=Yes", out)
            self.assertIn("-DLIBUSB_VERSION=1.0.27", out)
            self.assertIn("-b v0.3", out)
            self.assertNotIn("Cellar", out)
            self.assertIn("+ rewrite @rpath load commands in macOS "
                          "prefix libraries", out)

    def test_full_macos_plan_both_arches(self):
        for extra in ([], ["--macos-arch", "x86_64"]):
            with tempfile.TemporaryDirectory() as prefix:
                args = (["--platform", "macos", "--modules", "default",
                         "--package", "dmg", "--deps-prefix", prefix,
                         "--dry-run"] + extra)
                self.assertEqual(build.main(args), 0)

    def test_macos_arch_rejected_off_macos(self):
        with self.assertRaises(SystemExit):
            build.main(["--platform", "linux", "--macos-arch", "x86_64",
                        "--dry-run"])


class NormalizeIioHeadersTests(unittest.TestCase):
    def test_mirrors_flat_header(self):
        with tempfile.TemporaryDirectory() as prefix:
            inc = os.path.join(prefix, "include")
            os.makedirs(inc)
            with open(os.path.join(inc, "iio.h"), "w") as handle:
                handle.write("x")
            build.normalize_iio_headers(prefix)
            dest = os.path.join(inc, "iio", "iio.h")
            self.assertTrue(os.path.isfile(dest))
            with open(dest) as handle:
                self.assertEqual(handle.read(), "x")

    def test_leaves_existing_subdir_alone(self):
        with tempfile.TemporaryDirectory() as prefix:
            inc = os.path.join(prefix, "include")
            os.makedirs(os.path.join(inc, "iio"))
            with open(os.path.join(inc, "iio.h"), "w") as handle:
                handle.write("flat")
            with open(os.path.join(inc, "iio", "iio.h"), "w") as handle:
                handle.write("real")
            build.normalize_iio_headers(prefix)
            with open(os.path.join(inc, "iio", "iio.h")) as handle:
                self.assertEqual(handle.read(), "real")

    def test_mirrors_subdir_to_flat(self):
        with tempfile.TemporaryDirectory() as prefix:
            inc = os.path.join(prefix, "include")
            os.makedirs(os.path.join(inc, "iio"))
            with open(os.path.join(inc, "iio", "iio.h"), "w") as handle:
                handle.write("x")
            build.normalize_iio_headers(prefix)
            dest = os.path.join(inc, "iio.h")
            self.assertTrue(os.path.isfile(dest))
            with open(dest) as handle:
                self.assertEqual(handle.read(), "x")

    def test_missing_header_is_noop(self):
        with tempfile.TemporaryDirectory() as prefix:
            build.normalize_iio_headers(prefix)
            self.assertFalse(
                os.path.exists(os.path.join(prefix, "include", "iio")))

    def test_dry_run_logs_only(self):
        with tempfile.TemporaryDirectory() as prefix:
            build.normalize_iio_headers(prefix, dry_run=True)
            self.assertFalse(os.path.exists(os.path.join(prefix, "include")))


class LinkPathTests(unittest.TestCase):
    def test_adds_ldflags(self):
        env = build.with_link_path(
            {}, "/opt/homebrew/opt/libusb/lib")
        self.assertEqual(
            env["LDFLAGS"], "-L/opt/homebrew/opt/libusb/lib")

    def test_appends_to_existing(self):
        env = build.with_link_path(
            {"LDFLAGS": "-arch x86_64"}, "/x/lib")
        self.assertEqual(env["LDFLAGS"], "-arch x86_64 -L/x/lib")


class SoapyInstallCompleteTests(unittest.TestCase):
    def test_complete_install(self):
        with tempfile.TemporaryDirectory() as prefix:
            lib = os.path.join(prefix, "lib")
            os.makedirs(lib)
            for name in ("libSoapySDR.dylib", "libSoapySDR.0.8.dylib"):
                with open(os.path.join(lib, name), "w") as handle:
                    handle.write("x")
            self.assertTrue(build.soapy_install_complete(prefix))

    def test_missing_versioned_library(self):
        with tempfile.TemporaryDirectory() as prefix:
            lib = os.path.join(prefix, "lib")
            os.makedirs(lib)
            with open(os.path.join(lib, "libSoapySDR.dylib"), "w") as handle:
                handle.write("x")
            self.assertFalse(build.soapy_install_complete(prefix))

    def test_missing_dev_link(self):
        with tempfile.TemporaryDirectory() as prefix:
            lib = os.path.join(prefix, "lib")
            os.makedirs(lib)
            with open(os.path.join(lib, "libSoapySDR.0.8.dylib"),
                      "w") as handle:
                handle.write("x")
            self.assertFalse(build.soapy_install_complete(prefix))

    def test_dangling_symlink_counts_as_missing(self):
        try:
            with tempfile.TemporaryDirectory() as prefix:
                lib = os.path.join(prefix, "lib")
                os.makedirs(lib)
                with open(os.path.join(lib, "libSoapySDR.dylib"),
                          "w") as handle:
                    handle.write("x")
                os.symlink(os.path.join(lib, "nowhere.dylib"),
                           os.path.join(lib, "libSoapySDR.0.8.dylib"))
                self.assertFalse(build.soapy_install_complete(prefix))
        except (OSError, NotImplementedError):
            self.skipTest("symlinks unavailable")


class BuildSoapyTests(unittest.TestCase):
    def test_skip_when_install_complete(self):
        from unittest import mock
        with tempfile.TemporaryDirectory() as prefix:
            lib = os.path.join(prefix, "lib")
            os.makedirs(lib)
            for name in ("libSoapySDR.dylib", "libSoapySDR.0.8.dylib"):
                with open(os.path.join(lib, name), "w") as handle:
                    handle.write("x")
            os.makedirs(os.path.join(prefix, "stamps"))
            with open(build.stamp_path(prefix, "soapy"), "w") as handle:
                handle.write("ok\n")
            with mock.patch.object(build, "run") as run, \
                    mock.patch.object(build, "git_clone") as clone:
                build.build_soapy(prefix, "Release", 2)
            run.assert_not_called()
            clone.assert_not_called()

    def test_rebuild_when_install_incomplete(self):
        from unittest import mock
        with tempfile.TemporaryDirectory() as prefix:
            lib = os.path.join(prefix, "lib")
            os.makedirs(lib)
            with open(os.path.join(lib, "libSoapySDR.dylib"), "w") as handle:
                handle.write("x")
            os.makedirs(os.path.join(prefix, "stamps"))
            with open(build.stamp_path(prefix, "soapy"), "w") as handle:
                handle.write("ok\n")
            with mock.patch.object(build, "run") as run, \
                    mock.patch.object(build, "git_clone"):
                build.build_soapy(prefix, "Release", 2)
            self.assertTrue(run.called)
            with open(build.stamp_path(prefix, "soapy")) as handle:
                self.assertEqual(handle.read(), "ok\n")

    def test_dry_run_keeps_stamp(self):
        with tempfile.TemporaryDirectory() as prefix:
            os.makedirs(os.path.join(prefix, "stamps"))
            with open(build.stamp_path(prefix, "soapy"), "w") as handle:
                handle.write("ok\n")
            build.build_soapy(prefix, "Release", 2, dry_run=True)
            self.assertTrue(
                os.path.exists(build.stamp_path(prefix, "soapy")))


class VerifyBundleInputsTests(unittest.TestCase):
    def test_noop_off_darwin(self):
        from unittest import mock
        with mock.patch.object(build.platform, "system",
                               return_value="Linux"):
            with mock.patch.object(build.subprocess, "run") as run:
                build.verify_macos_bundle_inputs("/nonexistent")
        run.assert_not_called()

    def test_dry_run_logs_only(self):
        with tempfile.TemporaryDirectory() as prefix:
            build.verify_macos_bundle_inputs(prefix, dry_run=True)

    def test_no_app_is_an_error(self):
        from unittest import mock
        with tempfile.TemporaryDirectory() as prefix:
            with mock.patch.object(build.platform, "system",
                                   return_value="Darwin"):
                with self.assertRaises(SystemExit):
                    build.verify_macos_bundle_inputs(prefix)

    def _run_gate(self, prefix, app, lib_present):
        from types import SimpleNamespace
        from unittest import mock
        lib = os.path.join(prefix, "lib")
        os.makedirs(lib, exist_ok=True)
        if lib_present:
            with open(os.path.join(lib, "libSoapySDR.0.8.dylib"),
                      "w") as handle:
                handle.write("x")
        real_glob = build.glob.glob

        def fake_glob(pattern, recursive=False):
            if "CubicSDR.app" in pattern:
                return [app]
            return real_glob(pattern, recursive=recursive)

        def fake_run(cmd, **kwargs):
            if cmd[:2] == ["otool", "-l"]:
                return SimpleNamespace(
                    stdout="path %s (offset 12)\n" % lib, returncode=0)
            if cmd[:2] == ["otool", "-L"]:
                return SimpleNamespace(
                    stdout=("\t%s:\n"
                            "\t@rpath/libSoapySDR.0.8.dylib "
                            "(compatibility version 0.0.0)\n") % cmd[2],
                    returncode=0)
            raise AssertionError(cmd)

        with mock.patch.object(build.platform, "system",
                               return_value="Darwin"):
            with mock.patch.object(build.glob, "glob",
                                   side_effect=fake_glob):
                with mock.patch.object(build.subprocess, "run",
                                       side_effect=fake_run):
                    build.verify_macos_bundle_inputs(prefix)

    def test_pass_when_all_resolve(self):
        with tempfile.TemporaryDirectory() as prefix:
            app = os.path.join(prefix, "CubicSDR")
            with open(app, "w") as handle:
                handle.write("x")
            self._run_gate(prefix, app, True)

    def test_missing_library_is_an_error(self):
        with tempfile.TemporaryDirectory() as prefix:
            app = os.path.join(prefix, "CubicSDR")
            with open(app, "w") as handle:
                handle.write("x")
            with self.assertRaises(SystemExit):
                self._run_gate(prefix, app, False)


class SoapyModuleEnvTests(unittest.TestCase):
    def test_env_forwarded_to_cmake(self):
        from unittest import mock
        with tempfile.TemporaryDirectory() as prefix:
            env = {"CMAKE_OSX_ARCHITECTURES": "x86_64"}
            with mock.patch.object(build.platform, "system",
                                   return_value="Darwin"):
                with mock.patch.object(build, "git_clone"), \
                        mock.patch.object(build, "cmake_build") as cmake:
                    build.soapy_module("TestMod",
                                       "https://example.com/mod", prefix,
                                       "Release", 2, None, dry_run=False,
                                       extra=[], env=env)
            self.assertEqual(cmake.call_args[0][8], env)

    def test_arch_change_invalidates_stamp(self):
        from unittest import mock
        with tempfile.TemporaryDirectory() as prefix:
            cross = {"CMAKE_OSX_ARCHITECTURES": "x86_64"}
            with mock.patch.object(build.platform, "system",
                                   return_value="Darwin"):
                with mock.patch.object(build, "git_clone"), \
                        mock.patch.object(build, "cmake_build") as cmake:
                    build.soapy_module("TestMod",
                                       "https://example.com/mod", prefix,
                                       "Release", 2, None, dry_run=False,
                                       extra=[], env=cross)
                    self.assertEqual(cmake.call_count, 1)
                    build.soapy_module("TestMod",
                                       "https://example.com/mod", prefix,
                                       "Release", 2, None, dry_run=False,
                                       extra=[], env=cross)
                    self.assertEqual(cmake.call_count, 1)
                    build.soapy_module("TestMod",
                                       "https://example.com/mod", prefix,
                                       "Release", 2, None, dry_run=False,
                                       extra=[])
                    self.assertEqual(cmake.call_count, 2)


class NormalizeMacosLoadPathsTests(unittest.TestCase):
    def _gate(self, prefix, otool_stdout, lib_present=True):
        from types import SimpleNamespace
        from unittest import mock
        moddir = os.path.join(prefix, "lib", "SoapySDR", "modules0.8")
        os.makedirs(moddir, exist_ok=True)
        module = os.path.join(moddir, "librtlsdrSupport.so")
        with open(module, "w") as handle:
            handle.write("x")
        if lib_present:
            with open(os.path.join(prefix, "lib",
                                   "libSoapySDR.0.8.dylib"), "w") as handle:
                handle.write("x")
        calls = []

        def fake_subprocess(cmd, **kwargs):
            self.assertEqual(cmd[:2], ["otool", "-L"])
            return SimpleNamespace(stdout=otool_stdout, returncode=0)

        def fake_run(cmd, **kwargs):
            calls.append(cmd)

        with mock.patch.object(build.platform, "system",
                               return_value="Darwin"):
            with mock.patch.object(build.subprocess, "run",
                                   side_effect=fake_subprocess):
                with mock.patch.object(build, "run",
                                       side_effect=fake_run):
                    with mock.patch.object(build, "require"):
                        build.normalize_macos_load_paths(prefix)
        return calls

    def test_rewrites_rpath_ref(self):
        with tempfile.TemporaryDirectory() as prefix:
            os.makedirs(os.path.join(prefix, "lib"))
            out = "\t%s:\n\t@rpath/libSoapySDR.0.8.dylib (compat)\n"
            calls = self._gate(prefix, out % "librtlsdrSupport.so")
        changed = sorted(os.path.basename(call[4]) for call in calls)
        self.assertEqual(changed,
                         ["libSoapySDR.0.8.dylib", "librtlsdrSupport.so"])
        module_call = [call for call in calls
                       if call[4].endswith("librtlsdrSupport.so")]
        self.assertEqual(module_call[0][:3],
                         ["install_name_tool", "-change",
                          "@rpath/libSoapySDR.0.8.dylib"])
        self.assertTrue(module_call[0][3].endswith("libSoapySDR.0.8.dylib"))

    def test_transitive_build_tree_lib_is_rewritten(self):
        with tempfile.TemporaryDirectory() as prefix:
            os.makedirs(os.path.join(prefix, "lib"))
            builddir = os.path.join(prefix, "build", "libad9361", "src")
            os.makedirs(builddir)
            with open(os.path.join(builddir, "libad9361.dylib"),
                      "w") as handle:
                handle.write("x")
            with open(os.path.join(prefix, "lib", "libxml2.16.dylib"),
                      "w") as handle:
                handle.write("x")
            out = "\tlibad9361.dylib:\n\t@rpath/libxml2.16.dylib (compat)\n"
            calls = self._gate(prefix, out)
        changed = sorted(os.path.basename(call[4]) for call in calls)
        self.assertEqual(changed,
                         ["libSoapySDR.0.8.dylib", "libad9361.dylib",
                          "librtlsdrSupport.so", "libxml2.16.dylib"])
        ad9361_call = [call for call in calls
                       if call[4].endswith("libad9361.dylib")]
        self.assertEqual(ad9361_call[0][1:4],
                         ["-change", "@rpath/libxml2.16.dylib",
                          os.path.join(prefix, "lib", "libxml2.16.dylib")])

    def test_absolute_ref_is_untouched(self):
        from unittest import mock
        with tempfile.TemporaryDirectory() as prefix:
            moddir = os.path.join(prefix, "lib", "SoapySDR", "modules0.8")
            os.makedirs(moddir)
            with mock.patch.object(build.platform, "system",
                                   return_value="Darwin"):
                with mock.patch.object(build.subprocess, "run") as otool, \
                        mock.patch.object(build, "run") as run, \
                        mock.patch.object(build, "require"):
                    from types import SimpleNamespace
                    otool.return_value = SimpleNamespace(
                        stdout="\tmod:\n\t/usr/lib/libz.dylib\n",
                        returncode=0)
                    build.normalize_macos_load_paths(prefix)
            run.assert_not_called()

    def test_missing_target_is_an_error(self):
        with tempfile.TemporaryDirectory() as prefix:
            os.makedirs(os.path.join(prefix, "lib"))
            out = "\tmod:\n\t@rpath/libSoapySDR.0.8.dylib (compat)\n"
            with self.assertRaises(SystemExit):
                self._gate(prefix, out, lib_present=False)

    def test_noop_off_darwin(self):
        from unittest import mock
        with tempfile.TemporaryDirectory() as prefix:
            with mock.patch.object(build.platform, "system",
                                   return_value="Linux"):
                with mock.patch.object(build.subprocess, "run") as otool:
                    build.normalize_macos_load_paths(prefix)
            otool.assert_not_called()

    def test_dry_run_logs_only(self):
        with tempfile.TemporaryDirectory() as prefix:
            build.normalize_macos_load_paths(prefix, dry_run=True)


class GitCloneTests(unittest.TestCase):
    def _make_origin(self, tmp, tag=None):
        import subprocess
        origin = os.path.join(tmp, "origin")
        subprocess.run(["git", "init", "-q", origin], check=True)
        subprocess.run(["git", "-C", origin, "config", "user.email", "t@t"],
                       check=True)
        subprocess.run(["git", "-C", origin, "config", "user.name", "t"],
                       check=True)
        subprocess.run(["git", "-C", origin, "config", "commit.gpgsign",
                        "false"], check=True)
        with open(os.path.join(origin, "f.txt"), "w") as handle:
            handle.write("x")
        subprocess.run(["git", "-C", origin, "add", "."], check=True)
        subprocess.run(["git", "-C", origin, "commit", "-qm", "init"],
                       check=True)
        if tag:
            subprocess.run(["git", "-C", origin, "tag", tag], check=True)
        return origin

    def _git(self, *args):
        import subprocess
        return subprocess.run(["git"] + list(args), capture_output=True,
                              text=True, check=True)

    def test_reuses_checkout_at_pinned_tag(self):
        import shutil
        from unittest import mock
        if shutil.which("git") is None:
            self.skipTest("git not available")
        with tempfile.TemporaryDirectory() as tmp:
            origin = self._make_origin(tmp, tag="v0.3")
            dest = os.path.join(tmp, "libad9361")
            build.git_clone(origin, dest, branch="v0.3")
            with mock.patch.object(build, "run") as run:
                build.git_clone(origin, dest, branch="v0.3")
            run.assert_not_called()

    def test_reclones_checkout_predating_pin(self):
        import shutil
        if shutil.which("git") is None:
            self.skipTest("git not available")
        with tempfile.TemporaryDirectory() as tmp:
            origin = self._make_origin(tmp)
            dest = os.path.join(tmp, "libad9361")
            build.git_clone(origin, dest)
            marker = os.path.join(dest, "stale.txt")
            with open(marker, "w") as handle:
                handle.write("stale")
            self._git("-C", origin, "tag", "v0.3")
            build.git_clone(origin, dest, branch="v0.3")
            self.assertFalse(os.path.exists(marker))
            head = self._git("-C", dest, "rev-parse", "HEAD")
            want = self._git("-C", origin, "rev-parse", "v0.3^{commit}")
            self.assertEqual(head.stdout.strip(), want.stdout.strip())


if __name__ == "__main__":
    unittest.main()
