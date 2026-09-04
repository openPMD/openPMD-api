{ fetchFromGitHub }:
python-final: python-prev: {
  pybind11 = python-prev.pybind11.overrideAttrs (_: rec {
    version = "v3.1.0";
    name = "pybind11";

    src = fetchFromGitHub {
      owner = "pybind";
      repo = "pybind11";
      rev = version;
      sha256 = "sha256-rzpe7CrgIa5Df2OrB/9mxIJd3X5DA7FX0C+w7TcmAoQ=";
    };
  });
}
