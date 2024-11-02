{ pkgs ? import <nixpkgs> {} }:

let
  stdenv = pkgs.stdenv;
  cmake = pkgs.cmake;
in
pkgs.mkShell {
  buildInputs = [ cmake ];

  shellHook = ''
    echo "Entering development environment"

    # Save the current PS1 and source shell settings
    export ORIGINAL_PS1="$PS1"

    # Preserve the current shell environment
    if [ -n "$BASH" ]; then
      source ~/.bashrc
    elif [ -n "$ZSH_VERSION" ]; then
      source ~/.zshrc
    fi

    # Restore the original PS1 to prevent nix-shell from changing it
    export PS1="$ORIGINAL_PS1"

    # Create and navigate to the build directory
    build_dir="build"
    mkdir -p "$build_dir"
    cd "$build_dir"

    # Run CMake and Make
    cmake ..
    make

    echo "Build completed. You can find the output in the '$build_dir' directory."
    exit
  '';

  # Avoid overriding PS1
  stdenv.shell.dontRebuildPrompt = true;
}
