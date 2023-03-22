from conans import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain
from conan.tools.env import VirtualRunEnv
import os

class LabratRobotTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain", "VirtualRunEnv"

    def requirements(self):
        self.requires(self.tested_reference_str)
        self.requires("flatbuffers/22.12.06")

    def build_requirements(self):
        self.tool_requires("cmake/3.25.3")
        self.test_requires("gtest/1.13.0")

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
        cmake.test(env = "conanrun")
