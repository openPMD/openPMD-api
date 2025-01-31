import openpmd_api as pmd


def main():
    series = pmd.Series("./out.bp5", pmd.Access.read_only)
    scipp_loader = series.to_scipp()

if __name__ == "__main__":
    main()
