cmake_minimum_required(VERSION 3.10)
project(Compiler C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -O3")

# Source files (excluding main.c for library)
set(LIB_SOURCES
        src/lexer/lexer.c
        src/parser/parser.c
        src/errorHandling/errorHandling.c
        src/parser/parserHelpers.c
        src/semantic/typeChecker.c
        src/semantic/typeChecker.h
        src/semantic/symbolTable.c
        src/semantic/symbolTable.h
        src/semantic/builtIns.c
        src/semantic/builtIns.h
        src/semantic/semanticHelpers.c
        src/semantic/semanticHelpers.h
        src/errorHandling/errors.c
        src/IR/ir.c
        src/IR/ir.h
        src/IR/irHelpers.c
        src/IR/irHelpers.h
        src/IR/optimization.c
        src/IR/optimization.h
        src/codeGeneration/codegen.c
        src/codeGeneration/codegen.h
        src/codeGeneration/dataPool.h
        src/codeGeneration/dataPool.c
        src/codeGeneration/emiter.h
        src/codeGeneration/emiter.c
        src/codeGeneration/stringBuffer.h
        src/codeGeneration/stringBuffer.c
        src/codeGeneration/variableHandling.c
        src/codeGeneration/variableHandling.h
        src/modules/interface.c
        src/modules/interface.h
        src/modules/build.c
        src/modules/build.h
)

# Create compiler library
add_library(compiler_lib ${LIB_SOURCES})

target_include_directories(compiler_lib PUBLIC
        src/lexer
        src/parser
        src/errorHandling
        src/semantic
        src/IR
)

# Main executable
add_executable(orn src/main.c)
target_link_libraries(orn compiler_lib)

configure_file(
    ${CMAKE_SOURCE_DIR}/src/runtime/runtime.s
    ${CMAKE_BINARY_DIR}/runtime.s
    COPYONLY
)