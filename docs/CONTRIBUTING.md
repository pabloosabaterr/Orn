# Contributing to Orn

Thank you for your interest in contributing to Orn, a modern low-level programming language with clear error messages and fast builds!

This document outlines guidelines to help you contribute effectively. Whether you're reporting bugs, suggesting features, or submitting code, we appreciate your help.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How to Contribute](#how-to-contribute)
- [Development Setup](#development-setup)
- [Style Guide](#style-guide)
- [Testing](#testing)
- [License](#license)

## Code of Conduct

By participating in this project, you agree to abide by the [Contributor Covenant Code of Conduct](https://www.contributor-covenant.org/). We strive to foster a welcoming and inclusive community. Join us on [Discord](https://discord.gg/E8qqVC9jcf) for design discussions and questions.

## How to Contribute

### Reporting Issues

If you find a bug, please open an [issue](https://github.com/pabloosabaterr/Orn/issues) with:

- A clear and descriptive title
- A minimal `.orn` snippet that reproduces the problem
- Expected and actual behavior
- The compiler phase where the issue occurs if known (lexer, parser, type checker, IR, codegen)
- Any relevant flags used (`--ast`, `--ir`, `--verbose`)

### Suggesting Features

We welcome new ideas! Open an issue with the `[Feature Request]` prefix including:

- A description of the feature and its use cases
- How it fits Orn's philosophy of explicitness, safety, and clear error feedback
- Example `.orn` syntax if it involves new language constructs

### Submitting Code

1. Fork the repository and clone your fork locally
2. Create a feature branch (e.g., `feat/bitwise-operators` or `fix/array-bounds`)
3. Implement changes following the phase-by-phase pattern: lexer → parser → type checker → IR → codegen → tests
4. Add tests using the Unity framework
5. Run the full test suite and ensure CI passes
6. Commit with clear messages using `phase: description` format (e.g., `parser: add bitwise AND expression node`)
7. Push your branch and open a pull request targeting `main`

## Development Setup

Ensure you have GCC (or Clang), CMake 3.10+, and Git installed. Then:

```bash
git clone --recurse-submodules https://github.com/pabloosabaterr/Orn.git
cd Orn
mkdir build && cd build
cmake ..
make
```

The `--recurse-submodules` flag pulls in the [Unity](https://github.com/ThrowTheSwitch/Unity) test framework. Verify everything works:

```bash
./orn --help        # Compiler options
./test_frontend     # Run test suite
```

### Useful Compiler Flags

- `--ast` — Dump the AST for all modules
- `--ir` — Dump the intermediate representation
- `--verbose` — Show full build pipeline (module discovery, compilation order, linking)
- `-O0` to `-O3` / `-Ox` — Optimization levels

These are invaluable when debugging a new feature through the pipeline.

## Style Guide

- Follow consistent indentation (one tab)
- Use `camelCase` for functions/variables, `PascalCase` for types/structs, `UPPER_SNAKE_CASE` for constants `lower_snake_case` for tests
- Write clear and concise comments where intent isn't obvious
- Prefer explicit error handling — follow Orn's structured error format (`error [EXXXX]` with context, notes, and suggestions)
- Free what you allocate — run Valgrind if you're touching allocation logic
- Use the existing code style as a reference

## Testing

Tests use the [Unity](https://github.com/ThrowTheSwitch/Unity) framework and live under `tests/frontend/`, organized by feature (variables, functions, arrays, pointers, structs, control flow, etc.). Before submitting a pull request:

- Write tests for both success and failure cases
- Name tests descriptively: `test_<feature>_<scenario>` and `test_<feature>_<scenario>_fails`
- Register new tests in `frontend.c` and update `CMakeLists.txt` if adding new files
- Run the full suite: `./build/test_frontend`

CI runs automatically on all pull requests via GitHub Actions.

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).

---

Thank you for helping make Orn awesome!