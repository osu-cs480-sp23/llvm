#include <stdio.h>
#include <stdlib.h>

#include <llvm-c/Core.h>

#include "hash.h"

int main() {
    LLVMModuleRef module = LLVMModuleCreateWithName("foo.code");

    LLVMTypeRef foo_type = LLVMFunctionType(
        LLVMVoidType(), NULL, 0, 0
    );
    LLVMValueRef foo_fn = LLVMAddFunction(module, "foo", foo_type);

    LLVMDisposeModule(module);
    return 0;
}
