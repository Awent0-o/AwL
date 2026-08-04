<div align="center">

# 🦉 AWL Compiler Version 0.8.1

**A custom toy programming language, compiled to native machine code.**

[![Build](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Language: C](https://img.shields.io/badge/language-C-orange)]()
[![Made with ❤️](https://img.shields.io/badge/made%20with-%E2%9D%A4-red)]()

</div>

## 📑 Table of Contents

- [Features](#-features)
- [Example](#%EF%B8%8F-example)
- [Build](#%EF%B8%8F-build)
- [Language Syntax](#-language-syntax)
- [Roadmap](#%EF%B8%8F-roadmap)


## 🆕 What's New in v0.8.1

### 🚀 Language
- Added support for arrays in func.
- Added `for`, `while`, `if / else` hes own scope

### ⚙️ Compiler
- Command-line flags:
  - `-h`, `--help`
  - `--version`
  - `-v`
  - `-c`
  - `-k`
  - `-S`
  - `-t`
  - `-g`
  - `-O1`, `-O2`, `-O3`
  - `-Wall`
  - `-Wextra`
  - `-o <file>`

## ✨ Features

| Feature                                               |  Status  |
|-------------------------------------------------------|----------|
| Variables & arithmetic                                | ✅worked |
| Comparison operators                                  | ✅worked |
| if / else, while / for                                | ✅worked |
| Functions                                             | ✅worked |
| Arrays but without opperands                          | ✅worked |
| Integretion C code                                    | ✅worked |
| Standard library                                      | ✅worked |
| Semantic analyze                                      | ✅worked |

## ⬇️ Example

**input.awl**
```awl
import "libs/add.awl";
import "math";
import "libs/hello.py";

func isPositive(int x) -> bool {
    return x > 0;
}

func printAll(string msg, int times) -> void {
    for(times) {
        print msg;
    }
}

x = add(3, 5);
print sin(90);
print x;
print hello.greet("Awent0_o");
printAll("hello", 3);
```

## 🏗️ Build

Requires `gcc` and a POSIX-like shell.

```bash
BASH
git clone https://github.com/Awent0-o/AwL
cd AwL
./awl main.awl
```

```shell
WIN
git clone https://github.com/Awent0-o/AwL
Download gcc from mingw/msys2 or wsl
cd AwL
if mingw/msys2 gcc main.c lexer/lexer.c parser/parser.c codegen/codegen.c -o awl.exe
if wsl ./awl main.awl
```
## 📖 Language Syntax

<details>
<summary>📖 Click to see full grammar</summary>


program     := (funcDecl | statement)*

import      := 'import' path (math -> c, libs/hello.py -> py, libs/add.awl -> awl) ';'

funcDecl    := 'func' IDENT '(' params? ')' block

statement   := assign | print | if | while | return | block | exprStmt

assign      := IDENT '=' expr ';'

print       := 'print' "string" expr expr ';' | 'print' f"string: {expr}" ';'

if          := 'if' '(' expr ')' block ('else' block)?

while       := 'while' '(' expr ')' block

for         := 'for' '('i, times')' block (i not necessarily)

return      := 'return' expr ';'

expr        := comparison

comparison  := arith (('<' | '>' | '<=' | '>=' | '==' | '/=') arith)?

arith       := term (('+' | '-') term)*

term        := factor (('' | '/') factor)

factor      := NUMBER | IDENT | call | '(' expr ')'

call        := IDENT '(' args? ')'
</details>

## 🧑‍💻 Project Structure

├── lexer/

│   ├── lexer.h

│   ├── ast.h

│   └── lexer.c

├── parser/

│   ├── parser.h

│   └── parser.c

├── codegen/

│   ├── codegen.h

│   └── codegen.c

├── semantic/

│   ├── semantic.h

│   └── semantic.c

├── main.c

├── main.awl

└── awl

## 🗺️ Roadmap

- [x] Variables & arithmetic
- [x] Functions
- [x] Arrays
- [x] Lib, you can made his on python, c, awl (for now)
- [x] Semantic but bad for it, i could add line detect

---