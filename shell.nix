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

    stdenv.cc
    stdenv.cc.cc

    # Profiling
    flamegraph
      kernelPackages.perf # Needed By FlameGraph
  ];

  nativeBuildInputs = with pkgs; [
    gcc
    cmake
   ];

  #? Let me Explain why is this needed
  #? perf utility isn't a package in fact it's a kernel utility in linux
  #? Also it has kernel parameter
  #! /proc/sys/kernel/perf_event_paranoid
  #? And for
  #! /proc/sys/kernel/kptr_restrict
  #? it's to profile Kernel Calls

  #* In A technical sense of security you would
  #* never allow this on a "production" or "daily driver" machine
  #* As I am very conscious of this yet i need it for profiling
  #* A temporary solution would be enabling them in a shell temporarily then re-disabling them
  #* Sadly this can't be done in a shell and instead done system-wide so i need you to realize understand and evaluate
  #* the Risks comes with this

  shellHook = "

    ${builtins.readFile ./shell.sh}
    ${builtins.readFile ./profile.sh}
  ";
    # # Store original values to restore on exit
    # export ORIG_KPTR_RESTRICT=$(cat /proc/sys/kernel/kptr_restrict 2>/dev/null || echo "unknown")
    # export ORIG_PERF_PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo "unknown")

    # # Set kernel parameters for profiling
    # echo "Setting kernel parameters for profiling..."
    # echo "Original kptr_restrict: $ORIG_KPTR_RESTRICT"
    # echo "Original perf_event_paranoid: $ORIG_PERF_PARANOID"

    # sudo sysctl -w kernel.kptr_restrict=0
    # sudo sysctl -w kernel.perf_event_paranoid=0

    # echo "Kernel parameters set. Run 'exit' to restore original values."

    # # Set up exit trap to restore values
    # trap 'echo "Restoring kernel parameters..."; sudo sysctl -w kernel.kptr_restrict=$ORIG_KPTR_RESTRICT 2>/dev/null || true; sudo sysctl -w kernel.perf_event_paranoid=$ORIG_PERF_PARANOID 2>/dev/null || true' EXIT
}