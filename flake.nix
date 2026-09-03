{
  description = "";

  inputs = {
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, utils }: utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs { inherit system; };
    in
      {
        packages = {
          default = pkgs.callPackage ./default.nix {};
        };

        devShell = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
          buildInputs = with pkgs; [ clang-tools ];
          shellHook = ''
            function mkBuildDir() {
              case "$1" in
                debug|Debug|d|D)
                    BUILD_TYPE=Debug
                    BUILD_DIR=build_debug
                  ;;
                release|Release|r|R)
                    BUILD_TYPE=Release
                    BUILD_DIR=build_release
                  ;;
                *)
                  ;;
              esac
              cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
              rm compile_commands.json || true
              ln -s "$BUILD_DIR"/compile_commands.json .
            }
          '';
        };
      }
  );
}
