{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  name = "gcc-dev-shell";
  buildInputs = [ pkgs.gcc pkgs.gdb pkgs.pkg-config pkgs.cmake ];
  shellHook = ''
    echo "\U0001F527 Nix Shell started \u2013 GCC & CMake ready!"
    gcc --version
    cmake --version
  '';
}
