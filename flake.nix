{
  description = "openPMD-api";

  # Nixpkgs / NixOS version to use.
  inputs.nixpkgs.url = "nixpkgs/nixos-26.05";

  outputs =
    { self, nixpkgs }:
    let

      # Generate a user-friendly version number.
      version = "0.18.0-dev";

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
      overlays.default =
        final: prev:
        let
          callPackage = final.callPackage;
        in
        {
          adios2 = callPackage ./nix/adios2 {
            python = final.python3;
          };
          openpmd_api = callPackage ./nix/openpmd_api {
            inherit version;
            python = final.python3;
            hdf5 = final.hdf5-mpi;
          };
        };

      # Provide some binary packages for selected system types.
      packages = forAllSystems (system: rec {
        inherit (nixpkgsFor.${system}) openpmd_api;
        # The default package for 'nix build'. This makes sense if the
        # flake provides only one package or there is a clear "main"
        # package.
        default = openpmd_api;
      });

      # # A NixOS module, if applicable (e.g. if the package provides a system service).
      # nixosModules.hello =
      #   { pkgs, ... }:
      #   {
      #     nixpkgs.overlays = [ self.overlays.default ];

      #     environment.systemPackages = [ pkgs.hello ];

      #     #systemd.services = { ... };
      #   };

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
