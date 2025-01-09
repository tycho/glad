/*
 * Full vulkan, header only, mx
 *
 * GLAD: $GLAD --out-path=$tmp --api="vulkan" c --loader --header-only --mx --mx-global
 * COMPILE: $GCC $test -o $tmp/test -Ithird_party/xxHash -I$tmp/include -ldl
 * RUN: $tmp/test
 */

#define GLAD_VULKAN_IMPLEMENTATION
#include <glad/vulkan.h>

typedef void (*VOID_FUNCPTR)(void);
VOID_FUNCPTR loader_userptr(void *userptr, const char *name) { (void) name; (void) userptr; return NULL; }
VOID_FUNCPTR loader(const char *name) { (void) name; return NULL; }

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
