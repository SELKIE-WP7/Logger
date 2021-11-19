from setuptools import setup, find_packages
from os import path

here = path.abspath(path.dirname(__file__))

setup(
    name='SELKIELogger',
    version='1.3.1',
    description='Parsing and processing scripts for SELKIE data logger',
    author='Thomas Lake',
    author_email='t.lake@swansea.ac.uk',
    classifiers=[  # Optional
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Environment :: Console',
        'Environment :: Web Environment',
        'Operating System :: Linux'
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
    ],

    packages=find_packages(),  # Required
    python_requires='>=3.6, <4',
    install_requires=['msgpack'],
    extras_require={
        "Web": ['Flask>=1.1.0', 'Flask-Bootstrap>=3.3.7.1'],
        "AutoHat": ['autohatctl']
    },
    package_data={
        "SELKIELogger.WebInterface": ['templates/*.html']
    },
    include_package_data=True,
    entry_points = {
        "console_scripts": [
            "SLDump = SELKIELogger.scripts.SLDump:SLDump",
            "SLClassify = SELKIELogger.scripts.SLClassify:SLClassify",
            "SLWebDev = SELKIELogger.WebInterface:run_dev_server[Web]",
        ],
        "gui_scripts": [
            "SLView = SELKIELogger.scripts.SLView:SLView"
        ]
    }
)
