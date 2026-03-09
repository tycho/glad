/*
 * Full GLES2
 *
 * GLAD: $GLAD --out-path=$tmp --api="gles2" c
 * COMPILE: $GCC $test -o $tmp/test -I$repo_root/third_party/xxHash -I$tmp/include $tmp/src/gles2.c -ldl
 * RUN: $tmp/test
 */

#include <glad/gles2.h>

int main(void) {
    return 0;
}
