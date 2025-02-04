def series_to_scipp(series):
    import scipp

    from . import DataLoader

    dl = DataLoader(series)
    return dl
