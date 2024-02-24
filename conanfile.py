from conan import ConanFile, errors
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, update_conandata
from conan.tools.scm import Git
import regex
import os


class VersionInfo(dict):
    def __init__(self, conanfile):
        git = Git(conanfile)

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
    options = {"with_system_deps": [True, False]}
    default_options = {"with_system_deps": False}

    @property
    def _module_path(self):
        return os.path.join("lib", "cmake")
    
    def get_version_data(self):
        try:
            return VersionInfo(self)
        except:
            try:
                return self.conan_data["version_data"]
            except TypeError:
                pass
            raise
    
    def set_version(self):
        self.version = self.get_version_data()["semver"]

    def system_requirements(self):
        if self.settings.os != 'Linux':
            raise Exception("Package is only supported on Linux.")

    def requirements(self):
        if self.options.with_system_deps:
            return

        self.requires("flatbuffers/23.5.26")

        self.requires("mcap/1.3.0")
        self.requires("crc_cpp/1.2.0")

        #self.requires("foxglove-websocket/0.0.1")
        self.requires("nlohmann_json/3.11.3")
        self.requires("websocketpp/0.8.2")

    def build_requirements(self):
        if self.options.with_system_deps:
            return

        self.tool_requires("cmake/3.28.1")

    def configure(self):
        self.options["websocketpp"].asio = "standalone"

    def layout(self):
        cmake_layout(self)

    def export_sources(self):
        git = Git(self)
        git.run("submodule update --init")

        copy(self, "CMakeLists.txt", self.recipe_folder, self.export_sources_folder)
        copy(self, "src/*", self.recipe_folder, self.export_sources_folder)
        copy(self, "cmake/*", self.recipe_folder, self.export_sources_folder)
        copy(self, "install/*", self.recipe_folder, self.export_sources_folder)
        copy(self, "submodules/*", self.recipe_folder, self.export_sources_folder)

    def source(self):
        git = Git(self)

        try:
            git.run("status")
        except errors.ConanException:
            return

        git.run("submodule update --init")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        version_data = self.get_version_data()

        toolchain = CMakeToolchain(self)
        toolchain.variables["CMAKE_RUNTIME_OUTPUT_DIRECTORY"] = os.path.join(self.build_folder, "bin")
        toolchain.variables["CMAKE_LIBRARY_OUTPUT_DIRECTORY"] = os.path.join(self.build_folder, "lib")
        toolchain.variables["GIT_VERSION_MAJOR"] = version_data["version_major"]
        toolchain.variables["GIT_VERSION_MINOR"] = version_data["version_minor"]
        toolchain.variables["GIT_VERSION_PATCH"] = version_data["version_patch"]
        toolchain.variables["GIT_SEMVER"] = version_data["semver"]
        toolchain.variables["GIT_VERSION"] = version_data["version"]
        toolchain.variables["GIT_TAG"] = version_data["tag"]
        toolchain.variables["GIT_HASH"] = version_data["hash"]
        toolchain.variables["GIT_HASH_SHORT"] = version_data["hash_short"]
        toolchain.variables["GIT_BRANCH"] = version_data["branch"]
        toolchain.generate()

    def export(self):
        update_conandata(self, {"version_data": self.get_version_data()})

    def build(self):
        cmake = CMake(self)
    
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        module_paths = [
            os.path.join(self._module_path, "LabratRobotLauncher.cmake"),
            os.path.join(self._module_path, "LabratRobotGenerateFlatbuffer.cmake")
        ]

        self.cpp_info.set_property("cmake_find_mode", "module")
        self.cpp_info.set_property("cmake_build_modules", module_paths)

        self.cpp_info.components["core"].set_property("cmake_target_name", f"{self.name}::core")
        self.cpp_info.components["core"].set_property("cmake_module_target_name", f"{self.name}::core")
        self.cpp_info.components["core"].names["cmake_find_package"] = self.name
        self.cpp_info.components["core"].names["cmake_find_package_multi"] = self.name
        self.cpp_info.components["core"].build_modules["cmake_find_package"] = module_paths
        self.cpp_info.components["core"].build_modules["cmake_find_package_multi"] = module_paths
        self.cpp_info.components["core"].libs = ["robot"]
        self.cpp_info.components["core"].requires = ["flatbuffers::flatbuffers"]

        self.cpp_info.components["plugins"].set_property("cmake_target_name", f"{self.name}::plugins")
        self.cpp_info.components["plugins"].set_property("cmake_module_target_name", f"{self.name}::plugins")
        self.cpp_info.components["plugins"].names["cmake_find_package"] = self.name
        self.cpp_info.components["plugins"].names["cmake_find_package_multi"] = self.name
        self.cpp_info.components["plugins"].libs = ["plugins_mcap", "plugins_foxglove-ws", "plugins_mavlink", "plugins_udp-bridge", "plugins_serial-bridge"]
        #self.cpp_info.components["plugins"].requires = ["core", "mcap::mcap", "foxglove-websocket::foxglove-websocket", "crc_cpp::crc_cpp"]
        self.cpp_info.components["plugins"].requires = ["core", "mcap::mcap", "nlohmann_json::nlohmann_json", "websocketpp::websocketpp", "crc_cpp::crc_cpp"]

        self.runenv_info.append_path("LABRAT_ROBOT_REFLECTION_PATH", os.path.join(self.package_folder, "var", "run"))
