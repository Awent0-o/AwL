<div align="center">

# 🦉 AWL Compiler Version 0.2

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


## ✨ Features

| Feature                             | Status |
|-------------------------------------|----------|
| Variables (only int) & arithmetic   | ✅worked |
| Comparison operators                | ✅worked |
| if / else, while                    | ✅worked |
| Functions                           | ✅worked |
| Type var                            | ✅worked |
| Arrays                              | 🚧 planned |
| Standard library                    | ❌ not started |

## ⬇️ Example

**input.awl**
```awl
func add(a, b) {
    return a + b;
}

x = 5;
y = 10;

if (x < y) {
    print "x is smaller than y";
}

print add(x, y);

while (x < 15) {
    print x;
    x = x + 1;
}
```

**Output**

x is smaller than y

15

5

6

7

...

14

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

funcDecl    := 'func' IDENT '(' params? ')' block

statement   := assign | print | if | while | return | block | exprStmt

assign      := IDENT '=' expr ';'

print       := 'print' (expr | STRING) ';'

if          := 'if' '(' expr ')' block ('else' block)?

while       := 'while' '(' expr ')' block

return      := 'return' expr ';'

expr        := comparison

comparison  := arith (('<' | '>' | '<=' | '>=' | '==' | '!=') arith)?

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

├── main.c

├── main.awl

└── awl

## 🗺️ Roadmap

- [x] Variables & arithmetic
- [x] Functions
- [ ] Arrays
- [ ] Lib

---