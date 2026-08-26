{
  description = "Environnement de développement Bare-Metal pour ESP32-S3";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    esp-dev = {
      url = "github:mirrexagon/nixpkgs-esp-dev";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, esp-dev }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
    in
    {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [ esp-dev.overlays.default ];
          };
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              # Toolchain cross-compilation Xtensa pour ESP32-S3
              gcc-xtensa-esp32s3-elf

              # Émulateur (inclut qemu-system-xtensa)
              qemu

              # Débugueur (optionnel mais recommandé pour QEMU -s -S)
              gdb

              # Utilitaires système
              bash
              gnumake
              coreutils
            ];

            shellHook = ''
              echo "======================================================="
              echo "🚀 Environnement Bare-Metal ESP32-S3 actif !"
              echo "   - Toolchain GCC : $(xtensa-esp32s3-elf-gcc --version | head -n 1)"
              echo "   - Émulateur QEMU: $(qemu-system-xtensa --version | head -n 1)"
              echo "======================================================="
              echo "Lancez './run.sh' pour compiler et exécuter dans QEMU."
            '';
          };
        });
    };
}
