from setuptools import setup, find_packages
from os import path

here = path.abspath(path.dirname(__file__))

setup(
    name='SELKIELogger',
    version='1.2.0',
    description='Parsing and processing scripts for SELKIE data logger',
    author='Thomas Lake',
    author_email='t.lake@swansea.ac.uk',
    classifiers=[  # Optional
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
    ],

    packages=find_packages(),  # Required
    python_requires='>=3.6, <4',
    install_requires=['msgpack'],
    #package_data={
    #    "": ['*.html']
    #},
    include_package_data=True,
    entry_points = {
        "console_scripts": [
            "SLDump = SELKIELogger.scripts.SLDump:SLDump",
            "SLClassify = SELKIELogger.scripts.SLClassify:SLClassify",
            "SLView = SELKIELogger.scripts.SLView:SLView"
            ]
        }
)
