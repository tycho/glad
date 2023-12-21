/*
 * Emscripten GLES2 + header only. See also 003.
 * Related Issues: #387
 *
 * GLAD: $GLAD --out-path=$tmp --api="egl,gles2" c --loader --header-only
 * COMPILE: $GCC $test -o $tmp/test -Ithird_party/xxHash -I$tmp/include -ldl -Wno-pedantic
 * RUN: $tmp/test
 */

#include <stddef.h>

#define GLAD_GLES2_IMPLEMENTATION
#define GLAD_PLATFORM_EMSCRIPTEN 1
#include <glad/gles2.h>
/*
 * egl after gles2
 * this is not correct, that's why we disable pedantic but it should still compile without
 */
#include <glad/egl.h>

__eglMustCastToProperFunctionPointerType emscripten_GetProcAddress(const char *name) {
    GLAD_UNUSED(name);
    return GLAD_GNUC_EXTENSION (__eglMustCastToProperFunctionPointerType) NULL;
}

int main(void) {
    return 0;
}
