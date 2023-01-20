from conans import ConanFile, CMake, tools
from conan.tools.files import update_conandata
import regex
import os

class VersionInfo(dict):
    def __init__(self):
        git = tools.Git()

        super().__setitem__("commit", git.run("describe --tags"))
        super().__setitem__("tag", git.run("describe --tags --abbrev=0"))
        super().__setitem__("hash", git.run("rev-parse HEAD"))
        super().__setitem__("branch", git.run("rev-parse --abbrev-ref HEAD"))

        at_tag = (self["commit"] == self["tag"])
        super().__setitem__("hash_short", self["hash"][:7])

        pattern = regex.compile('^v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$')
        search = pattern.search(self["tag"])
        super().__setitem__("version_major", search.group(1))
        super().__setitem__("version_minor", search.group(2))
        super().__setitem__("version_patch", search.group(3))
        
        super().__setitem__("version", "%s.%s.%s" % (self["version_major"], self["version_minor"], self["version_patch"]))
        super().__setitem__("semver", "v%s+%s" % (self["version"], self["hash_short"]) if at_tag else self["version"])

class LabratRobotExamplesConan(ConanFile):
    name = "labrat-robot-examples"
    license = "GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)"
    author = "Max Yvon Zimmermann (maxyvon@gmx.de)"
    description = "Examples for labrat-robot"
    settings = "os", "compiler", "build_type", "arch"
    options = {"with_system_deps": [True, False]}
    default_options = {"with_system_deps": False}
    generators = "cmake"
    exports_sources = ("CMakeLists.txt", "src/*")

    def __init__(self, output, runner, display_name="", user=None, channel=None):
        try:
            self.version_data = VersionInfo()
        except:
            self.version_data = self.conan_data["version_data"]

        self.version = self.version_data["semver"]

        super().__init__(output, runner, display_name, user, channel)

    def system_requirements(self):
        if self.settings.os != 'Linux':
            raise Exception("Package is only supported on Linux.")

    def build_requirements(self):
        if self.options.with_system_deps:
            return

        self.build_requires('cmake/3.25.1')
        self.build_requires('labrat-robot/%s' % (self.version))
        self.build_requires('protobuf/3.21.9')

    def export(self):
        update_conandata(self, {"version_data": self.version_data})

    def _configure_cmake(self):
        cmake = CMake(self)

        self.run("cmake %s %s" % (self.source_folder, cmake.command_line))

        return cmake

    def _build_target(self, cmake, target = "all"):
        self.run("cmake --build . --target %s %s -j%d" % (target, cmake.build_config, tools.cpu_count()))

    def build(self):
        cmake = self._configure_cmake()

        self._build_target(cmake)

    def package(self):
        cmake = self._configure_cmake()

        self._build_target(cmake, "install")
