{
  lib,
  stdenv,
  version,
  cmake,
  nlohmann_json,
  toml11,
  catch2,
  hdf5 ? null,
  adios2 ? null,
  mpi ? null,
  python ? null,
  doCheck ? false,
}:

let
  pybind11 = python.pkgs.pybind11 or null;

  # for key `format`
  # pkgs/development/interpreters/python/mk-python-derivation.nix
  builder = if python != null then python.pkgs.buildPythonPackage else stdenv.mkDerivation;
  builder_params = if python != null then { format = "other"; } else { };
  maybe_path = dependency: lib.optionalString (dependency != null) "${dependency}:";

in
builder (
  builder_params
  // rec {
    inherit version doCheck;
    pname = "openpmd-api";

    src = ../..;

    preConfigure = ''
      # wont find these three otherwise
      export CMAKE_PREFIX_PATH="${maybe_path catch2}${maybe_path toml11}${maybe_path nlohmann_json}${maybe_path pybind11}$CMAKE_PREFIX_PATH"
      ${
        if doCheck then
          ''
            # TODO: download samples
          ''
        else
          ""
      }
    '';

    postBuild =
      if doCheck then
        ''
          ctest --output-on-failure
          false
        ''
      else
        "";

    postInstall = ''
      sed -Ei 's|=.*}/'"$out"'|='"$out"'|' $out/lib/pkgconfig/openPMD.pc
    '';

    propagatedBuildInputs =
      builtins.filter (el: el != null) [
        hdf5
        adios2
        mpi
      ]
      ++ (
        if python != null then
          [
            python
            python.pkgs.mpi4py
            python.pkgs.numpy
          ]
        else
          [ ]
      );
    nativeBuildInputs = (
      builtins.filter (el: el != null) [
        toml11
        nlohmann_json
        cmake
        catch2
        pybind11
      ]
    );

    cmakeFlags = [
      (lib.cmakeBool "openPMD_BUILD_TESTING" doCheck)
      (lib.cmakeBool "openPMD_BUILD_EXAMPLES" doCheck)
      (lib.cmakeBool "openPMD_USE_ADIOS2" (adios2 != null))
      (lib.cmakeBool "openPMD_USE_HDF5" (hdf5 != null))
      (lib.cmakeBool "openPMD_USE_MPI" (mpi != null))
      (lib.cmakeBool "openPMD_USE_PYTHON" (python != null))
      (lib.cmakeBool "openPMD_USE_INTERNAL_CATCH" false)
      (lib.cmakeBool "openPMD_USE_INTERNAL_JSON" false)
      (lib.cmakeBool "openPMD_USE_INTERNAL_TOML11" false)
      (lib.cmakeBool "openPMD_USE_INTERNAL_PYBIND11" false)
    ]
    # buildPythonPackage might have issues here
    ++ (
      if mpi != null && python != null then
        [
          "-DCMAKE_C_COMPILER=${mpi.dev}/bin/mpicc"
          "-DCMAKE_CXX_COMPILER=${mpi.dev}/bin/mpicxx"
        ]
      else
        [ ]
    )
    ++ (
      if mpi != null && doCheck then
        [
          "-DMPIEXEC_EXECUTABLE=${mpi}/bin/mpiexec"
        ]
      else
        [ ]
    );

    meta = with lib; {
      homepage = "https://github.com/openPMD/openPMD-api";
      description = "C++ & Python API for scientific I/O with openPMD";
      longDescription = ''
        The openPMD-api provides a high-level C++ and Python interface for
        reading and writing data following the openPMD standard, supporting
        the HDF5 and ADIOS2 file formats.
      '';
      changelog = "https://github.com/openPMD/openPMD-api/blob/dev/CHANGELOG.rst";
      license = licenses.lgpl3Plus;
      platforms = platforms.unix;
      maintainers = [
        {
          name = "Axel Huebl";
          github = "ax3l";
        }
        {
          name = "Franz Pöschel";
          github = "franzpoeschel";
        }
      ];
    };
  }
)
