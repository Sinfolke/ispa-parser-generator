# ISPA Parser Generator

**ISPA Parser** is a high-performance, 
infrastructure-level parser generator written from scratch 
in modern C++ (C++20/C++23). It provide fully automatic AST generation,
Advanced, feature-rich lexer and extensible parser

---

## 🚀 Current State
- Core backend and frontend is implemented and the executable can be compiled on both **Windows** and **Linux**
- Output can be compiled and run
- Lexer is under heavy tests
- Parser (LL*) is mostly correct; Needs some changes and tests
- Provide the way to generate output to any language with **LangAPI** IR; **C++17** is the only target right now
---

## Lexer Capabilities
- Match tokens with quick DFA table
- Build AST-like tokens natively during lexing. **You can capture only whatever you need**
- Include semantic actions right into lexer. Context-sensitive lexing is possible [needs to throw further grammar with Advanced Rules]
- Match tokens inside other tokens without structure loss. Capture these tokens preserving their data type
- Match non-terminals right inside terminals (rules inside tokens) during lexing, separately from parsing. [needs implementation]
## Parser Capabilities
- Generate LL(\*) or LR(\*) parser [**LL(*) is the only aim to be stable right now**]
- Build automatic, fully typed and easy to traverse AST during parsing. No post-parsing or callback functions required.

## Both have
- Semantic actions with in-grammar python pseudocode with advanced rules
- Error messages and custom context-sensitive recovery strategies in fail blocks
- Nested rules, where one rule may encapsulate another rule
## Syntax example
```ispa
  STRING:
    '"'
        (@ '\\"' | @ [^"] | '${' @ expr '}')*  
    '"'
    {@}
  ;
  NUMBER:
    @ [0-9]+ ('.' @ [0-9]+)?
    @{decimal, floating}
  ;
  ID:
    @ ([a-zA-Z_][a-zA-Z0-9_]*)
    {@}
  ;
  expr:
      @ #logical
      {@}

      #logical:
          @ compare (@ LOGICAL_OP @ compare)*

          @{left, op, right}
      ;

      #compare:
          @ arithmetic (@ COMPARE_OP @ arithmetic)*

          @{first, op, sequence}
      ;

      #arithmetic:
          @ term (@ PLUS | MINUS @ term)*

          @{first, op, sequence}
      ;

      #term:
          @ value (@ MULTIPLE | DIVIDE | MODULO @ value)*

          @{first, op, sequence}
      ;

      #value:
          @ STRING | NUMBER | ID
          {@}
      ;

      #group:
          '(' @ expr ')'
          {@}
      ;
  ;
  
```

## 🔮 Future Roadmap

- [ ] **PLL Algorithm**: Finalize custom Predictive LL algorithm to seamlessly resolve left-recursion in LL parsers while preserving structural parity with LR parsers.
- [ ] **Multi-Target Code Generation**:
  - [ ] Python target emitter
- [ ] **Full LR Parser Stabilization**: Finalize integration and production readiness for LR(1), LALR, and LR(*) modes.
- [ ] **Grammar Standard Library (StdLib)**: Pre-packaged standard grammar definitions for common formats (JSON, XML, Math Expressions, C-like statements).
- [ ] Add features described in **concepts/**
---

## 🛠️ Build & Requirements
1. Clang compiler with libc++ on ubuntu, latest ubuntu version
2. MSVC cl compiler on windows
3. Exact version of cmake 4.3.2

**Note: YOU WILL NOT BE ABLE TO BUILD THIS WITH GCC YET**

**Note: build with clang is not tested on windows**

### Build on Ubuntu
Generate cmake build files. **Change ISPA_SOURCE_DIR to your local path**:
```sh
export ISPA_SOURCE_DIR=/mnt/5EE9F38E0E9F2DC9/ispa-parser
export CC=clang-20
export CXX=clang++-20
export CFLAGS="-stdlib=libc++"
export CXXFLAGS="-stdlib=libc++"
export LDFLAGS="-stdlib=libc++"
cmake -B cmake-build-release \ 
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$ISPA_SOURCE_DIR/cmake/toolchains/clang-libcpp.cmake \
-DVCPKG_TARGET_TRIPLET=x64-linux-libcxx \
-DVCPKG_OVERLAY_TRIPLETS=$ISPA_SOURCE_DIR/cmake/triplets
```
Build:
```sh
cmake --build cmake-build-release
```
---

### Build on Windows
Note:

- Change CMAKE_TOOLCHAIN_FILE to your vcpkg installation path if this does not work
- You might need to provide CMAKE_CXX_COMPILER variable pointing to cl.exe
- You might need to configure other variables MSVC require to build
```bat
cmake -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake"
```