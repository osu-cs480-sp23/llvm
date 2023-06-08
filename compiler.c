#include <stdio.h>
#include <stdlib.h>

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

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
        case '<':
            lhs = LLVMBuildFCmp(builder, LLVMRealULT, lhs, rhs, "lt_result");
            return LLVMBuildUIToFP(builder, lhs, LLVMFloatType(), "cast_result");
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

LLVMValueRef build_if_else(struct hash* symbols, LLVMBuilderRef builder) {
    LLVMValueRef cond = build_binop(
        build_variable_val("b", symbols, builder),
        build_number(8),
        '<',
        builder
    );
    cond = LLVMBuildFCmp(
        builder,
        LLVMRealUNE,
        cond,
        build_number(0),
        "cond_result"
    );

    LLVMBasicBlockRef curr_blk = LLVMGetInsertBlock(builder);
    LLVMValueRef curr_fn = LLVMGetBasicBlockParent(curr_blk);
    LLVMBasicBlockRef then_blk = LLVMAppendBasicBlock(curr_fn, "then");
    LLVMBasicBlockRef else_blk = LLVMAppendBasicBlock(curr_fn, "else");
    LLVMBasicBlockRef continue_blk = LLVMAppendBasicBlock(curr_fn, "continue");

    LLVMBuildCondBr(builder, cond, then_blk, else_blk);

    /*
     * Then block.
     */
    LLVMPositionBuilderAtEnd(builder, then_blk);
    LLVMValueRef a_times_b = build_binop(
        build_variable_val("a", symbols, builder),
        build_variable_val("b", symbols, builder),
        '*',
        builder
    );
    LLVMValueRef then_assign =
        build_assignment("c", a_times_b, symbols, builder);
    LLVMBuildBr(builder, continue_blk);

    /*
     * Else block.
     */
    LLVMPositionBuilderAtEnd(builder, else_blk);
    LLVMValueRef a_plus_b = build_binop(
        build_variable_val("a", symbols, builder),
        build_variable_val("b", symbols, builder),
        '+',
        builder
    );
    LLVMValueRef else_assign =
        build_assignment("c", a_plus_b, symbols, builder);
    LLVMBuildBr(builder, continue_blk);

    LLVMPositionBuilderAtEnd(builder, continue_blk);
    return LLVMBasicBlockAsValue(continue_blk);
}

void generate_obj_file(char* filename, LLVMModuleRef module) {
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();

    char* triple = LLVMGetDefaultTargetTriple();

    LLVMTargetRef target = NULL;
    char* error = NULL;
    LLVMGetTargetFromTriple(triple, &target, &error);
    if (error) {
        fprintf(stderr, "Error: %s\n", error);
        abort();
    }

    char* cpu = LLVMGetHostCPUName();
    char* features = LLVMGetHostCPUFeatures();

    fprintf(stderr, "== triple: %s\n", triple);
    fprintf(stderr, "== cpu: %s\n", cpu);
    fprintf(stderr, "== features: %s\n", features);

    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
        target,
        triple,
        cpu,
        features,
        LLVMCodeGenLevelNone,
        LLVMRelocDefault,
        LLVMCodeModelDefault
    );

    LLVMTargetMachineEmitToFile(
        machine,
        module,
        filename,
        LLVMObjectFile,
        &error
    );
    if (error) {
        fprintf(stderr, "Error: %s\n", error);
        abort();
    }

    LLVMDisposeMessage(triple);
    LLVMDisposeMessage(cpu);
    LLVMDisposeMessage(features);
    LLVMDisposeTargetMachine(machine);
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

    LLVMValueRef if_else = build_if_else(symbols, builder);

    LLVMBuildRet(builder, build_variable_val("c", symbols, builder));

    LLVMVerifyModule(module, LLVMAbortProcessAction, NULL);
    char* out = LLVMPrintModuleToString(module);
    printf("%s\n", out);

    generate_obj_file("foo.o", module);

    LLVMDisposeModule(module);
    LLVMDisposeBuilder(builder);
    return 0;
}
