cmake_minimum_required(VERSION 3.10)
project(Compiler C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -O3")
set(LIB_SOURCES
    src/frontend/lexer/lexer.c
    src/frontend/parser/parserCore.c
    src/frontend/parser/parserExpression.c
    src/frontend/parser/parserStatement.c
    src/frontend/parser/parserDeclaration.c
    src/frontend/parser/parserFunction.c
    src/frontend/parser/parserType.c
    src/frontend/parser/parserAST.c
    src/frontend/parser/parserHelpers.c
    src/frontend/semantic/semanticCore.c
    src/frontend/semantic/semanticTable.c
    src/frontend/semantic/semanticSymbols.c
    src/frontend/semantic/semanticScope.c
    src/frontend/semantic/semanticTypes.c
    src/frontend/semantic/semanticCheck.c
    src/frontend/semantic/semanticBuiltins.c
    src/frontend/semantic/semanticUtils.c
    src/middleend/IR/ir.c
    src/middleend/IR/irHelpers.c
    src/middleend/IR/optimization.c
    src/backend/codeGeneration/codegen.c
    src/backend/codeGeneration/dataPool.c
    src/backend/codeGeneration/emiter.c
    src/backend/codeGeneration/stringBuffer.c
    src/backend/codeGeneration/variableHandling.c
    src/errorHandling/errorHandling.c
    src/errorHandling/errors.c
    src/modules/interface.c
    src/modules/build.c
)
add_library(compiler_lib ${LIB_SOURCES})
target_include_directories(compiler_lib PUBLIC
    src/frontend/lexer src/frontend/parser src/frontend/semantic
    src/middleend/IR src/backend/codeGeneration
    src/errorHandling src/modules
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