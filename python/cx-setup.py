from cx_Freeze import setup, Executable

import os
import sys

sys.path.append(".")
import setup_common as sc


PYTHON_INSTALL_DIR = os.path.dirname(os.__file__)
include_files = [
    (
        os.path.join(PYTHON_INSTALL_DIR, "..", "DLLs", "tk86t.dll"),
        os.path.join("lib", "tk86t.dll"),
    ),
    (
        os.path.join(PYTHON_INSTALL_DIR, "..", "DLLs", "tcl86t.dll"),
        os.path.join("lib", "tcl86t.dll"),
    ),
]

build_options = {
    "packages": ["SELKIELogger", "openpyxl"],
    "excludes": [],
    "include_files": include_files,
}

gui = "Win32GUI" if sys.platform == "win32" else None
console = "Console"

executables = [
    Executable(
        "SELKIELogger/scripts/GUI/SLConvert.py",
        base=gui,
        target_name="SLConvertGUI",
        shortcut_name="SLConvert",
        shortcut_dir="ProgramMenuFolder",
    ),
    Executable(
        "SELKIELogger/scripts/GUI/SLExtract.py",
        base=gui,
        target_name="SLExtractGUI",
        shortcut_name="SLExtract",
        shortcut_dir="ProgramMenuFolder",
    ),
    Executable(
        "SELKIELogger/scripts/SLConvert.py", base=console, target_name="SLConvert"
    ),
    Executable(
        "SELKIELogger/scripts/SLExtract.py", base=console, target_name="SLExtract"
    ),
    Executable(
        "SELKIELogger/scripts/SLClassify.py", base=console, target_name="SLClassify"
    ),
    Executable("SELKIELogger/scripts/SLDump.py", base=console, target_name="SLDump"),
    Executable(
        "SELKIELogger/scripts/SLGPSWatch.py", base=console, target_name="SLGPSWatch"
    ),
]

setup(
    name=sc.project,
    version=sc.getVersionString(),
    description=sc.description,
    author=sc.author,
    author_email=sc.author_email,
    options={
        "build_exe": build_options,
        "bdist_msi": {"upgrade_code": "{F0311C90-5B59-4B08-AC4D-9036BC3FD23F}"},
    },
    executables=executables,
)
