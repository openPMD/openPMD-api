{ version }:
final: prev:
let
  callPackage = final.callPackage;

  pybind11 = import ./pybind11 { fetchFromGitHub = final.fetchFromGitHub; };

  python3 = import ./python3 {
    inherit (prev) lib;
    python = prev.python3;
    packageOverlays = final.pythonPackagesOverlays;
  };

in
{
  adios2 = callPackage ./adios2 {
    python = final.python3;
  };
  openpmd_api = callPackage ./openpmd_api {
    inherit version;
    python = final.python3;
    hdf5 = final.hdf5-mpi;
    catch2 = final.catch2_3;
  };

  # python overlay as in
  # https://discourse.nixos.org/t/add-python-package-via-overlay/19783/3
  pythonPackagesOverlays = (prev.pythonPackagesOverlays or [ ]) ++ [
    pybind11
  ];

  inherit python3;
  python3Packages = final.python3.pkgs;
}
