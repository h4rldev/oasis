{
  description = "Development environment for the Oasis project";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    # TODO: Remove this once clay-renderers-small-fixes is merged
    clay.url = "github:h4rldev/clay-renderers-small-fixes/pull1";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    clay,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {inherit system;};
    in {
      devShells.default = pkgs.mkShell {
        packages = with pkgs; [
          alejandra
          gcc
          ffmpeg-headless
          clang-tools
          bear
          pkg-config
          raylib
          just
          clay.packages.${system}.default
        ];

        LD_LIBRARY_PATH = "${pkgs.lib.makeLibraryPath [
          pkgs.raylib
          pkgs.ffmpeg-headless
        ]}";
      };
    });
}
