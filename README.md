# Pixel Language Documentation

## Overview

Pixel is a statically-typed programming language that compiles to C. It features a syntax inspired by modern languages with built-in support for structs, functions, arrays, pointers, and C interop and also probably more stuff that ill make soon (i mean, duh, it's in beta)

## Table of Contents
- [Basic Syntax](#basic-syntax)
- [Data Types](#data-types)
- [Variables and Declarations](#variables-and-declarations)
- [Functions](#functions)
- [Structs](#structs)
- [Control Flow](#control-flow)
- [Arrays](#arrays)
- [Pointers](#pointers)
- [C Interoperability](#c-interoperability)
- [Modules and Imports](#modules-and-imports)

---

## Basic Syntax

### Comments
```
// Single line comment
```

### Program Structure
A Pixel program consists of:
- Function definitions
- Struct definitions
- Global variables
- Import statements

---

## Data Types

### Primitive Types
| Type | Description | Example |
|------|-------------|---------|
| `Int` | Signed integer | `Int x = 42` |
| `Float` | Floating-point number | `Float pi = 3.14` |
| `Bool` | Boolean value | `Bool flag = true` |
| `String` | String literal | `String name = "Pixel"` |
| `Void` | No return value | `fn main() -> Void` |

### Type Modifiers
- **Pointer**: `*` after type (e.g., `Int*`)
- **Array**: `Array(Type)` (e.g., `Array(Int)`)

### Literals
```
42          // Integer
3.14        // Float
true        // Boolean
false       // Boolean
"hello"     // String
[1, 2, 3]   // Array literal
```

---

## Variables and Declarations

### Declaration Syntax
```
Type variable_name = initial_value
```

### Examples
```
Int age = 25
Float price = 99.99
Bool is_active = true
String greeting = "Hello"
Int* ptr = nullptr
Array(Int) numbers = [1, 2, 3, 4, 5]
```

### Assignment
```
age = 26
price = 89.99
```

### Declaration Requirements
- Variables must be declared before use
- Type must be specified at declaration
- Initialization is optional but recommended

---

## Functions

### Function Definition
```
fn function_name(param1: Type1, param2: Type2) -> ReturnType {
    // function body
    return expression
}
```

### Examples
```
fn add(a: Int, b: Int) -> Int {
    return a + b
}

fn greet(name: String) -> Void {
    // Void functions don't need a return statement
}

fn factorial(n: Int) -> Int {
    if (n <= 1) {
        return 1
    }
    return n * factorial(n - 1)
}
```

### Function Calls
```
let result = add(5, 3)
greet("Alice")
```

---

## Structs

### Struct Definition
```
struct StructName {
    field1: Type1
    field2: Type2
    // ...
}
```

### Examples
```
struct Person {
    name: String
    age: Int
    active: Bool
}

struct Point {
    x: Float
    y: Float
}

struct Node {
    data: Int
    next: Node*
}
```

### Struct Usage
```
let person = Person
person->name = "Bob"
person->age = 30
```

### Field Access
```
// Dot notation for field access
person->name  // Access name field
person->age   // Access age field
```

---

## Control Flow

### If Statement
```
if (condition) {
    // then block
}
```

### Examples
```
if (age >= 18) {
    // Adult
}

if (score > 90) {
    // Excellent
}
```

### While Loop
```
while (condition) {
    // loop body
}
```

### Examples
```
while (count < 10) {
    count = count + 1
}

while (running) {
    // main loop
}
```

### Return Statement
```
return expression
```

### Examples
```
return 42
return x + y
return "success"
```

---

## Arrays

### Array Declaration
```
Array(Type) array_name = [elem1, elem2, ...]
```

### Examples
```
Array(Int) numbers = [1, 2, 3, 4, 5]
Array(String) names = ["Alice", "Bob", "Charlie"]
Array(Float) prices = [19.99, 29.99, 39.99]
```

### Array Access
```
numbers[0]  // First element
names[1]    // Second element
```

---

## Pointers

### Pointer Declaration
```
Type* pointer_name
```

### Dereference
```
@pointer_name  // Dereference operator
```

### Examples
```
Int value = 42
Int* ptr = value
@ptr  // Returns 42
```

---

## C Interoperability

### External Code Blocks
```
ext {
    // Raw C code
    #include <stdio.h>
    printf("Hello from C!\n");
}
```

### Header Binding
```
#bind "file.h"  // Include C header in generated output
```

### Examples
```
// Include a C library
#bind "math.h"

// Use C functions
ext {
    double sqrt(double x);
}

// Call from Pixel
let result = sqrt(16.0)  // 4.0
```

---

## Modules and Imports

### Import Syntax
```
#use "path/to/file.px"
```

### Import Behavior
- Files are parsed and imported at compile time
- Circular imports are prevented
- Relative paths are resolved based on the importing file's location

### Standard Library
- `lib/builtins.px` is automatically imported
- Provides core language functionality

### Examples
```
#use "math.px"
#use "lib/collections.px"
#use "../utils.px"
```

---

## Language Features Summary

### Type System
- Static typing with type inference
- Primitive types: Int, Float, Bool, String, Void
- Composite types: Arrays, Structs
- Pointers for C interoperability

### Memory Model
- No manual memory management
- Variables have scope-based lifetime
- Structs are value types

### Error Handling
- Compile-time type checking
- Parser errors with descriptive messages
- Circular import detection

### Code Organization
- Module system with `#use` imports
- Structs for data encapsulation
- Functions for code reuse

---

## Best Practices

1. **Type Safety**: Always specify explicit types for variables
2. um something

---

## Command Line Usage

### Compilation
```
pixel source.px
```

--keep-c-file is an Optional flag. If provided, the intermediate C file (output.c) is not deleted after compilation; otherwise it is removed automatically.

### Output
Generates an executable

---

## Language Design Philosophy

Pixel aims to be:
- **something**:  yeah whatever

---

## Notes

- The language automatically imports `lib/builtins.px` for core functionality
- Structs must be defined before use
- Variables must be declared in scope before use
- Arrays are 0-indexed
- The `@` operator dereferences pointers
- The `->` operator accesses struct fields