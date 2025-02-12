import openpmd_api as pmd


def main():
    series = pmd.Series("../samples/git-sample/data%T.h5", pmd.Access.read_only)

    try:
        scipp_loader = series.to_scipp()
        import plopp
        print("Plopp version:", plopp.__version__)
    except ImportError:
        print("Need to install scipp and plopp to run this example.")
        return
    import openpmd_api.scipp as pmdsc
    import scipp as sc
    time = 65 * sc.Unit("fs")

    print(scipp_loader.iterations)
    Ex = scipp_loader.get_field("E", "x", time=time)
    print(Ex)
    slicing_idx = pmdsc.closest(Ex, "x", 2 * sc.Unit("um"))
    Ex_slice = Ex["x", slicing_idx]
    print(Ex_slice)
    Ex_slice.plot().save("slice.png")
    Ex_line = Ex_slice["z", pmdsc.closest(Ex_slice, "z", 1.4e-5 * sc.Unit("m"))]
    print(Ex_line)
    Ex_line.plot().save("line.png")
    (Ex_line * Ex_line).plot().save("line_squared.png")

    # The full 3D array is not loaded into memory at this point.
    Ex = scipp_loader.get_field("E", "x", time=time, relay=True)
    # This time we will select a range rather than a slice.
    # For a range there is no need for an exact match.
    # But, we could also select a slice just like in the previous example.
    Ex = Ex["x", -2e-6 * sc.Unit("m") : 2e-6 * sc.Unit("m")]
    # Only now the smaller subset wil be loaded into memory
    Ex = Ex.load_data()
    print(Ex)

    Ex = sc.concat(
        [
            scipp_loader.get_field("E", "x", iteration=iteration.value, time_tolerance=None)
            for iteration in scipp_loader.iterations["iteration_id"]
        ],
        dim="t",
    )
    print(Ex)

    # Let us just slice at some points to get a 2D dataset
    Ex = Ex["x", 10]["y", 10]
    print(Ex)
    Ex.plot().save("moving_window.png")

if __name__ == "__main__":
    main()
