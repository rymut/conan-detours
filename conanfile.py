from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.files import load, copy
from conan.tools.scm import Git
import os


class Detours(ConanFile):
    name = "detours"
    version = "4.0.1"
    description = "Conan recipe for Detours by Microsoft Research"
    url = "https://github.com/microsoft/Detours.git"
    homepage = "https://github.com/rymut/conan-detours"
    author = "Boguslaw Rymut (boguslaw@rymut.org)"
    topics = ("detours", "win32", "native")
    license = "MIT"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [False],
    }
    default_options = {
        "shared": False,
    }
    exports_sources = "CMakeLists.txt"
    def requirements(self):
        if self.settings.os != "Windows":
            raise RuntimeError("Only Windows is supported!")
        if self.settings.arch not in ('x86', 'x86_64', 'armv8'):
            raise RuntimeError("Architecture %s is not supported")
    
            
    def layout(self):
        cmake_layout(self)

    def source(self):
        # Clone the Detours repo and checkout our target commit
        git = Git(self)
        git.clone(url=self.url, target="detours")
        git.folder="detours"
        ver = self.version
        if self.version.startswith("rev_"):
            ver = self.version[4:]
        else:
            ver = "v%s" % self.version
        git.checkout(commit=ver)
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["DETOURS_SRC_ROOT"] = os.path.join(self.source_folder, "detours").replace('\\', '/');
        tc.generate()

    def package(self):
        cmake = CMake(self)
        cmake.install()
    
    def package_info(self):
        self.cpp_info.includedirs = ["include/detours"]
        self.cpp_info.libs = ["detours"]
