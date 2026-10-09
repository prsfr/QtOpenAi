#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""aqtinstall, taught the per-compiler Windows repository layout of Qt 6.11+ (#33).

From 6.11 download.qt.io splits the Windows desktop repository by compiler:

    windows_x86/desktop/qt6_6111/qt6_6111_msvc2022_64/Updates.xml

aqtinstall 3.3.0 -- the latest release -- still asks for
qt6_6111/qt6_6111/Updates.xml, which is a 404, and reports it as "Failed to
locate XML data". The new location carries its Updates.xml.sha256, so nothing
about verification changes: this only points aqtinstall at the right
directory, and falls back to its own lookup when that directory is absent.

Delete this script, and go back to jurplel/install-qt-action on Windows, once
an aqtinstall release handles the layout itself.

Usage is exactly `python -m aqt ...`:

    python scripts/ci/install-qt.py install-qt windows desktop 6.11.1 \\
        win64_msvc2022_64 --outputdir Qt --modules qtwebsockets
"""

import sys

from aqt import archives
from aqt.exceptions import ArchiveDownloadError
from aqt.installer import Cli
from aqt.metadata import Version

_original_get_archives = archives.QtArchives._get_archives


def _get_archives(self):
    if self.os_name != "windows" or self.version < Version("6.11.0"):
        return _original_get_archives(self)
    series = f"qt{self.version.major}_{self._version_str()}"
    compiler = self.arch.removeprefix("win64_")
    try:
        self._get_archives_base(f"{series}/{series}_{compiler}", self._target_packages())
    except ArchiveDownloadError:
        _original_get_archives(self)


# The guard is not optional: aqtinstall extracts in worker processes, and on
# Windows those re-import this file as a module.
if __name__ == "__main__":
    archives.QtArchives._get_archives = _get_archives
    sys.exit(Cli().run(sys.argv[1:]))
