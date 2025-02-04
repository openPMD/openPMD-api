def series_to_scipp(series):

    import scipp

    from .scipp import DataLoader

    dl = DataLoader(series)
    return dl
