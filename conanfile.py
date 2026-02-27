from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain
from pathlib import Path


class Carimbo(ConanFile):
    settings = "os", "arch", "compiler", "build_type"

    @property
    def _is_webassembly(self):
        return str(self.settings.os).lower() == "emscripten"

    def requirements(self):
        for package in [
            "box2d/3.1.1",
            "entt/3.16.0",
            "libspng/0.7.4",
            "miniaudio/0.11.22",
            "opusfile/0.12",
            "physfs/3.2.0",
            "sdl/3.4.0",
        ]:
            self.requires(package)

        if self._is_webassembly:
            self.requires("lua/5.4.8")
        else:
            self.requires("luajit/2.1.0-beta3")

    def configure(self):
        self.options["miniaudio"].header_only = True

    def generate(self):
        license_output = Path(self.build_folder) / "LICENSES"
        with license_output.open("w", encoding="utf-8") as out:
            for dep in self.dependencies.values():
                if dep.is_build_context or not dep.package_folder:
                    continue

                pid = f"{dep.ref.name}/{dep.ref.version}"
                for file in Path(dep.package_folder).rglob("*"):
                    if not file.is_file():
                        continue

                    name = file.name.lower()
                    if name.startswith(("license", "copying", "copyright")):
                        text = file.read_text(encoding="utf-8", errors="ignore").strip()
                        out.write(f"{pid}\n{text}\n\n")

        toolchain = CMakeToolchain(self)

        toolchain.generate()
        CMakeDeps(self).generate()
