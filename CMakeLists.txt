cmake_minimum_required(VERSION 3.10)
project(Compiler C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -O3")

set(LIB_SOURCES
    src/lexer/lexer.c
    src/parser/parserCore.c
    src/parser/parserExpression.c
    src/parser/parserStatement.c
    src/parser/parserDeclaration.c
    src/parser/parserFunction.c
    src/parser/parserType.c
    src/parser/parserAst.c
    src/parser/parserHelpers.c
    src/semantic/typeChecker.c
    src/semantic/symbolTable.c
    src/semantic/builtIns.c
    src/semantic/semanticHelpers.c
    src/errorHandling/errorHandling.c
    src/errorHandling/errors.c
    src/IR/ir.c
    src/IR/irHelpers.c
    src/IR/optimization.c
    src/codeGeneration/codegen.c
    src/codeGeneration/dataPool.c
    src/codeGeneration/emiter.c
    src/codeGeneration/stringBuffer.c
    src/codeGeneration/variableHandling.c
    src/modules/interface.c
    src/modules/build.c
)

add_library(compiler_lib ${LIB_SOURCES})
target_include_directories(compiler_lib PUBLIC
    src/lexer src/parser src/errorHandling src/semantic
    src/IR src/codeGeneration src/modules
)

add_executable(orn src/main.c)
target_link_libraries(orn compiler_lib)

configure_file(src/runtime/runtime.s runtime.s COPYONLY)

if(EXISTS "${CMAKE_SOURCE_DIR}/unity/src/unity.c" AND 
   EXISTS "${CMAKE_SOURCE_DIR}/tests/frontEnd/frontend.c")
    enable_testing()
    add_library(unity unity/src/unity.c)
    target_include_directories(unity PUBLIC unity/src)
    
    add_executable(test_frontend
        tests/frontEnd/frontend.c
        tests/frontEnd/basic/operators.c
        tests/frontEnd/declarations/variables.c
        tests/frontEnd/declarations/functions.c
        tests/frontEnd/types/structs.c
        tests/frontEnd/types/pointers.c
        tests/frontEnd/types/arrays.c
        tests/frontEnd/expressions/arithmetic.c
        tests/frontEnd/expressions/casting.c
        tests/frontEnd/expressions/ternary.c
        tests/frontEnd/expressions/assignment.c
        tests/frontEnd/statements/controlFlow.c
        tests/frontEnd/statements/scopes.c
        tests/frontEnd/modules/exports.c
        tests/frontEnd/integration/programs.c
    )
    target_link_libraries(test_frontend PRIVATE compiler_lib unity)
    target_include_directories(test_frontend PRIVATE
        src 
        unity/src 
        tests/frontEnd
    )
    add_test(NAME test_frontend COMMAND test_frontend)
endif()