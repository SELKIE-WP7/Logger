#!/usr/bin/env python3

import re
import sys

from shutil import which
from subprocess import run, PIPE

project = "SELKIELogger"
description = "Parsing and processing scripts for SELKIE data logger"
author = "Thomas Lake"
author_email = "t.lake@swansea.ac.uk"


git = which("git")


def getGitProjectVersion():
    out = run([git, "describe", "--always", "--dirty=X", "--tags"], stdout=PIPE)
    if out.returncode != 0:
        return None
    versionString = out.stdout.decode("utf-8").strip()
    return versionString


def getGitProjectBranch():
    out = run([git, "symbolic-ref", "-q", "--short", "HEAD"], stdout=PIPE)
    if out.returncode != 0:
        return None
    branch = out.stdout.decode("utf-8").strip()
    return branch


def versionStringToTuple(v):
    m = re.match("^v([0-9]+)\.([0-9]+)-([0-9]+)-(.*)$", v)
    if m:
        return int(m[1]), int(m[2]), int(m[3])
    else:
        return None


def getVersionString():
    version = getGitProjectVersion()
    if version is None:
        raise ValueError("Unable to determine project version")

    vTup = versionStringToTuple(version)
    if vTup is None:
        return version

    versionString = ".".join(str(x) for x in vTup)
    return versionString


if __name__ == "__main__":
    version = getGitProjectVersion()
    branch = getGitProjectBranch()

    if version is None:
        print("Unable to determine project version")
        sys.exit(1)

    if branch is None:
        print("Unable to determine project branch")
        sys.exit(1)

    print(f"Project version: {version}")
    print(f"Current branch: {branch}")

    print(f"Decoded version: {versionStringToTuple(version)}")
