{ lib
, stdenv
, version
, cmake
, nlohmann_json
, toml11
, catch2
, hdf5 ? null
, adios2 ? null
, mpi ? null
, python ? null
, doCheck ? false
}:

let
  cmake_on_off = dependency: if dependency == null || dependency == false then "OFF" else "ON";
  pybind11 = python.pkgs.pybind11 or null;

  # for key `format`
  # pkgs/development/interpreters/python/mk-python-derivation.nix
  builder =
    if python != null
    then python.pkgs.buildPythonPackage
    else stdenv.mkDerivation;
  builder_params =
    if python != null
    then { format = "other"; }
    else { };
  maybe_path = dependency: lib.optionalString (dependency != null) "${dependency}:";

in
builder (builder_params // rec {
  inherit version doCheck;
  pname = "openPMD-api";

  src = ../..;

  preConfigure = ''
    # wont find these three otherwise
    export CMAKE_PREFIX_PATH="${maybe_path catch2}${maybe_path toml11}${maybe_path nlohmann_json}${maybe_path pybind11}$CMAKE_PREFIX_PATH"
    ${if doCheck then ''
      # TODO: download samples
    '' else ""}
  '';

  postBuild =
    if doCheck then ''
      ctest --output-on-failure
      false
    '' else "";

  postInstall = ''
    sed -Ei 's|=.*}/'"$out"'|='"$out"'|' $out/lib/pkgconfig/openPMD.pc
  '';

  propagatedBuildInputs =
    builtins.filter
      (el: el != null)
      [
        hdf5
        adios2
        mpi
        mpi.dev
      ]
    ++
    (if python != null
    then [
      python
      python.pkgs.mpi4py
      python.pkgs.numpy
    ]
    else [ ]);
  nativeBuildInputs =
    (builtins.filter (el: el != null) [
      toml11
      nlohmann_json
      cmake
      catch2
      pybind11
    ]);

  cmakeFlags = [
    "-DopenPMD_BUILD_TESTING=${cmake_on_off doCheck}"
    "-DopenPMD_BUILD_EXAMPLES=${cmake_on_off doCheck}"
    "-DopenPMD_USE_ADIOS2=${cmake_on_off adios2}"
    "-DopenPMD_USE_HDF5=${cmake_on_off hdf5}"
    "-DopenPMD_USE_MPI=${cmake_on_off mpi}"
    "-DopenPMD_USE_PYTHON=${cmake_on_off python}"
    "-DopenPMD_USE_INTERNAL_CATCH=OFF"
    "-DopenPMD_USE_INTERNAL_JSON=OFF"
    "-DopenPMD_USE_INTERNAL_TOML11=OFF"
    "-DopenPMD_USE_INTERNAL_PYBIND11=OFF"
  ]
  # buildPythonPackage might have issues here
  ++ (if mpi != null && python != null then [
    "-DCMAKE_C_COMPILER=${mpi.dev}/bin/mpicc"
    "-DCMAKE_CXX_COMPILER=${mpi.dev}/bin/mpicxx"
  ] else [ ]) ++ (if mpi != null && doCheck then [
    "-DMPIEXEC_EXECUTABLE=${mpi}/bin/mpiexec"
  ] else [ ]);

  meta = {
    # homepage = "https://rr-project.org/";
    description = "openPMD-api";
    longDescription = ''
      openPMD-api
    '';

    license = with lib.licenses; [ lgpl3 ];
    platforms = lib.platforms.x86;
  };
}
)
