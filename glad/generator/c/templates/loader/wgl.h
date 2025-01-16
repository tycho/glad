{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_WGL
GLAD_API_CALL int gladLoaderLoadWGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(', ') }}HDC hdc);
GLAD_API_CALL void gladLoaderUnloadWGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }});
GLAD_API_CALL void gladLoaderResetWGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }});
{% if options.mx_global %}
GLAD_API_CALL int gladLoaderLoadWGL(HDC hdc);
GLAD_API_CALL void gladLoaderUnloadWGL(void);
GLAD_API_CALL void gladLoaderResetWGL(void);
{% endif %}
#endif
