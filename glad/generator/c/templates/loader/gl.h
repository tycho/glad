{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_GL

GLAD_API_CALL int gladLoaderLoadGLContext({{ template_utils.context_arg() }});
GLAD_API_CALL void gladLoaderUnloadGLContext({{ template_utils.context_arg() }});
GLAD_API_CALL void gladLoaderResetGLContext({{ template_utils.context_arg() }});

GLAD_API_CALL int gladLoaderLoadGL(void);
GLAD_API_CALL void gladLoaderUnloadGL(void);
GLAD_API_CALL void gladLoaderResetGL(void);

#endif
