from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain
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
        super().__setitem__("semver", "v%s" % (self["version"]) if at_tag else "v%s+%s" % (self["version"], self["hash_short"]))

class LabratRobotConan(ConanFile):
    name = "labrat-robot"
    license = "GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)"
    author = "Max Yvon Zimmermann (maxyvon@gmx.de)"
    url = "https://gitlab.com/labrat.eu/robot"
    description = "Minimal robot framework to provide an alternative to ROS."
    topics = "robotics", "messaging"
    settings = "os", "compiler", "build_type", "arch"
    options = {"with_system_deps": [True, False], "manual_build": [True, False]}
    default_options = {"with_system_deps": False, "manual_build": False}
    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = "CMakeLists.txt", "src/*", "cmake/*", "submodules/*"

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

    def requirements(self):
        if self.options.with_system_deps:
            return

        self.requires("flatbuffers/22.12.06")
        self.requires("mcap/0.5.0")
        self.requires("foxglove-websocket/0.0.1")
        self.requires("crc_cpp/1.2.0")

    def build_requirements(self):
        if self.options.with_system_deps:
            return

        self.tool_requires("cmake/3.25.1")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self)
        toolchain.variables["CMAKE_RUNTIME_OUTPUT_DIRECTORY"] = os.path.join(self.build_folder, "bin")
        toolchain.variables["CMAKE_LIBRARY_OUTPUT_DIRECTORY"] = os.path.join(self.build_folder, "lib")
        toolchain.variables["GIT_VERSION_MAJOR"] = self.version_data["version_major"]
        toolchain.variables["GIT_VERSION_MINOR"] = self.version_data["version_minor"]
        toolchain.variables["GIT_VERSION_PATCH"] = self.version_data["version_patch"]
        toolchain.variables["GIT_SEMVER"] = self.version_data["semver"]
        toolchain.variables["GIT_VERSION"] = self.version_data["version"]
        toolchain.variables["GIT_TAG"] = self.version_data["tag"]
        toolchain.variables["GIT_HASH"] = self.version_data["hash"]
        toolchain.variables["GIT_HASH_SHORT"] = self.version_data["hash_short"]
        toolchain.variables["GIT_BRANCH"] = self.version_data["branch"]
        toolchain.generate()

    def export(self):
        git = tools.Git()
        git.run("submodule update --init")

        update_conandata(self, {"version_data": self.version_data})

    def build(self):
        cmake = CMake(self)
        cmake.configure()

        if self.options.manual_build:
            return

        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "module")

        self.cpp_info.components["core"].set_property("cmake_target_name", f"{self.name}::core")
        self.cpp_info.components["core"].set_property("cmake_module_target_name", f"{self.name}::core")
        self.cpp_info.components["core"].names["cmake_find_package"] = self.name
        self.cpp_info.components["core"].names["cmake_find_package_multi"] = self.name
        self.cpp_info.components["core"].libs = ["robot"]
        self.cpp_info.components["core"].requires = ["flatbuffers::flatbuffers"]

        self.cpp_info.components["plugins"].set_property("cmake_target_name", f"{self.name}::plugins")
        self.cpp_info.components["plugins"].set_property("cmake_module_target_name", f"{self.name}::plugins")
        self.cpp_info.components["plugins"].names["cmake_find_package"] = self.name
        self.cpp_info.components["plugins"].names["cmake_find_package_multi"] = self.name
        self.cpp_info.components["plugins"].libs = ["plugins_mcap", "plugins_foxglove-ws", "plugins_mavlink", "plugins_udp-bridge", "plugins_serial-bridge"]
        self.cpp_info.components["plugins"].requires = ["core", "mcap::mcap", "foxglove-websocket::foxglove-websocket", "crc_cpp::crc_cpp"]

        self.runenv_info.append_path("LABRAT_ROBOT_REFLECTION_PATH", os.path.join(self.package_folder, "var", "run"))
