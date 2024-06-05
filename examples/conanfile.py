from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
import subprocess
import regex


def get_lbot_version():
    output = subprocess.run(["conan", "inspect", ".."], stdout=subprocess.PIPE)
    print(output)
    pattern = regex.compile('version: (.*)\n')
    search = pattern.search(output.stdout.decode('utf-8'))
    return search.group(1)


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
        self.requires("lbot/" + get_lbot_version())

    def build_requirements(self):
        self.tool_requires("cmake/3.29.3")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self)
        toolchain.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
