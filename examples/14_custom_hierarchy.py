import numpy as np
import openpmd_api as io


def main():
    if "bp" in io.file_extensions:
        filename = "../samples/custom_hierarchy.bp"
    else:
        filename = "../samples/custom_hierarchy.json"
    s = io.Series(filename, io.Access.create)
    it = s.write_iterations()[100]

    # write openPMD part
    temp = it.meshes["temperature"]
    temp.axis_labels = ["x", "y"]
    temp.unit_dimension = {io.Unit_Dimension.T: 1}
    temp.position = [0.5, 0.5]
    temp.grid_spacing = [1, 1]
    temp.grid_global_offset = [0, 0]
    temp.reset_dataset(io.Dataset(np.dtype("double"), [5, 5]))
    temp[()] = np.zeros((5, 5))

    # write NeXus part
    nxentry = it["Scan"]
    nxentry.set_attribute("NX_class", "NXentry")
    nxentry.set_attribute("default", "data")

    data = nxentry["data"]
    data.set_attribute("NX_class", "NXdata")
    data.set_attribute("signal", "counts")
    data.set_attribute("axes", ["two_theta"])
    data.set_attribute("two_theta_indices", [0])

    counts = data.as_container_of_datasets()["counts"]
    counts.set_attribute("units", "counts")
    counts.set_attribute("long_name", "photodiode counts")
    counts.reset_dataset(io.Dataset(np.dtype("int"), [15]))
    counts[()] = np.zeros(15, dtype=np.dtype("int"))

    two_theta = data.as_container_of_datasets()["two_theta"]
    two_theta.set_attribute("units", "degrees")
    two_theta.set_attribute("long_name", "two_theta (degrees)")
    two_theta.reset_dataset(io.Dataset(np.dtype("double"), [15]))
    two_theta[()] = np.zeros(15)


if __name__ == "__main__":
    main()
