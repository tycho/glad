/*
 * Full vulkan, mx
 *
 * GLAD: $GLAD --out-path=$tmp --api="vulkan" c --loader
 * COMPILE: $GCC $test -o $tmp/test -Ithird_party/xxHash -I$tmp/include $tmp/src/vulkan.c -ldl
 * RUN: $tmp/test
 */

#include <glad/vulkan.h>

typedef void (*VOID_FUNCPTR)(void);
VOID_FUNCPTR loader_userptr(void *userptr, const char *name, enum GLADcommandscope scope) { (void) name; (void) userptr; (void) scope; return NULL; }
VOID_FUNCPTR loader(const char *name, enum GLADcommandscope scope) { (void) name; (void) scope; return NULL; }

int main(void) {
    GladVulkanContext context = {0};
    (void) gladLoadVulkanUserPtr(NULL, NULL, NULL, loader_userptr, NULL);
    (void) gladLoadVulkanContextUserPtr(&context, NULL, NULL, NULL, loader_userptr, NULL);
    (void) gladLoadVulkan(NULL, NULL, NULL, loader);
    (void) gladLoadVulkanContext(&context, NULL, NULL, NULL, loader);
    (void) gladLoaderLoadVulkan(NULL, NULL, NULL);
    (void) gladLoaderLoadVulkanContext(&context, NULL, NULL, NULL);
    return 0;
}

