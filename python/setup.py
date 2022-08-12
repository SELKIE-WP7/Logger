from setuptools import setup, find_packages
from os import path

import sys

sys.path.append(".")
import setup_common as sc


here = path.abspath(path.dirname(__file__))

setup(
    name=sc.project,
    version=sc.getVersionString(),
    description=sc.description,
    author=sc.author,
    author_email=sc.author_email,
    classifiers=[  # Optional
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Environment :: Console",
        "Environment :: Web Environment",
        "Operating System :: Linux" "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
    ],
    packages=find_packages(),  # Required
    python_requires=">=3.6, <4",
    install_requires=["msgpack", "pandas", "scipy"],
    extras_require={
        "Web": ["Flask>=1.1.0", "Flask-Bootstrap>=3.3.7.1"],
        "AutoHat": ["autohatctl"],
        "Serial": ["pyserial>=3.4"],
    },
    package_data={"SELKIELogger.WebInterface": ["templates/*.html"]},
    include_package_data=True,
    entry_points={
        "console_scripts": [
            "SLDump = SELKIELogger.scripts.SLDump:SLDump",
            "SLClassify = SELKIELogger.scripts.SLClassify:SLClassify",
            "SLConvert = SELKIELogger.scripts.SLConvert:SLConvert",
            "SLExtract = SELKIELogger.scripts.SLExtract:SLExtract",
            "SLGPSWatch = SELKIELogger.scripts.SLGPSWatch:SLGPSWatch",
            "SLIcinga = SELKIELogger.scripts.SLIcinga:SLIcinga",
            "SLSerialLog = SELKIELogger.scripts.SLSerialLog:SLSerialLog[Serial]",
            "SLWebDev = SELKIELogger.WebInterface:run_dev_server[Web]",
        ],
        "gui_scripts": [
            "SLView = SELKIELogger.scripts.SLView:SLView",
            "SLConvertGUI = SELKIELogger.scripts.GUI.apps:runSLConvertGUI",
            "SLExtractGUI = SELKIELogger.scripts.GUI.apps:runSLExtractGUI",
        ],
    },
)
