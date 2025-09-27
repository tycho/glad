#ifdef GLAD_WGL

{% include 'loader/library.c' %}

typedef void* (GLAD_API_PTR *GLADwglprocaddrfunc)(const char*);
struct _glad_wgl_userptr {
    void *handle;
    GLADwglprocaddrfunc wgl_get_proc_address_ptr;
};

static void* glad_wgl_dlopen_handle({{ template_utils.context_arg() }}) {
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

    if ({{ template_utils.handle() }} == NULL) {
        {{ template_utils.handle() }} = glad_get_dlopen_handle(NAMES, GLAD_ARRAYSIZE(NAMES));
    }

    return {{ template_utils.handle() }};
}

static struct _glad_wgl_userptr glad_wgl_build_userptr(void *handle) {
    struct _glad_wgl_userptr userptr;

    userptr.handle = handle;
#if GLAD_PLATFORM_APPLE || defined(__HAIKU__)
    userptr.wgl_get_proc_address_ptr = NULL;
#elif GLAD_PLATFORM_WIN32
    userptr.wgl_get_proc_address_ptr =
        (GLADwglprocaddrfunc) glad_dlsym_handle(handle, "wglGetProcAddress");
#else
    userptr.wgl_get_proc_address_ptr =
        (GLADwglprocaddrfunc) glad_dlsym_handle(handle, "glXGetProcAddressARB");
#endif

    return userptr;
}

static GLADapiproc glad_wgl_get_proc(void *vuserptr, const char *name) {
    struct _glad_wgl_userptr userptr = *(struct _glad_wgl_userptr*) vuserptr;
    GLADapiproc result = NULL;

    if(userptr.wgl_get_proc_address_ptr != NULL) {
        result = GLAD_GNUC_EXTENSION (GLADapiproc) userptr.wgl_get_proc_address_ptr(name);
    }
    if(result == NULL) {
        result = glad_dlsym_handle(userptr.handle, name);
    }

    return result;
}

int gladLoaderLoadWGLContext({{ template_utils.context_arg(', ') }}HDC hdc) {
    int version = 0;
    void *handle;
    int did_load = 0;
    struct _glad_wgl_userptr userptr;

    did_load = {{ template_utils.handle() }} == NULL;
    handle = glad_wgl_dlopen_handle(context);
    if (handle) {
        userptr = glad_wgl_build_userptr(handle);

        version = gladLoadWGLContextUserPtr(context, hdc, glad_wgl_get_proc, &userptr);

        if (!version && did_load) {
            gladLoaderUnloadWGLContext(context);
        }
    }
    return version;
}

int gladLoaderLoadWGL(HDC hdc) {
    return gladLoaderLoadWGLContext(gladGet{{ feature_set.name|api }}Context(), hdc);
}

void gladLoaderResetWGL(void) {
    gladLoaderResetWGLContext(gladGetWGLContext());
}

void gladLoaderUnloadWGLContext({{ template_utils.context_arg() }}) {
    if ({{ template_utils.handle() }} != NULL) {
        glad_close_dlopen_handle({{ template_utils.handle() }});
        {{ template_utils.handle() }} = NULL;
    }

    gladLoaderResetWGLContext(context);
}

void gladLoaderResetWGLContext({{ template_utils.context_arg() }}) {
    memset(context, 0, sizeof(GladWGLContext));
}

void gladLoaderUnloadWGL(void) {
    gladLoaderUnloadWGLContext(gladGet{{ feature_set.name|api }}Context());
}

#endif /* GLAD_WGL */
