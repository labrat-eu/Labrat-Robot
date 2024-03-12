from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualRunEnv
import os


class LbotExamplesConan(ConanFile):
    name = "lbot-examples"
    version = "<your version>"
    author = "<your name and e-mail>"
    description = "Examples for lbot"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = ("CMakeLists.txt", "src/*")

    def system_requirements(self):
        if self.settings.os != 'Linux':
            raise Exception("Package is only supported on Linux.")

    def requirements(self):
        # You may add more dependencies here.
        self.requires("lbot/v0.0.11+f61d7ee")

    def build_requirements(self):
        self.tool_requires("cmake/3.28.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self)
        toolchain.generate()

        environment = VirtualRunEnv(self)
        environment.environment().append_path("LBOT_REFLECTION_PATH", os.path.join(self.folders.build, "flatbuffer", "schema"))
        environment.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
