{
  lib,
  runCommand,
  fetchurl,
  gnutar,
  gzip,
  unzip,
}:
let
  # Nix-friendly equivalent of running share/openPMD/download_samples.sh in
  # the CMake build directory: the example datasets are fetched as fixed
  # output derivations and assembled into the `samples/` layout the test
  # suite expects, instead of being downloaded at test time.
  #
  # example datasets: https://github.com/openPMD/openPMD-example-datasets
  example3d = fetchurl {
    name = "example-3d.tar.gz";
    url = "https://github.com/openPMD/openPMD-example-datasets/raw/f3b73e43511db96217a153dc3ab3cb2e8f81f7db/example-3d.tar.gz";
    sha256 = "sha256-jidfkKETe2iZbnpIrfKUBH/HUrHba6lNZdAoYaJB5qM=";
  };
  thetaMode = fetchurl {
    name = "example-thetaMode.tar.gz";
    url = "https://github.com/openPMD/openPMD-example-datasets/raw/f3b73e43511db96217a153dc3ab3cb2e8f81f7db/example-thetaMode.tar.gz";
    sha256 = "sha256-i1C7pa+ZN3kcsO4gs6wp5dfG5lc9QLq19rwJl56MZ2g=";
  };
  bp4 = fetchurl {
    name = "example-3d-bp4.tar.gz";
    url = "https://github.com/openPMD/openPMD-example-datasets/raw/f3b73e43511db96217a153dc3ab3cb2e8f81f7db/example-3d-bp4.tar.gz";
    sha256 = "sha256-cjEY7sTEGUB4L2ISphInIgEBjgtJwZaUDHl7D8AcKdU=";
  };
  legacy = fetchurl {
    name = "legacy_datasets.tar.gz";
    url = "https://github.com/openPMD/openPMD-example-datasets/raw/566b356030df38f56049484941baacafef331163/legacy_datasets.tar.gz";
    sha256 = "sha256-V4U0S68GS9lvV2MmuCApgEGR34EAgXvSlTw8grjGeic=";
  };
  # issue samples: https://github.com/yt-project/yt/pull/1645
  noFields = fetchurl {
    name = "no_fields.zip";
    url = "https://github.com/yt-project/yt/files/1542668/no_fields.zip";
    sha256 = "sha256-tcdWcmLwUSE+sz68TsNeXjY7Bp2X1SF55ed6DtmoYvA=";
  };
  noParticles = fetchurl {
    name = "no_particles.zip";
    url = "https://github.com/yt-project/yt/files/1542670/no_particles.zip";
    sha256 = "sha256-lM3RN2pjBxCtloVD5+AgzEh9bSlVAFDMCFFoz31/m7Y=";
  };
  # alternate FBPIC sample: https://github.com/openPMD/openPMD-viewer/issues/296
  diags = fetchurl {
    name = "diags.zip";
    url = "https://github.com/openPMD/openPMD-viewer/files/5655027/diags.zip";
    sha256 = "sha256-FRsifeBE2oiB1t8whL2oEnoAfWiikMWqijDEHUnQItk=";
  };
in
runCommand "openpmd-example-datasets"
  {
    nativeBuildInputs = [
      gnutar
      gzip
      unzip
    ];
    meta = {
      description = "Example datasets for the openPMD-api test suite";
      homepage = "https://github.com/openPMD/openPMD-example-datasets";
    };
  }
  ''
    mkdir -p $out/git-sample/thetaMode $out/git-sample/3d-bp4 $out/git-sample/legacy $out/issue-sample

    tar -xzf ${example3d}
    tar -xzf ${thetaMode}
    tar -xzf ${bp4}
    tar -xzf ${legacy}

    mv example-3d/hdf5/* $out/git-sample/
    mv example-thetaMode/hdf5/* $out/git-sample/thetaMode/
    mv example-3d-bp4/* $out/git-sample/3d-bp4/
    mv legacy_datasets/* $out/git-sample/legacy/

    unzip -q ${noFields}
    unzip -q ${noParticles}
    mv no_fields $out/issue-sample/
    mv no_particles $out/issue-sample/

    unzip -q ${diags}
    mv diags/hdf5/data00000050.h5 $out/issue-sample/empty_alternate_fbpic_00000050.h5

    # make sure we do not need write access when reading data
    chmod u-w $out/git-sample/*.h5
    chmod u-w $out/git-sample/thetaMode/*.h5
    chmod u-w $out/issue-sample/*.h5
    chmod u-w $out/issue-sample/no_fields/*.h5
    chmod u-w $out/issue-sample/no_particles/*.h5
    find $out/git-sample/3d-bp4 -type f -exec chmod u-w {} \;
  ''
