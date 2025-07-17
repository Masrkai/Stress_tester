{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    gdb
    cmake
    ];

  shellHook = builtins.readFile ./shell.sh;

  # Avoid overriding PS1
  stdenv.shell.dontRebuildPrompt = true;
}
