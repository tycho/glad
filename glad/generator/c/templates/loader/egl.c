{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_EGL

{% include 'loader/library.c' %}

struct _glad_egl_userptr {
    void *handle;
    PFNEGLGETPROCADDRESSPROC get_proc_address_ptr;
};

static GLADapiproc glad_egl_get_proc(void *vuserptr, const char* name) {
    struct _glad_egl_userptr userptr = *(struct _glad_egl_userptr*) vuserptr;
    GLADapiproc result = NULL;

    result = glad_dlsym_handle(userptr.handle, name);
    if (result == NULL) {
        result = GLAD_GNUC_EXTENSION (GLADapiproc) userptr.get_proc_address_ptr(name);
    }

    return result;
}

{% if not options.mx %}
static void* {{ template_utils.handle() }} = NULL;
{% endif %}

static void* glad_egl_dlopen_handle({{ template_utils.context_arg(def='void') }}) {
#if GLAD_PLATFORM_APPLE
    static const char *NAMES[] = {"libEGL.dylib"};
#elif GLAD_PLATFORM_WIN32
    static const char *NAMES[] = {"libEGL.dll", "EGL.dll"};
#else
    static const char *NAMES[] = {"libEGL.so.1", "libEGL.so"};
#endif

    if ({{ template_utils.handle() }} == NULL) {
        {{ template_utils.handle() }} = glad_get_dlopen_handle(NAMES, sizeof(NAMES) / sizeof(NAMES[0]));
    }

    return {{ template_utils.handle() }};
}

static struct _glad_egl_userptr glad_egl_build_userptr(void *handle) {
    struct _glad_egl_userptr userptr;
    userptr.handle = handle;
    userptr.get_proc_address_ptr = (PFNEGLGETPROCADDRESSPROC) glad_dlsym_handle(handle, "eglGetProcAddress");
    return userptr;
}

{% if not options.on_demand %}
int gladLoaderLoadEGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(',') }} EGLDisplay display) {
    int version = 0;
    void *handle;
    int did_load = 0;
    struct _glad_egl_userptr userptr;

    did_load = {{ template_utils.handle() }} == NULL;
    handle = glad_egl_dlopen_handle({{ 'context' if options.mx }});
    if (handle) {
        userptr = glad_egl_build_userptr(handle);

        version = gladLoadEGL{{ 'Context' if options.mx }}UserPtr({{ 'context, ' if options.mx }}display, glad_egl_get_proc, &userptr);

        if (!version && did_load) {
            gladLoaderUnloadEGL{{ 'Context' if options.mx }}({{ 'context' if options.mx }});
        }
    }

    return version;
}
{% endif %}

{% if options.mx_global %}
int gladLoaderLoadEGL(EGLDisplay display) {
    return gladLoaderLoadEGLContext(gladGet{{ feature_set.name|api }}Context(), display);
}
{% endif %}

{% if options.on_demand %}
{% call template_utils.zero_initialized() %}static struct _glad_egl_userptr glad_egl_internal_loader_global_userptr{% endcall %}
static GLADapiproc glad_egl_internal_loader_get_proc(const char *name) {
    if (glad_egl_internal_loader_global_userptr.handle == NULL) {
        glad_egl_internal_loader_global_userptr = glad_egl_build_userptr(glad_egl_dlopen_handle());
    }

    return glad_egl_get_proc((void *) &glad_egl_internal_loader_global_userptr, name);
}
{% endif %}

{% if options.mx_global %}
void gladLoaderResetEGL(void) {
    gladLoaderResetEGLContext(gladGetEGLContext());
}
{% endif %}

void gladLoaderUnloadEGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
    if ({{ template_utils.handle() }} != NULL) {
        glad_close_dlopen_handle({{ template_utils.handle() }});
        {{ template_utils.handle() }} = NULL;
{% if options.on_demand %}
        glad_egl_internal_loader_global_userptr.handle = NULL;
{% endif %}
    }

{% if not options.mx %}
    gladLoaderResetEGL();
{% else %}
    gladLoaderResetEGLContext(context);
{% endif %}
}

{%if options.mx_global %}
void gladLoaderUnloadEGL(void) {
    gladLoaderUnloadEGLContext(gladGet{{ feature_set.name|api }}Context());
}
{% endif %}

void gladLoaderResetEGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
{% if options.mx %}
    memset(context, 0, sizeof(GladEGLContext));
{% else %}
{% if not options.on_demand %}
{% for feature in feature_set.features %}
    GLAD_{{ feature.name }} = 0;
{% endfor %}

{% for extension in feature_set.extensions %}
    GLAD_{{ extension.name }} = 0;
{% endfor %}
{% endif %}

{% for extension, commands in loadable() %}
{% for command in commands %}
    {{ command.name|ctx }} = NULL;
{% endfor %}
{% endfor %}
{% endif %}
}

#endif /* GLAD_EGL */
