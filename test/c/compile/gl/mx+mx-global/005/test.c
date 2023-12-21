/*
 * MX GL 2.1 All extensions
 *
 * GLAD: $GLAD --out-path=$tmp --api="gl:compatibility=2.1" c --loader --mx --mx-global
 * COMPILE: $GCC $test -o $tmp/test -Ithird_party/xxHash -I$tmp/include $tmp/src/gl.c -ldl
 * RUN: $tmp/test
 */

#include <glad/gl.h>

int main(void) {
    GladGLContext gl = {0};
    (void) gladLoaderLoadGL();
    (void) gladLoaderLoadGLContext(&gl);
    return 0;
}
