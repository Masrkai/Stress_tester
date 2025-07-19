# Please Note I have a very limited understanding of the inner workings
# of certain things here I am just using what I have seen necessary for
# my application feel free to argue or change that as much as you please

{ pkgs ? import <nixpkgs> {} }:

let
  # use nixos-rebuild to get the system config
  systemConfig = (import <nixpkgs/nixos> {}).config;
  kernelPackages = systemConfig.boot.kernelPackages;
in

pkgs.mkShell {
  packages = with pkgs; [
    gcc
    cmake

    # Profiling
    flamegraph
      kernelPackages.perf # Needed By FlameGraph
  ];

  shellHook = ''
    ${builtins.readFile ./shell.sh}
  '';
}