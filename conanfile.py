from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy, load
import os
import re


def get_version_from_header():
    """Extract version from version.hpp"""
    try:
        version_file = os.path.join(os.path.dirname(__file__), "include", "JsonFusion", "version.hpp")
        content = load(None, version_file)
        match = re.search(r'#define\s+JSONFUSION_VERSION\s+"([^"]+)"', content)
        if match:
            return match.group(1)
    except:
        pass
    return "0.710.0"  # Fallback version


class JsonFusionConan(ConanFile):
    name = "jsonfusion"
    
    # Automatically extract version from version.hpp
    def set_version(self):
        self.version = get_version_from_header()
    
    # Metadata
    license = "MIT"
    author = "Aleksandr Tuchkov"
    url = "https://github.com/tucher/JsonFusion"
    homepage = "https://github.com/tucher/JsonFusion"
    description = ("Type-driven JSON/CBOR parser and serializer with declarative validation. "
                   "Parse JSON directly into your C++ structs with zero boilerplate. "
                   "Header-only, compile-time reflection-driven, embedded-friendly.")
    topics = ("json", "parser", "serializer", "cbor", "validation", "reflection", 
              "header-only", "embedded", "type-safe", "constexpr", "c++23", "schema")
    
    # Package configuration
    package_type = "header-library"
    settings = "os", "compiler", "build_type", "arch"
    no_copy_source = True
    
    def layout(self):
        cmake_layout(self, src_folder=".")
    
    def package_id(self):
        # Header-only library, so package is the same for all configurations
        self.info.clear()
    
    def validate(self):
        # JsonFusion requires C++23
        if self.settings.compiler.cppstd:
            from conan.tools.build import check_min_cppstd
            check_min_cppstd(self, "23")
    
    def source(self):
        # When creating locally with 'conan create', this is not needed
        # When publishing to Conan Center, use get() to fetch from GitHub
        pass
    
    def export_sources(self):
        # Export the include directory with the recipe
        copy(self, "*.hpp", src=os.path.join(self.recipe_folder, "include"),
             dst=os.path.join(self.export_sources_folder, "include"), keep_path=True)
        copy(self, "*.h", src=os.path.join(self.recipe_folder, "include"),
             dst=os.path.join(self.export_sources_folder, "include"), keep_path=True)
        copy(self, "LICENSE.md", src=self.recipe_folder,
             dst=self.export_sources_folder)
    
    def package(self):
        # Copy the license file
        copy(self, "LICENSE.md", 
             src=self.source_folder, 
             dst=os.path.join(self.package_folder, "licenses"))
        
        # Copy all headers from include directory (recursively, including subdirectories)
        copy(self, "*.hpp", 
             src=os.path.join(self.source_folder, "include"), 
             dst=os.path.join(self.package_folder, "include"),
             keep_path=True)
        
        copy(self, "*.h", 
             src=os.path.join(self.source_folder, "include"), 
             dst=os.path.join(self.package_folder, "include"),
             keep_path=True)
    
    def package_info(self):
        # Since it's header-only, no libs to link
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        
        # Set the include directory
        self.cpp_info.includedirs = ["include"]
        
        # Set CMake target name (Conan 2.x style)
        self.cpp_info.set_property("cmake_target_name", "JsonFusion::JsonFusion")
        self.cpp_info.set_property("cmake_file_name", "JsonFusion")
