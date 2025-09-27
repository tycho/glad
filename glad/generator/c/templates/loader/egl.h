#ifdef GLAD_EGL

GLAD_API_CALL int gladLoaderLoadEGLContext({{ template_utils.context_arg(',') }} EGLDisplay display);
GLAD_API_CALL void gladLoaderUnloadEGLContext({{ template_utils.context_arg() }});
GLAD_API_CALL void gladLoaderResetEGLContext({{ template_utils.context_arg() }});

GLAD_API_CALL int gladLoaderLoadEGL(EGLDisplay display);
GLAD_API_CALL void gladLoaderUnloadEGL(void);
GLAD_API_CALL void gladLoaderResetEGL(void);

#endif
