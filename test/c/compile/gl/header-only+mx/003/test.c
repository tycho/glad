/*
 * No extensions compatibility GL MX header only
 *
 * GLAD: $GLAD --out-path=$tmp --api="gl:compatibility" --extensions="" c --loader --mx --mx-global --header-only
 * COMPILE: $GCC $test -o $tmp/test -Ithird_party/xxHash -I$tmp/include -ldl
 * RUN: $tmp/test
 */

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

int main(void) {
    GladGLContext gl = {0};
    (void) gladLoaderLoadGL();
    (void) gladLoaderLoadGLContext(&gl);
    return 0;
}
