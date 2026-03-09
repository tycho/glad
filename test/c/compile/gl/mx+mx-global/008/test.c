/*
 * Full compatibility GL MX should not contain any symbols in COMMON section,
 * to prevent linking issues with OSX.
 *
 * Only tested here under the assumption that this build contains the most symbols.
 *
 * See: https://github.com/Dav1dde/glad/issues/158
 *
 * GLAD: $GLAD --out-path=$tmp --api="gl:compatibility" c --loader
 * COMPILE: $GCC -c -o $tmp/test.o -I$repo_root/third_party/xxHash -I$tmp/include $tmp/src/gl.c -ldl
 * RUN: [ "$(nm $tmp/test.o | grep ' C ' | wc -l)" -eq "0" ]
 */

#error "this testfile should not be compiled"
