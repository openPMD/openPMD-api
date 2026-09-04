{
  description = "openPMD-api";

  # Nixpkgs / NixOS version to use.
  inputs.nixpkgs.url = "nixpkgs/nixos-26.05";

  outputs =
    { self, nixpkgs }:
    let

      # Derive a user-friendly version number from the CMake project version.
      version =
        let
          cmakeLists = builtins.readFile ./CMakeLists.txt;
          versionLine = builtins.head (
            builtins.filter (
              line:
              builtins.isString line && builtins.match ".*project\\(openPMD VERSION ([0-9.]+)\\).*" line != null
            ) (builtins.split "\n" cmakeLists)
          );
          versionString = builtins.head (
            builtins.match ".*project\\(openPMD VERSION ([0-9.]+)\\).*" versionLine
          );
        in
        "${versionString}-dev";

      # System types to support.
      supportedSystems = [
        "x86_64-linux"
        "x86_64-darwin"
        "aarch64-linux"
        "aarch64-darwin"
      ];

      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      # Nixpkgs instantiated for supported system types.
      nixpkgsFor = forAllSystems (
        system:
        import nixpkgs {
          inherit system;
          overlays = [ self.overlays.default ];
        }
      );

    in

    {

      # A Nixpkgs overlay.
      overlays.default = import ./nix/overlay.nix { inherit version; };

      # Provide some binary packages for selected system types.
      packages = forAllSystems (system: rec {
        inherit (nixpkgsFor.${system}) openpmd_api;
        # The default package for 'nix build'. This makes sense if the
        # flake provides only one package or there is a clear "main"
        # package.
        default = openpmd_api;
      });

      legacyPackages = nixpkgsFor;

      # Command line tools provided by the packages.
      apps = forAllSystems (
        system:
        let
          openpmd = nixpkgsFor.${system}.openpmd_api;
        in
        {
          ls = {
            type = "app";
            program = "${openpmd}/bin/openpmd-ls";
          };
          pipe = {
            type = "app";
            program = "${openpmd}/bin/openpmd-pipe";
          };
          default = {
            type = "app";
            program = "${openpmd}/bin/openpmd-ls";
          };
        }
      );

      # Development shell for working on openPMD-api.
      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
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
      );

      # The formatter used by 'nix fmt'.
      formatter = forAllSystems (system: nixpkgsFor.${system}.nixfmt-tree);

      # Tests run by 'nix flake check' and by Hydra.
      checks = forAllSystems (
        system:
        with self.packages.${system};

        {
          # Additional tests, if applicable.
          test = openpmd_api.override { doCheck = true; };
        }
      );

    };
}
