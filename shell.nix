# Please Note I have a very limited understanding of the inner workings
# of certain things here I am just using what I have seen necessary for
# my application feel free to argue or change that as much as you please

{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  packages = with pkgs; [
    gcc
    gdb
    cmake
    valgrind             # Memory analysis
    perf                         # Performance profiling
    flamegraph       # Brendan Gregg's Flamegraph tool
    linuxPackages.perf           # For system-wide perf tools
    cppcheck             # Static analysis
    bear                  # Compilation database generator for clang tooling
    clang-tools          # Includes clang-tidy, clangd, etc.
    ccache               # Speeds up repeated builds
  ];

  # Environment variables to support profiling tools and compilers
  shellHook = builtins.readFile ./shell.sh;

  # Avoid overriding PS1
  stdenv.shell.dontRebuildPrompt = true;
}
