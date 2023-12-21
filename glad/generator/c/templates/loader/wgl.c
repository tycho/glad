#ifdef GLAD_WGL

static GLADapiproc glad_wgl_get_proc(void *vuserptr, const char* name) {
    GLAD_UNUSED(vuserptr);
    return GLAD_GNUC_EXTENSION (GLADapiproc) wglGetProcAddress(name);
}

int gladLoaderLoadWGL(HDC hdc) {
    return gladLoadWGLUserPtr(hdc, glad_wgl_get_proc, NULL);
}

{% if options.mx_global %}
void gladLoaderResetWGL(void) {
    gladLoaderResetWGLContext(gladGetWGLContext());
}
{% endif %}

void gladLoaderResetWGL{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
{% if options.mx %}
    memset(context, 0, sizeof(GladWGLContext));
{% else %}
{% for feature in feature_set.features %}
    GLAD_{{ feature.name }} = 0;
{% endfor %}

{% for extension in feature_set.extensions %}
    GLAD_{{ extension.name }} = 0;
{% endfor %}

{% for extension, commands in loadable() %}
{% for command in commands %}
    {{ command.name|ctx }} = NULL;
{% endfor %}
{% endfor %}
{% endif %}
}

#endif /* GLAD_WGL */
