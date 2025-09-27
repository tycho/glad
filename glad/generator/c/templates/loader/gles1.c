{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_GLES1

{% set loader_handle = template_utils.handle('gles1') %}
{% include 'loader/library.c' %}

#include <glad/egl.h>

struct _glad_gles1_userptr {
    void *handle;
    PFNEGLGETPROCADDRESSPROC get_proc_address_ptr;
};


static GLADapiproc glad_gles1_get_proc(void *vuserptr, const char* name) {
    struct _glad_gles1_userptr userptr = *(struct _glad_gles1_userptr*) vuserptr;
    GLADapiproc result = NULL;

    {# /* dlsym first, since some implementations don't return function pointers for core functions */ #}
    result = glad_dlsym_handle(userptr.handle, name);
    if (result == NULL) {
        result = userptr.get_proc_address_ptr(name);
    }

    return result;
}

static void* glad_gles1_dlopen_handle({{ template_utils.context_arg() }}) {
#if GLAD_PLATFORM_APPLE
    static const char *NAMES[] = {"libGLESv1_CM.dylib"};
#elif GLAD_PLATFORM_WIN32
    static const char *NAMES[] = {"GLESv1_CM.dll", "libGLESv1_CM", "libGLES_CM.dll"};
#else
    static const char *NAMES[] = {"libGLESv1_CM.so.1", "libGLESv1_CM.so", "libGLES_CM.so.1"};
#endif

    if ({{ loader_handle }} == NULL) {
        {{ loader_handle }} = glad_get_dlopen_handle(NAMES, GLAD_ARRAYSIZE(NAMES));
    }

    return {{ loader_handle }};
}

static struct _glad_gles1_userptr glad_gles1_build_userptr(void *handle) {
    struct _glad_gles1_userptr userptr;
    userptr.handle = handle;
    userptr.get_proc_address_ptr = eglGetProcAddress;
    return userptr;
}

int gladLoaderLoadGLES1Context({{ template_utils.context_arg() }}) {
    int version = 0;
    void *handle = NULL;
    int did_load = 0;
    struct _glad_gles1_userptr userptr;

    if (eglGetProcAddress == NULL) {
        return 0;
    }

    did_load = {{ loader_handle }} == NULL;
    handle = glad_gles1_dlopen_handle(context);
    if (handle != NULL) {
        userptr = glad_gles1_build_userptr(handle);

        version = gladLoadGLES1ContextUserPtr(context, glad_gles1_get_proc, &userptr);

        if (!version && did_load) {
            gladLoaderUnloadGLES1Context(context);
        }
    }

    return version;
}

int gladLoaderLoadGLES1(void) {
    return gladLoaderLoadGLES1Context(gladGet{{ feature_set.name|api }}Context());
}

void gladLoaderUnloadGLES1Context({{ template_utils.context_arg() }}) {
    if ({{ loader_handle }} != NULL) {
        glad_close_dlopen_handle({{ loader_handle }});
        {{ loader_handle }} = NULL;
    }
}

#endif /* GLAD_GLES1 */
