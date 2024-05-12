from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.system.package_manager import Apt
import os


class GzLbotBridgeConan(ConanFile):
    name = "gz-lbot-bridge"
    version = "v0.0.0"
    author = "Max Yvon Zimmermann (maxyvon@gmx.de)"
    description = "Bridge between gazebo sim and lbot"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = ("CMakeLists.txt", "src/*")

    def system_requirements(self):
        if self.settings.os != 'Linux':
            raise Exception("Package is only supported on Linux.")

        apt = Apt(self)
        apt.install(["libgz-cmake3-dev", "libgz-sim8-dev"], check = True)

    def build_requirements(self):
        self.tool_requires("cmake/3.29.3")

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
