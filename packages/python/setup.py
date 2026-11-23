# setup.py exists only to make platform wheels: the package ships
# native binaries, so wheels are never pure. The platform tag comes
# from $TINYC_WHEEL_PLAT (set by build_wheel_matrix.sh); project
# metadata lives in pyproject.toml.
import os

from setuptools import setup

try:  # setuptools >= 70.1
    from setuptools.command.bdist_wheel import bdist_wheel
except ImportError:  # older setuptools: bdist_wheel lives in wheel
    from wheel.bdist_wheel import bdist_wheel


class TinycBdistWheel(bdist_wheel):
    def finalize_options(self):
        super().finalize_options()
        self.root_is_pure = False

    def get_tag(self):
        python, abi, plat = super().get_tag()
        return python, abi, os.environ.get("TINYC_WHEEL_PLAT", plat)


setup(cmdclass={"bdist_wheel": TinycBdistWheel})
