<div align="center">

# 🦉 AWL Compiler Version 0.8.2

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
- Added pointers and addresses: `int* p`, `ptr<int>`, `p = &x`, `*p = 5`
- Added logical operators: `!`, `!=`, `&&`, `||`
- `func main()` is optional and can be written without `-> int`

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
| Pointers & addresses                                  | ✅worked |
| Logical operators `!`, `!=`, `&&`, `\|\|`             | ✅worked |
| Optional `func main()`                                | ✅worked |
| Integretion C code                                    | ✅worked |
| Standard library                                      | ✅worked |

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
if mingw/msys2 gcc main.c lexer/lexer.c parser/parser.c parser/expr_parser.c parser/stmt_parser.c parser/func_parser.c codegen/c/codegen.c codegen/c/utils_codegen.c codegen/c/expr_codegen.c codegen/c/stmt_codegen.c codegen/c/import_codegen.c codegen/c/bin_codegen.c codegen/python/codepy.c utils/flags.c utils/file_utils.c utils/string_utils.c -o awl.exe
if wsl ./awl main.awl
```
## 📖 Language Syntax

<details>
<summary>📖 Click to see full grammar</summary>


program     := (funcDecl | statement)*   (func main is optional)

import      := 'import' path (math -> c, libs/hello.py -> py, libs/add.awl -> awl) ';'

funcDecl    := 'func' IDENT '(' params? ')' ('->' type)? block

param       := type IDENT

type        := 'int' | 'float' | 'string' | 'bool' | 'void'
             | 'array' '<' type '>'
             | 'ptr' '<' type '>'
             | type '*'

statement   := assign | derefAssign | print | if | while | return | block | exprStmt

assign      := IDENT '=' expr ';'

derefAssign := '*' unary '=' expr ';'

print       := 'print' "string" expr expr ';' | 'print' f"string: {expr}" ';'

if          := 'if' '(' expr ')' block ('else' block)?

while       := 'while' '(' expr ')' block

for         := 'for' '('i, times')' block (i not necessarily)

return      := 'return' expr ';'

expr        := logicalOr

logicalOr   := logicalAnd ('||' logicalAnd)*

logicalAnd  := comparison ('&&' comparison)*

comparison  := arith (('<' | '>' | '<=' | '>=' | '==' | '!=' | '/=') arith)?

arith       := term (('+' | '-') term)*

term        := unary (('*' | '/') unary)*

unary       := '!' unary | '&' unary | '*' unary | factor

factor      := NUMBER | IDENT | call | '(' expr ')' | '[' args? ']'

call        := IDENT '(' args? ')'
</details>

## 🧑‍💻 Project Structure

├── lexer/

│   ├── lexer.h

│   ├── ast.h

│   └── lexer.c

├── parser/

│   ├── parser.h

│   ├── parser.c          # tokens, program, blocks, statements, imports

│   ├── expr_parser.c     # expressions and arrays

│   ├── stmt_parser.c     # if / while / for / print / return / assign

│   └── func_parser.c     # functions and type annotations

├── codegen/

│   ├── c/

│   │   ├── codegen.h

│   │   ├── codegen_internal.h

│   │   ├── codegen.c         # top-level C generation

│   │   ├── utils_codegen.c   # variable table and type helpers

│   │   ├── expr_codegen.c    # expressions and f-strings

│   │   ├── stmt_codegen.c    # statements and functions

│   │   ├── import_codegen.c  # .awl / .py / C imports

│   │   └── bin_codegen.c     # gcc invocation

│   └── python/

│       ├── codepy.h

│       └── codepy.c

├── utils/

│   ├── utils.h

│   ├── flags.c           # CLI flags and help

│   ├── file_utils.c      # file reading

│   └── string_utils.c    # string helpers

├── main.c

└── awl

## 🗺️ Roadmap

- [x] Variables & arithmetic
- [x] Functions
- [x] Arrays
- [x] Lib, you can made his on python, c, awl (for now)
- [x] Pointers, addresses and dereferencing
- [x] Logical operators and optional `main`
- [ ] Semantic analysis (removed in this version, planned rewrite)

---