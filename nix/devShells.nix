{ pkgs, openpmd_api }:
{
  default = pkgs.mkShell {
    packages = with pkgs; [
      clang-tools
      ninja
      nixfmt
      pre-commit
      ruff
    ];
    inputsFrom = [ openpmd_api ];
  };
}
