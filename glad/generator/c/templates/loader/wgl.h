{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_WGL

GLAD_API_CALL int gladLoaderLoadWGLContext({{ template_utils.context_arg(', ') }}HDC hdc);
GLAD_API_CALL void gladLoaderUnloadWGLContext({{ template_utils.context_arg() }});
GLAD_API_CALL void gladLoaderResetWGLContext({{ template_utils.context_arg() }});

GLAD_API_CALL int gladLoaderLoadWGL(HDC hdc);
GLAD_API_CALL void gladLoaderUnloadWGL(void);
GLAD_API_CALL void gladLoaderResetWGL(void);

#endif
