{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_GLES2

{% set loader_handle = template_utils.handle('gles2') %}
{% include 'loader/library.c' %}

#if GLAD_PLATFORM_EMSCRIPTEN
#ifndef GLAD_EGL_H_
  typedef void (*__eglMustCastToProperFunctionPointerType)(void);
  typedef __eglMustCastToProperFunctionPointerType (GLAD_API_PTR *PFNEGLGETPROCADDRESSPROC)(const char *name);
#endif
  extern __eglMustCastToProperFunctionPointerType emscripten_GetProcAddress(const char *name);
#elif defined(GLAD_GLES2_USE_SYSTEM_EGL)
  #include <EGL/egl.h>
  typedef __eglMustCastToProperFunctionPointerType (GLAD_API_PTR *PFNEGLGETPROCADDRESSPROC)(const char *name);
#else
  #include <glad/egl.h>
#endif


struct _glad_gles2_userptr {
    void *handle;
    PFNEGLGETPROCADDRESSPROC get_proc_address_ptr;
};


static GLADapiproc glad_gles2_get_proc(void *vuserptr, const char* name) {
    struct _glad_gles2_userptr userptr = *(struct _glad_gles2_userptr*) vuserptr;
    GLADapiproc result = NULL;

#if GLAD_PLATFORM_EMSCRIPTEN
    GLAD_UNUSED(glad_dlsym_handle);
#else
    {# /* dlsym first, since some implementations don't return function pointers for core functions */ #}
    result = glad_dlsym_handle(userptr.handle, name);
#endif
    if (result == NULL) {
        result = userptr.get_proc_address_ptr(name);
    }

    return result;
}

{% if not options.mx %}
static void* {{ loader_handle }} = NULL;
{% endif %}

static void* glad_gles2_dlopen_handle({{ template_utils.context_arg(def='void') }}) {
#if GLAD_PLATFORM_EMSCRIPTEN
#elif GLAD_PLATFORM_APPLE
    static const char *NAMES[] = {"libGLESv2.dylib"};
#elif GLAD_PLATFORM_WIN32
    static const char *NAMES[] = {"GLESv2.dll", "libGLESv2.dll"};
#else
    static const char *NAMES[] = {"libGLESv2.so.2", "libGLESv2.so"};
#endif

#if GLAD_PLATFORM_EMSCRIPTEN
{% if options.mx %}
    GLAD_UNUSED(context);
{% endif %}
    GLAD_UNUSED(glad_get_dlopen_handle);
    return NULL;
#else
    if ({{ loader_handle }} == NULL) {
        {{ loader_handle }} = glad_get_dlopen_handle(NAMES, GLAD_ARRAYSIZE(NAMES));
    }

    return {{ loader_handle }};
#endif
}

static struct _glad_gles2_userptr glad_gles2_build_userptr(void *handle) {
    struct _glad_gles2_userptr userptr;
#if GLAD_PLATFORM_EMSCRIPTEN
    GLAD_UNUSED(handle);
    userptr.get_proc_address_ptr = emscripten_GetProcAddress;
#else
    userptr.handle = handle;
    userptr.get_proc_address_ptr = eglGetProcAddress;
#endif
    return userptr;
}

int gladLoaderLoadGLES2{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
    int version = 0;
    void *handle = NULL;
    int did_load = 0;
    struct _glad_gles2_userptr userptr;

#if GLAD_PLATFORM_EMSCRIPTEN
    GLAD_UNUSED(handle);
    GLAD_UNUSED(did_load);
    GLAD_UNUSED(glad_gles2_dlopen_handle);
    GLAD_UNUSED(glad_gles2_build_userptr);
    userptr.get_proc_address_ptr = emscripten_GetProcAddress;
    version = gladLoadGLES2{{ 'Context' if options.mx }}UserPtr({{ 'context, ' if options.mx }}glad_gles2_get_proc, &userptr);
#else
#ifndef GLAD_GLES2_USE_SYSTEM_EGL
    if (eglGetProcAddress == NULL) {
        return 0;
    }
#endif
    did_load = {{ loader_handle }} == NULL;
    handle = glad_gles2_dlopen_handle({{ 'context' if options.mx }});
    if (handle != NULL) {
        userptr = glad_gles2_build_userptr(handle);

        version = gladLoadGLES2{{ 'Context' if options.mx }}UserPtr({{ 'context, ' if options.mx }}glad_gles2_get_proc, &userptr);

        if (!version && did_load) {
            gladLoaderUnloadGLES2{{ 'Context' if options.mx }}({{ 'context' if options.mx }});
        }
    }
#endif

    return version;
}

void gladLoaderResetGLES2{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
{% if options.mx %}
    memset(context, 0, sizeof(Glad{{ feature_set.name|api }}Context));
{% else %}
{% for feature in feature_set.features %}
    {{ ('GLAD_' + feature.name)|ctx(name_only=True) }} = 0;
{% endfor %}

{% for extension in feature_set.extensions %}
    {{ ('GLAD_' + extension.name)|ctx(name_only=True) }} = 0;
{% endfor %}

{% for extension, commands in loadable() %}
{% for command in commands %}
    {{ command.name|ctx }} = NULL;
{% endfor %}
{% endfor %}
{% endif %}
}

{% if options.mx_global %}
void gladLoaderResetGLES2(void) {
    gladLoaderResetGLES2Context(gladGet{{ feature_set.name|api }}Context());
}
{% endif %}

{% if options.mx_global %}
int gladLoaderLoadGLES2(void) {
    return gladLoaderLoadGLES2Context(gladGet{{ feature_set.name|api }}Context());
}
{% endif %}

void gladLoaderUnloadGLES2{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
    if ({{ loader_handle }} != NULL) {
        glad_close_dlopen_handle({{ loader_handle }});
        {{ loader_handle }} = NULL;
    }

{% if not options.mx %}
    gladLoaderResetGLES2();
{% else %}
    gladLoaderResetGLES2Context(context);
{% endif %}
}

{%if options.mx_global %}
void gladLoaderUnloadGLES2(void) {
    gladLoaderUnloadGLES2Context(gladGet{{ feature_set.name|api }}Context());
}
{% endif %}

#endif /* GLAD_GLES2 */
