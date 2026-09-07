{ pkgs }:

let
  pkgs_ = pkgs.extend (
    final: prev: {
      openpmd_api = prev.openpmd_api.override {
        cmake = final.cmakeCurses;
      };
    }
  );
in
let
  pkgs = pkgs_;
in
{
  default = pkgs.mkShell {
    packages = with pkgs; [
      clang-tools
      ninja
      nixfmt
      pre-commit
      ruff
    ];
    inputsFrom = [ pkgs.openpmd_api ];
  };
}
