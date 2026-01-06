# Orn Lang

*A modern low-level programming language with clear error messages and fast builds*

<p align="center">
  <img src="assets/orn.png" alt="Orn Lang Logo" width="120">
</p>
<p align="center">
  <a href="#why">Why?</a> •
  <a href="#language-goals">Goals</a> •
  <a href="#performance-architecture">Performance</a> •
  <a href="#getting-started">Getting Started</a> •
  <a href="#usage">Usage</a>
</p>

---

## Introduction

Orn is a strongly typed programming language designed for **performance** and **clarity**. Inspired by TypeScript's approach to bringing type safety to dynamic languages, Orn aims to make low-level programming more accessible with modern type annotations and clear syntax. Its primary goals are **fast compilation**, **precise error feedback**, and a **clean, maintainable architecture**.

The syntax is designed to be approachable for developers coming from high-level languages while providing direct control over low-level operations. The long-term vision is to evolve into a fully **object-oriented language**.

---

## Why?

Many low-level languages have steep learning curves that intimidate developers from high-level backgrounds. Orn bridges this gap by offering:

* **Modern syntax** – TypeScript-style type annotations with `const` and `let`
* **Clear error feedback** – Error messages are precise and tell you exactly how to fix problems
* **Low-level control** – Direct access to memory and performance-critical operations
* **Fast compile times** – Efficient compilation pipeline for quick iteration
* **Strong type guarantees** – Minimize runtime surprises with a robust type system
* **Gradual learning curve** – Start with high-level concepts, dive into low-level details as needed

---

## Language Goals

* **🎯 Clear Error Feedback** – Errors are actionable and easy to understand
* **⚡ Fast Compilation** – Quick iteration cycles for development
* **🔒 Type Safety** – Strong typing at the core
* **🚀 Path to OOP** – Current syntax is just the beginning
* **🛠️ Simplicity First** – Minimalism without sacrificing power

---

## Performance Architecture

Orn uses a **zero-copy reference design** inspired by production compilers like Clang and Rust:

```md
Source Buffer (one malloc)
    ↓
Tokens (ptr+len references)
    ↓
AST (ptr+len references)
    ↓
Semantic Analysis (ptr+len references)
    ↓
IR (Three-Address Code)
    ↓
IR Optimization (multiple passes)
    ↓
Assembly (new strings)
```

**Benefits:**

* Single source allocation, thousands fewer mallocs
* No duplicate string storage throughout pipeline
* Better memory locality and faster compilation
* References become copies only in final assembly output

Traditional compilers duplicate every identifier dozens of times. Orn references the original buffer until code generation.

---

## Getting Started

### Prerequisites

You'll need:

* **[GCC](https://gcc.gnu.org/)** or **[Clang](https://clang.llvm.org/)** – C compiler
* **[CMake](https://cmake.org/)** (3.10+) – Build system
* **[Git](https://git-scm.com/)** – Version control
* **[Valgrind](https://valgrind.org/)** *(optional)* – Memory debugging

### Installation

```bash
# Clone the repository
git clone https://github.com/Blopaa/Orn.git
cd Orn

# Build the project
mkdir build && cd build
cmake ..
cmake --build .
```

You can now run Orn on your own programs:

```bash
./orn program.orn
```

Or, for verbose compilation output:

```bash
./orn --verbose program.orn
```

This will perform lexical analysis, parsing, semantic analysis, and IR generation.

---

## Usage

### Example Program

```ts
const x: int = 42;
let rate: float = 3.14;
const msg: string = "Hello, World!";
const b: bool = true; // bools and ints cannot mix

if x > 0 {
   print(msg);
}

let i: int = 0;
while i <= 10 {
    print(i);
    i++;
}

// simple add function
fn add(a: int, b: int) -> int {
    return a + b;
}

print(add(3, 5));
```

### Error Example

Orn provides actionable error messages:

```ts
error [E2005]: cannot assign to constant (x)
  --> source.orn:2:1
   |
 2 | x = 20;
   | ^
   |
   = help: assignment to immutable value
   = note: constants cannot be modified after initialization
   = suggestion: use a mutable variable instead

error [E1001]: mismatched types (x)
  --> source.orn:2:11
   |
 2 | const x: int = "hello";
   |                ^^^^^^^
   |
   = expected `int`, found `string`
   = note: string literals cannot be assigned to int variables
   = suggestion: change variable type or cast the value
```

---

## Join Us

We welcome contributors and feedback!

* Visit the [GitHub repository](https://github.com/Blopaa/Orn)
* Check the [contribution guidelines](CONTRIBUTING.md)
* Report issues on the [issue tracker](https://github.com/Blopaa/Orn/issues)
* Explore the [roadmap](https://github.com/Blopaa/Orn/projects)
* Join our [Discord](https://discord.gg/E8qqVC9jcf)
* Check [Benchmark](https://github.com/Blopaa/Orn/tree/main/benchmarks/benchmark.md) info

---

### Attribution

This README's structure and presentation were inspired by [TheDevConnor / Luma](https://github.com/TheDevConnor/luma).

---

<p align="center">
  <strong>Built with ❤️ </strong>
</p>
