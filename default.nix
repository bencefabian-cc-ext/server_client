{
  stdenv,
  lib,
  cmake,
  ninja,
  clang,
  boost,
}:

stdenv.mkDerivation {
  pname = "chat-app";
  version = "0.0.1";
  src = with lib.fileset; toSource {
    root = ./.;
    fileset = unions [
      ./client.cpp
      ./CMakeLists.txt
      ./main.cpp
    ];
  };

  nativeBuildInputs = [
    cmake
    ninja
    clang
  ];

  buildInputs = [
    boost
  ];
}
