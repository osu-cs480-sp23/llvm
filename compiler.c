#include <stdio.h>
#include <stdlib.h>

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>

#include "hash.h"

LLVMValueRef build_number(float val) {
    return LLVMConstReal(LLVMFloatType(), val);
}

LLVMValueRef build_binop(
    LLVMValueRef lhs,
    LLVMValueRef rhs,
    char op,
    LLVMBuilderRef builder
) {
    if (LLVMIsUndef(lhs) || LLVMIsUndef(rhs)) {
        return LLVMGetUndef(LLVMFloatType());
    }
    switch (op) {
        case '+':
            return LLVMBuildFAdd(builder, lhs, rhs, "add_result");
        case '-':
            return LLVMBuildFSub(builder, lhs, rhs, "sub_result");
        case '*':
            return LLVMBuildFMul(builder, lhs, rhs, "mul_result");
        case '/':
            return LLVMBuildFDiv(builder, lhs, rhs, "div_result");
        default:
            fprintf(stderr, "Error: invalid operator: %c\n", op);
            return LLVMGetUndef(LLVMFloatType());
    }
}

LLVMValueRef build_alloca(char* name, LLVMBuilderRef builder) {
    LLVMBasicBlockRef insert_blk = LLVMGetInsertBlock(builder);
    LLVMValueRef curr_fn = LLVMGetBasicBlockParent(insert_blk);
    LLVMBasicBlockRef entry_blk = LLVMGetEntryBasicBlock(curr_fn);
    LLVMValueRef first_instr = LLVMGetFirstInstruction(entry_blk);

    LLVMBuilderRef alloca_builder = LLVMCreateBuilder();
    if (LLVMIsAInstruction(first_instr)) {
        LLVMPositionBuilderBefore(alloca_builder, first_instr);
    } else {
        LLVMPositionBuilderAtEnd(alloca_builder, entry_blk);
    }

    LLVMValueRef alloca =
        LLVMBuildAlloca(alloca_builder, LLVMFloatType(), name);
    LLVMDisposeBuilder(alloca_builder);
    return alloca;
}

LLVMValueRef build_assignment(
    char* lhs,
    LLVMValueRef rhs,
    struct hash* symbols,
    LLVMBuilderRef builder
) {
    if (!hash_contains(symbols, lhs)) {
        LLVMValueRef alloca = build_alloca(lhs, builder);
        hash_insert(symbols, lhs, alloca);
    }
    return LLVMBuildStore(builder, rhs, hash_get(symbols, lhs));
}

LLVMValueRef build_variable_val(
    char* name,
    struct hash* symbols,
    LLVMBuilderRef builder
) {
    if (!hash_contains(symbols, name)) {
        fprintf(stderr, "Unknown variable: %s\n", name);
        return LLVMGetUndef(LLVMFloatType());
    }
    return LLVMBuildLoad2(
        builder,
        LLVMFloatType(),
        hash_get(symbols, name),
        name
    );
}

int main() {
    LLVMModuleRef module = LLVMModuleCreateWithName("foo.code");
    struct hash* symbols = hash_create();

    LLVMTypeRef foo_type = LLVMFunctionType(
        LLVMFloatType(), NULL, 0, 0
    );
    LLVMValueRef foo_fn =
        LLVMAddFunction(module, "foo", foo_type);
    LLVMBasicBlockRef entry_block =
        LLVMAppendBasicBlock(foo_fn, "entry");

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry_block);

    LLVMValueRef expr1 = build_binop(
        build_number(4.0),
        build_number(2.0),
        '*',
        builder
    );
    LLVMValueRef expr2 = build_binop(
        build_number(8),
        expr1,
        '+',
        builder
    );

    LLVMValueRef assgn1 = build_assignment("a", expr2, symbols, builder);

    LLVMValueRef expr3 = build_binop(
        build_variable_val("a", symbols, builder),
        build_number(4),
        '/',
        builder
    );
    LLVMValueRef assgn2 = build_assignment("b", expr3, symbols, builder);

    LLVMBuildRet(builder, build_variable_val("b", symbols, builder));

    LLVMVerifyModule(module, LLVMAbortProcessAction, NULL);
    char* out = LLVMPrintModuleToString(module);
    printf("%s\n", out);

    LLVMDisposeModule(module);
    LLVMDisposeBuilder(builder);
    return 0;
}
