{
  description = "openPMD-api";

  # Nixpkgs / NixOS version to use.
  inputs.nixpkgs.url = "nixpkgs/nixos-26.05";

  outputs =
    { self, nixpkgs }:
    let

      # Derive the version number from include/openPMD/version.hpp.
      version =
        let
          versionHeader = builtins.readFile ./include/openPMD/version.hpp;
          lines = builtins.filter builtins.isString (builtins.split "\n" versionHeader);

          # Extract the value of a '#define NAME value' macro from version.hpp.
          macroValue =
            name:
            let
              line = builtins.head (builtins.filter (l: builtins.match ".*#define ${name} .*" l != null) lines);
            in
            builtins.head (builtins.match ".*#define ${name} (.*)" line);

          major = macroValue "OPENPMDAPI_VERSION_MAJOR";
          minor = macroValue "OPENPMDAPI_VERSION_MINOR";
          patch = macroValue "OPENPMDAPI_VERSION_PATCH";
          # The version label macro is a quoted string, e.g. OPENPMDAPI_VERSION_LABEL "dev".
          label = builtins.replaceStrings [ "\"" ] [ "" ] (macroValue "OPENPMDAPI_VERSION_LABEL");
        in
        if label == "" then "${major}.${minor}.${patch}" else "${major}.${minor}.${patch}-${label}";

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
        import ./nix/apps.nix {
          openpmd_api = nixpkgsFor.${system}.openpmd_api;
        }
      );

      # Development shell for working on openPMD-api.
      devShells = forAllSystems (
        system:
        import ./nix/devShells.nix {
          pkgs = nixpkgsFor.${system};
        }
      );

      # The formatter used by 'nix fmt'.
      formatter = forAllSystems (system: nixpkgsFor.${system}.nixfmt-tree);

      # Tests run by 'nix flake check' and by Hydra.
      checks = forAllSystems (
        system:
        with self.packages.${system};

        let
          pkgs = nixpkgsFor.${system};
        in
        {
          test = openpmd_api.override {
            doCheck = true;
            samples = pkgs.openpmd_example_datasets;
          };
        }
      );

    };
}
