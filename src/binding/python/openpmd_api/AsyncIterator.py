"""
This file is part of the openPMD-api.

Copyright 2026 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""

import asyncio

from . import openpmd_api_cxx as cxx


def stateful_iterator_aiter(self):
    """Return an asynchronous iterator over the Snapshots.

    Use together with Snapshots.begin_lazy(), so that the first IO step is
    opened by __anext__() and can run into a timeout.
    """
    return self


async def stateful_iterator_anext(self):
    """Await the next Iteration, or raise StopAsyncIteration at the end.

    This polls the non-blocking try_next(0) API and yields to the event loop
    between attempts. This is the minimal asyncio integration: it does not run
    a blocking wait in a worker thread.
    """
    while True:
        status = self.try_next(0.0)
        if status == cxx.AdvanceStatus.try_again:
            await asyncio.sleep(0.01)
            continue
        if status == cxx.AdvanceStatus.over:
            raise StopAsyncIteration
        if status == cxx.AdvanceStatus.random_access:
            raise RuntimeError(
                "async iteration requires a linear read workflow, "
                "use ordinary iteration instead"
            )
        return self.current()
