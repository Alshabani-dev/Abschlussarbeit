{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "gcc-dev-shell";

  buildInputs = [
    pkgs.gcc
    pkgs.gdb
    pkgs.pkg-config
    pkgs.cmake
  ];

  shellHook = ''
    echo "🔧 Nix Shell started – GCC & CMake ready!"
    gcc --version
    cmake --version
  '';
}
