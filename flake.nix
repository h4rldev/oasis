{
  description = "Development environment for the Oasis project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    # TODO: Remove this once clay-nix is merged
    clay.url = "github:h4rldev/clay-nix/pull1";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    clay,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {
        inherit system;
      };

      ffmpegWithPulse = pkgs.ffmpeg-headless.override {
        withPulse = true;
      };
    in {
      devShells.default = pkgs.mkShell {
        packages = with pkgs; [
          alejandra
          clang-tools
          bear
          pkg-config
          just
          clay.packages.${system}.default
        ];

        buildInputs = with pkgs; [
          libbsd
          ffmpegWithPulse
          raylib
          miniaudio
          unity-test
          libpulseaudio
        ];
      };
    });
}
