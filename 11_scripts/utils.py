#!/usr/bin/python
import sys
import stat
from pathlib import Path


def chmod_rx(p):
    if p is None:
        return

    s_mode = p.stat().st_mode
    if s_mode & stat.S_IRUSR:
        s_mode = s_mode | stat.S_IRGRP
        s_mode = s_mode | stat.S_IROTH

    if s_mode & stat.S_IXUSR:
        s_mode = s_mode | stat.S_IXGRP
        s_mode = s_mode | stat.S_IXOTH

    p.chmod(s_mode)

    if not p.is_dir():
        return

    for child in p.iterdir():
        chmod_rx(child)


def chmod_remove_x(p):
    if p is None:
        return

    if p.is_file():
        mode = p.stat().st_mode
        mode = mode & (~(stat.S_IXGRP | stat.S_IXOTH |
                         stat.S_IXUSR | stat.S_IWOTH | stat.S_IWGRP))
        p.chmod(mode)

    if p.is_dir():
        mode = p.stat().st_mode
        mode = mode & (~(stat.S_IXOTH | stat.S_IWOTH | stat.S_IWGRP))
        p.chmod(mode)

        for child in p.iterdir():
            chmod_remove_x(child)


if __name__ == "__main__":
    root_path = Path(sys.argv[1])
    chmod_remove_x(root_path)
