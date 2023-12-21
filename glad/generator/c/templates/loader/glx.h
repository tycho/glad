#ifdef GLAD_GLX

GLAD_API_CALL int gladLoaderLoadGLX(Display *display, int screen);
GLAD_API_CALL void gladLoaderUnloadGLX(void);
GLAD_API_CALL void gladLoaderResetGLX{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }});
{% if not options.mx_global %}
GLAD_API_CALL void gladLoaderResetGLX(void);
{% endif %}

#endif