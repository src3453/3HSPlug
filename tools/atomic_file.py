"""Publish complete files, allowing brief Windows reader/scanner locks."""
import os
from pathlib import Path
import tempfile
import time


def atomic_write(path, write, *, retry_seconds=3.0):
    """Call write(binary_stream), then replace path; keep pending data on failure.

    Only Windows access/share/lock violations during replacement are retried.
    Each caller owns a unique staging file in the destination directory.
    """
    path = Path(path).absolute()
    fd, name = tempfile.mkstemp(prefix=f".{path.name}.", suffix=".tmp", dir=path.parent)
    pending = Path(name)
    try:
        with os.fdopen(fd, "wb") as stream:
            write(stream)
    except BaseException:
        # A partial file is not a recoverable checkpoint; preserve the write error.
        try:
            pending.unlink()
        except OSError:
            pass
        raise

    deadline = time.monotonic() + retry_seconds
    delay = 0.025
    while True:
        try:
            os.replace(pending, path)
            return
        except OSError as exc:
            remaining = deadline - time.monotonic()
            if (os.name == "nt" and getattr(exc, "winerror", None) in (5, 32, 33)
                    and remaining > 0):
                time.sleep(min(delay, remaining))
                delay = min(delay * 2, 0.2)
                continue
            # Never remove the old checkpoint or the newly completed data.
            raise OSError(exc.errno,
                          f"Could not replace {path}: {exc}. "
                          f"Completed data retained at {pending}; close applications "
                          "holding the destination and check write permissions.", str(path)) from exc


def atomic_write_bytes(path, data, **kwargs):
    atomic_write(path, lambda stream: stream.write(data), **kwargs)
