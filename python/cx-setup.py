from cx_Freeze import setup, Executable
import setup_common as sc

# Dependencies are automatically detected, but it might need
# fine tuning.
build_options = {"packages": ["SELKIELogger"], "excludes": []}

import sys

gui = "Win32GUI" if sys.platform == "win32" else None
console = "Console"

executables = [
    Executable(
        "SELKIELogger/scripts/GUI/SLConvert.py", base=gui, target_name="SLConvertGUI"
    ),
    Executable(
        "SELKIELogger/scripts/GUI/SLExtract.py", base=gui, target_name="SLExtractGUI"
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
    options={"build_exe": build_options},
    executables=executables,
)
