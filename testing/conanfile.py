from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualRunEnv
import os

class LabratRobotTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires(self.tested_reference_str)
        self.requires("flatbuffers/23.5.26")

    def build_requirements(self):
        self.tool_requires("cmake/3.28.1")
        self.test_requires("gtest/1.14.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.build_context_activated = ["gtest"]
        deps.build_context_build_modules = ["gtest"]
        deps.generate()

        toolchain = CMakeToolchain(self)
        toolchain.variables["CMAKE_RUNTIME_OUTPUT_DIRECTORY"] = os.path.join(self.build_folder, "bin")
        toolchain.variables["CMAKE_LIBRARY_OUTPUT_DIRECTORY"] = os.path.join(self.build_folder, "lib")
        toolchain.generate()

        environment = VirtualRunEnv(self)
        environment.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        cmake = CMake(self)
        cmake.test(cli_args=['ARGS=--output-junit report.xml'], env = "conanrun")
