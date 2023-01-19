from conans import ConanFile, CMake, tools

class LabratRobotTestConan(ConanFile):
    generators = "cmake"

    def build_requirements(self):
        self.build_requires('protobuf/3.21.9')
        self.build_requires('gtest/cci.20210126')

    def _configure_cmake(self):
        cmake = CMake(self)

        self.run("cmake %s %s" % (self.source_folder, cmake.command_line))

        return cmake

    def _build_target(self, cmake, target = "all"):
        self.run("cmake --build . --target %s %s -j%d" % (target, cmake.build_config, tools.cpu_count()))

    def build(self):
        cmake = self._configure_cmake()

        self._build_target(cmake)

    def test(self):
        self.run("ctest")
