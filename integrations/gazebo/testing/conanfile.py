from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
import os
import subprocess
import regex


def get_lbot_version():
    output = subprocess.run(["conan", "inspect", "../../.."], stdout=subprocess.PIPE)
    print(output)
    pattern = regex.compile('version: (.*)\n')
    search = pattern.search(output.stdout.decode('utf-8'))
    return search.group(1)


class GzLbotBridgeTestConan(ConanFile):
    name = "gz-lbot-bridge-test"
    version = "v0.0.0"
    author = "Max Yvon Zimmermann (maxyvon@gmx.de)"
    description = "Bridge between gazebo sim and lbot"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = ("CMakeLists.txt", "src/*")

    def system_requirements(self):
        if self.settings.os != 'Linux':
            raise Exception("Package is only supported on Linux.")
        
    def requirements(self):
        self.requires("lbot/" + get_lbot_version())

    def build_requirements(self):
        self.tool_requires("cmake/3.28.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self)
        toolchain.variables["CMAKE_RUNTIME_OUTPUT_DIRECTORY"] = os.path.join(self.build_folder, "bin")
        toolchain.variables["CMAKE_LIBRARY_OUTPUT_DIRECTORY"] = os.path.join(self.build_folder, "lib")
        toolchain.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
