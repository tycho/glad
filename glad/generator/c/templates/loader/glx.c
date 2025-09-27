#ifdef GLAD_GLX

{% include 'loader/library.c' %}

typedef void* (GLAD_API_PTR *GLADglxprocaddrfunc)(const char*);
struct _glad_glx_userptr {
    void *handle;
    GLADglxprocaddrfunc glx_get_proc_address_ptr;
};

static GLADapiproc glad_glx_get_proc(void *vuserptr, const char *name) {
    struct _glad_glx_userptr userptr = *(struct _glad_glx_userptr*) vuserptr;
    GLADapiproc result = NULL;

    if(userptr.glx_get_proc_address_ptr != NULL) {
        result = GLAD_GNUC_EXTENSION (GLADapiproc) userptr.glx_get_proc_address_ptr(name);
    }
    if(result == NULL) {
        result = glad_dlsym_handle(userptr.handle, name);
    }

    return result;
}

static void* glad_glx_dlopen_handle({{ template_utils.context_arg() }}) {
    static const char *NAMES[] = {
#if defined __CYGWIN__
        "libGL-1.so",
#endif
        "libGL.so.1",
        "libGL.so"
    };

    if ({{ template_utils.handle() }} == NULL) {
        {{ template_utils.handle() }} = glad_get_dlopen_handle(NAMES, GLAD_ARRAYSIZE(NAMES));
    }

    return {{ template_utils.handle() }};
}

static struct _glad_glx_userptr glad_glx_build_userptr(void *handle) {
    struct _glad_glx_userptr userptr;

    userptr.handle = handle;
    userptr.glx_get_proc_address_ptr =
        (GLADglxprocaddrfunc) glad_dlsym_handle(handle, "glXGetProcAddressARB");

    return userptr;
}

int gladLoaderLoadGLXContext({{ template_utils.context_arg(', ') }}Display *display, int screen) {
    int version = 0;
    void *handle;
    int did_load = 0;
    struct _glad_glx_userptr userptr;

    did_load = {{ template_utils.handle() }} == NULL;
    handle = glad_glx_dlopen_handle(context);
    if (handle) {
        userptr = glad_glx_build_userptr(handle);

        version = gladLoadGLXContextUserPtr(context, display, screen, glad_glx_get_proc, &userptr);

        if (!version && did_load) {
            gladLoaderUnloadGLXContext(context);
        }
    }
    return version;
}

int gladLoaderLoadGLX(Display *display, int screen) {
    return gladLoaderLoadGLXContext(gladGet{{ feature_set.name|api }}Context(), display, screen);
}

void gladLoaderUnloadGLXContext({{ template_utils.context_arg() }}) {
    if ({{ template_utils.handle() }} != NULL) {
        glad_close_dlopen_handle({{ template_utils.handle() }});
        {{ template_utils.handle() }} = NULL;
    }

    gladLoaderResetGLXContext(context);
}

void gladLoaderUnloadGLX() {
    gladLoaderUnloadGLXContext(gladGet{{ feature_set.name|api }}Context());
}

void gladLoaderResetGLX(void) {
    gladLoaderResetGLXContext(gladGetGLXContext());
}

void gladLoaderResetGLXContext({{ template_utils.context_arg() }}) {
    memset(context, 0, sizeof(GladGLXContext));
}

#endif /* GLAD_GLX */
