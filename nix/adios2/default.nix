{ lib
, stdenv
, fetchFromGitHub
, cmake
, coreutils
, pkg-config
, perl
, hdf5 ? null
, mpi ? null
, ucx ? null
, blosc2 ? null
, bzip2 ? null
, libfabric ? null
, python ? null
, sqlite ? null
, zlib ? null
, dataspaces ? null
, cudaPackages ? null
, enableCuda ? false
}:

let
  python3Packages = if python == null then null else python.pkgs;
  cmake_on_off = dependency:
    let
      dependencies =
        if builtins.isList dependency
        then dependency else [ dependency ];
    in
    if builtins.all (d: d != null) dependencies then "ON" else "OFF";
in

stdenv.mkDerivation rec {
  version = "v2.12.1";
  pname = "adios2";

  src = fetchFromGitHub {
    owner = "ornladios";
    repo = "ADIOS2";
    rev = version;
    sha256 = "sha256-3jMvVYYO93/Pu7RW2x5mzTRMrZ3oC3IwGrUz2tSqJxQ=";
  };

  preConfigure = ''
    sed -i 's/if(NOT WIN32)/if(FALSE)/' cmake/install/post/CMakeLists.txt
  '';

  # Nix sets CMAKE_INSTALL_LIBDIR as an absolute path
  # meaning that path concatenation results in a wrong path here
  postConfigure = ''
    sed -Ei 's|(EVPATH_MODULE_INSTALL_DIR.*)/nix/.*/nix/|\1/nix/|' thirdparty/EVPath/EVPath/config.h
  '';

  propagatedBuildInputs =
    builtins.filter (el: el != null) [
      hdf5
      mpi
      ucx
      blosc2
      bzip2
      libfabric
      sqlite
      zlib
    ]
    ++
    (if mpi != null
    then builtins.filter (el: el != null) [ dataspaces ]
    else [ ])
    ++
    (if python != null
    then [
      python3Packages.python
      python3Packages.mpi4py
      python3Packages.numpy
    ]
    else [ ])
    ++
    (if enableCuda
    then [ cudaPackages.cudatoolkit ]
    else [ ]);
  nativeBuildInputs = [ cmake pkg-config perl ];
  buildInputs = [ coreutils ];
  cmakeFlags = [
    "-DADIOS2_BUILD_EXAMPLES=OFF"
    "-DBUILD_TESTING=OFF"
    "-DADIOS2_USE_MPI=${cmake_on_off mpi}"
    "-DADIOS2_USE_Python=${cmake_on_off python3Packages}"
    "-DADIOS2_USE_Campaign=${cmake_on_off [zlib sqlite mpi]}"
    "-DADIOS2_USE_DataSpaces=${cmake_on_off dataspaces}"
    "-DADIOS2_USE_CUDA=${if enableCuda then "ON" else "OFF"}"
    "-DADIOS2_USE_DataMan=ON"
    "-DADIOS2_USE_Fortran=OFF"
  ] ++ (if enableCuda then [
    "-DCMAKE_CUDA_HOST_COMPILER=${cudaPackages.cudatoolkit.cc}/bin/gcc"
    "-DCMAKE_CUDA_COMPILER=${cudaPackages.cudatoolkit}/bin/nvcc"
  ] else [ ]);

  meta = {
    description = "ADIOS2";
    longDescription = ''
      ADIOS2
    '';
    platforms = lib.platforms.x86;
  };
}
