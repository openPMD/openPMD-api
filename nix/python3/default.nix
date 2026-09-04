{
  lib,
  python,
  packageOverlays,
}:

let
  self = python.override {
    inherit self;
    packageOverrides = lib.composeManyExtensions packageOverlays;
  };
in
self
