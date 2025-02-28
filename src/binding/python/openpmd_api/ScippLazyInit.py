"""
This file is part of the openPMD-api.

Copyright 2025 openPMD contributors
Authors: Franz Poeschel
License: LGPLv3+
"""


def series_to_scipp(series):

    # lazy import
    try:
        import scipp  # noqa
        found_scipp = True
    except ImportError as original_error:
        found_scipp = False
        original_error_string = f"{original_error}"

    if not found_scipp:
        raise ImportError(
            f"Scipp NOT found. Install scipp for Scipp support. "
            f"Original error: {original_error_string}")

    from .scipp import DataLoader

    dl = DataLoader(series)
    return dl
