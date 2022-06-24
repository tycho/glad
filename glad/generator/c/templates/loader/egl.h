#ifdef GLAD_EGL

{% if not options.on_demand %}
GLAD_API_CALL int gladLoaderLoadEGL(EGLDisplay display);
{% endif %}

GLAD_API_CALL void gladLoaderUnloadEGL(void);
GLAD_API_CALL void gladLoaderResetEGL(void);

#endif
