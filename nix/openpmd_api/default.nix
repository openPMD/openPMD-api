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
  samples ? null,
}:

let
  pybind11 = python.pkgs.pybind11 or null;

  # The caller's `doCheck` flag also names a derivation attribute below, so
  # capture its value here to avoid a self-referential `rec` binding.
  runChecks = doCheck;

  # for key `format`
  # pkgs/development/interpreters/python/mk-python-derivation.nix
  builder = if python != null then python.pkgs.buildPythonPackage else stdenv.mkDerivation;
  builder_params = if python != null then { format = "other"; } else { };
  maybe_path = dependency: lib.optionalString (dependency != null) "${dependency}:";

in
builder (
  builder_params
  // rec {
    inherit version;
    pname = "openpmd-api";

    src = ../..;

    preConfigure = ''
      # wont find these three otherwise
      export CMAKE_PREFIX_PATH="${maybe_path catch2}${maybe_path toml11}${maybe_path nlohmann_json}${maybe_path pybind11}$CMAKE_PREFIX_PATH"
      ${
        if runChecks then
          if samples != null then
            ''
              # make the example datasets available to the test suite
              # nixpkgs configures CMake out-of-source into cmakeBuildDir
              mkdir -p "''${cmakeBuildDir:-build}/samples"
              cp -R ${samples}/. "''${cmakeBuildDir:-build}/samples/"
            ''
          else
            ""
        else
          ""
      }
    '';

    # the test suite is driven by CTest; example data is expected to be
    # present when running the checks
    #
    # several tests launch MPI internally (via mpiexec or by importing the
    # MPI-enabled Python binding) and are order-, timing- and
    # resource-sensitive inside the restricted, single-node Nix sandbox:
    #   - the C++ parallel tests read back append-mode iterations whose
    #     on-disk order depends on the MPI write scheduling,
    #   - MPI/UCX/PmiX thread and process creation is limited in the sandbox
    #     (cgroup pids.max), so running many such tests at once is flaky.
    # They are skipped here and meant to be run in a proper MPI environment.
    # CTest is forced to run the remaining tests serially (-j 1) since
    # CMake 4 defaults to parallel execution, which exhausts the sandbox
    # process limit.
    # Testing is driven by CTest, but only from installCheckPhase (see below)
    # so it runs exactly once. buildPythonPackage has no checkPhase and
    # internally remaps doCheck → doInstallCheck (mk-python-derivation.nix);
    # for the plain stdenv path we must not run the generic checkPhase either,
    # otherwise ctest would run twice when doCheck = true.
    doCheck = if python != null then runChecks else false;
    doInstallCheck = runChecks;
    installCheckPhase = ''
      ctest --output-on-failure -j 1 -E 'MPI\.|CLI\.pipe\.py|Example\.py\..*_parallel'
    '';

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
      (lib.cmakeBool "openPMD_BUILD_TESTING" runChecks)
      (lib.cmakeBool "openPMD_BUILD_EXAMPLES" runChecks)
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
      if mpi != null && runChecks then
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
