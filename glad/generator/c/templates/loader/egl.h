#ifdef GLAD_EGL

GLAD_API_CALL int gladLoaderLoadEGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(',') }} EGLDisplay display);
GLAD_API_CALL void gladLoaderUnloadEGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }});
GLAD_API_CALL void gladLoaderResetEGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }});
{% if options.mx_global %}
GLAD_API_CALL int gladLoaderLoadEGL(EGLDisplay display);
GLAD_API_CALL void gladLoaderUnloadEGL(void);
GLAD_API_CALL void gladLoaderResetEGL(void);
{% endif %}

#endif
