{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_GL

{% set loader_handle = template_utils.handle('gl') %}
{% include 'loader/library.c' %}

typedef void* (GLAD_API_PTR *GLADglprocaddrfunc)(const char*);
struct _glad_gl_userptr {
    void *handle;
    GLADglprocaddrfunc gl_get_proc_address_ptr;
};

static GLADapiproc glad_gl_get_proc(void *vuserptr, const char *name) {
    struct _glad_gl_userptr userptr = *(struct _glad_gl_userptr*) vuserptr;
    GLADapiproc result = NULL;

    if(userptr.gl_get_proc_address_ptr != NULL) {
        result = GLAD_GNUC_EXTENSION (GLADapiproc) userptr.gl_get_proc_address_ptr(name);
    }
    if(result == NULL) {
        result = glad_dlsym_handle(userptr.handle, name);
    }

    return result;
}

{% if not options.mx %}
static void* {{ loader_handle }} = NULL;
{% endif %}

static void* glad_gl_dlopen_handle({{ template_utils.context_arg(def='void') }}) {
#if GLAD_PLATFORM_APPLE
    static const char *NAMES[] = {
        "../Frameworks/OpenGL.framework/OpenGL",
        "/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
    };
#elif GLAD_PLATFORM_WIN32
    static const char *NAMES[] = {"opengl32.dll"};
#else
    static const char *NAMES[] = {
  #if defined(__CYGWIN__)
        "libGL-1.so",
  #endif
        "libGL.so.1",
        "libGL.so"
    };
#endif

    if ({{ loader_handle }} == NULL) {
        {{ loader_handle }} = glad_get_dlopen_handle(NAMES, GLAD_ARRAYSIZE(NAMES));
    }

    return {{ loader_handle }};
}

static struct _glad_gl_userptr glad_gl_build_userptr(void *handle) {
    struct _glad_gl_userptr userptr;

    userptr.handle = handle;
#if GLAD_PLATFORM_APPLE || defined(__HAIKU__)
    userptr.gl_get_proc_address_ptr = NULL;
#elif GLAD_PLATFORM_WIN32
    userptr.gl_get_proc_address_ptr =
        (GLADglprocaddrfunc) glad_dlsym_handle(handle, "wglGetProcAddress");
#else
    userptr.gl_get_proc_address_ptr =
        (GLADglprocaddrfunc) glad_dlsym_handle(handle, "glXGetProcAddressARB");
#endif

    return userptr;
}

int gladLoaderLoadGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
    int version = 0;
    void *handle;
    int did_load = 0;
    struct _glad_gl_userptr userptr;

    did_load = {{ loader_handle }} == NULL;
    handle = glad_gl_dlopen_handle({{ 'context' if options.mx }});
    if (handle) {
        userptr = glad_gl_build_userptr(handle);

        version = gladLoadGL{{ 'Context' if options.mx }}UserPtr({{ 'context, ' if options.mx }}glad_gl_get_proc, &userptr);

        if (!version && did_load) {
            gladLoaderUnloadGL{{ 'Context' if options.mx }}({{ 'context' if options.mx }});
        }
    }

    return version;
}

void gladLoaderResetGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
{% if options.mx %}
    memset(context, 0, sizeof(GladGLContext));
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
void gladLoaderResetGL(void) {
    gladLoaderResetGLContext(gladGetGLContext());
}

int gladLoaderLoadGL(void) {
    return gladLoaderLoadGLContext(gladGet{{ feature_set.name|api }}Context());
}
{% endif %}

void gladLoaderUnloadGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
    if ({{ loader_handle }} != NULL) {
        glad_close_dlopen_handle({{ loader_handle }});
        {{ loader_handle }} = NULL;
    }

{% if not options.mx %}
    gladLoaderResetGL();
{% else %}
    gladLoaderResetGLContext(context);
{% endif %}
}

{%if options.mx_global %}
void gladLoaderUnloadGL(void) {
    gladLoaderUnloadGLContext(gladGet{{ feature_set.name|api }}Context());
}
{% endif %}

#endif /* GLAD_GL */
