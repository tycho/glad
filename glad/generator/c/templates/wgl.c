{% extends 'base_template.c' %}

{% block preimpl %}
#ifdef _WIN32
{% endblock %}

{% block hashsearch %}
{% endblock %}
{% block exthashes %}
{% endblock %}

{% block loader %}
static int glad_wgl_has_extension(const char *extensions, const char *ext) {
    const char *terminator;
    const char *loc;

    if(extensions == NULL || ext == NULL)
        return 0;

    while(1) {
        loc = strstr(extensions, ext);
        if(loc == NULL)
            break;

        terminator = loc + strlen(ext);
        if((loc == extensions || *(loc - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0'))
        {
            return 1;
        }
        extensions = terminator;
    }

    return 0;
}

static GLADapiproc glad_wgl_get_proc_from_userptr(void *userptr, const char* name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

{% for api in feature_set.info.apis %}
static int glad_wgl_find_extensions_{{ api|lower }}({{ template_utils.context_arg(', ') }}HDC hdc) {
{% if feature_set.extensions|length > 0 %}
{% if not feature_set.extensions|index_consecutive_0_to_N %}
    static const uint16_t extIdx[] = {
{% for extension in feature_set.extensions %}
        {{ "{:>4}".format(extension.index) }}, /* {{ extension.name }} */
{% endfor %}
    };
{% endif %}
    const char *extensions;
    uint32_t i;

    if({{ 'GLAD_wglGetExtensionsStringEXT'|ctx }} == NULL && {{ 'GLAD_wglGetExtensionsStringARB'|ctx }} == NULL)
        return 0;

    if({{ 'GLAD_wglGetExtensionsStringARB'|ctx }} == NULL || hdc == INVALID_HANDLE_VALUE)
        extensions = {{ 'GLAD_wglGetExtensionsStringEXT'|ctx }}();
    else
        extensions = {{ 'GLAD_wglGetExtensionsStringARB'|ctx }}(hdc);

    #pragma nounroll
{# If the list is a consecutive 0 to N list, we can just scan the whole thing without emitting an array. #}
{% if feature_set.extensions|index_consecutive_0_to_N %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_ext_names); ++i)
        context->extArray[i] = glad_wgl_has_extension(extensions, GLAD_{{ feature_set.name|api }}_ext_names[i]);
{% else %}
    for (i = 0; i < GLAD_ARRAYSIZE(extIdx); ++i)
        context->extArray[extIdx[i]] = glad_wgl_has_extension(extensions, GLAD_{{ feature_set.name|api }}_ext_names[extIdx[i]]);
{% endif %}

{% endif %}
    return 1;
}

static int glad_wgl_find_core_{{ api|lower }}({{ template_utils.context_arg(def='void') }}) {
    {% set hv = feature_set.features|select('supports', api)|list|last %}
    int major = {{ hv.version.major }}, minor = {{ hv.version.minor }};
{% for feature in feature_set.features|select('supports', api) %}
    {{ ('GLAD_' + feature.name)|ctx }} = (major == {{ feature.version.major }} && minor >= {{ feature.version.minor }}) || major > {{ feature.version.major }};
{% endfor %}
    return GLAD_MAKE_VERSION(major, minor);
}

GLAD_NO_INLINE int gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{ template_utils.context_arg(', ') }}HDC hdc, GLADuserptrloadfunc load, void *userptr) {
    int version;
    {{ 'GLAD_wglGetExtensionsStringARB'|ctx }} = (PFNWGLGETEXTENSIONSSTRINGARBPROC) load(userptr, "wglGetExtensionsStringARB");
    {{ 'GLAD_wglGetExtensionsStringEXT'|ctx }} = (PFNWGLGETEXTENSIONSSTRINGEXTPROC) load(userptr, "wglGetExtensionsStringEXT");
    if({{ 'GLAD_wglGetExtensionsStringARB'|ctx }} == NULL && {{ 'GLAD_wglGetExtensionsStringEXT'|ctx }} == NULL) return 0;
    version = glad_wgl_find_core_{{ api|lower }}({{'context' if options.mx }});

{% for feature, _ in loadable(feature_set.features, api=api) %}
    glad_wgl_load_{{ feature.name }}({{'context, ' if options.mx }}load, userptr);
{% endfor %}

    if (!glad_wgl_find_extensions_{{ api|lower }}({{'context, ' if options.mx }}hdc)) return 0;
{% for extension, _ in loadable(feature_set.extensions, api=api) %}
    glad_wgl_load_{{ extension.name }}({{'context, ' if options.mx }}load, userptr);
{% endfor %}

{% if options.alias %}
    glad_wgl_resolve_aliases({{'context' if options.mx }});
{% endif %}

    return version;
}

{% if options.mx_global %}
int gladLoad{{ api|api }}UserPtr(HDC hdc, GLADuserptrloadfunc load, void *userptr) {
    return gladLoad{{ api|api }}ContextUserPtr(gladGet{{ feature_set.name|api }}Context(), hdc, load, userptr);
}
{% endif %}

int gladLoad{{ api|api }}{{ 'Context' if options.mx }}({{ template_utils.context_arg(', ') }}HDC hdc, GLADloadfunc load) {
    return gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{'context,' if options.mx }} hdc, glad_wgl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}

{% if options.mx_global %}
int gladLoad{{ api|api }}(HDC hdc, GLADloadfunc load) {
    return gladLoad{{ api|api }}Context(gladGet{{ feature_set.name|api }}Context(), hdc, load);
}
{% endif %}

{% if options.mx_global %}
Glad{{ feature_set.name|api }}Context* gladGet{{ feature_set.name|api }}Context() {
    return &{{ global_context }};
}

void gladSet{{ feature_set.name|api }}Context(Glad{{ feature_set.name|api }}Context *context) {
    if (!context) return;
    if (&{{ global_context }} == context) return;
    {{ global_context }} = *context;
}
{% endif %}
{% endfor %}
{% endblock %}
{% block postimpl %}
#endif
{% endblock %}
