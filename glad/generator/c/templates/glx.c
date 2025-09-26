{% extends 'base_template.c' %}

{% block preimpl %}
#ifdef __linux__
{% endblock %}

{% block extnames %}
{% endblock %}

{% block loader %}
static GLADapiproc glad_glx_get_proc_from_userptr(void *userptr, const char* name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

static int glad_glx_get_extensions({{ template_utils.context_arg(', ') }}Display *display, int screen, uint64_t **out_exts, uint32_t *out_num_exts) {
#ifdef GLX_VERSION_1_1
    uint32_t num_exts = 0;
    uint64_t *exts = NULL;
    const char *exts_str = NULL;
    const char *cur = NULL;
    const char *next = NULL;
    uint32_t len = 0, j = 0;

    if ({{'GLAD_glXQueryExtensionsString'|ctx}} == NULL) {
        return -2;
    }

    exts_str = {{'GLAD_glXQueryExtensionsString'|ctx}}(display, screen);

    /* This is done in two passes. The first pass counts up the number of
     * extensions. The second pass hashes their names and stores them in
     * a heap-allocated uint64 array for searching.
     */
    for (j = 0; j < 2; ++j) {
        num_exts = 0;
        cur = exts_str;
        next = cur + strcspn(cur, " ");
        while (1) {
            cur += strspn(cur, " ");

            if (!cur[0])
                break;

            len = next - cur;

            if (exts != NULL) {
                exts[num_exts++] = glad_hash_string(cur, len);
            } else {
                num_exts++;
            }

            cur = next + strspn(next, " ");
            next = cur + strcspn(cur, " ");
        }

        if (!exts)
            exts = (uint64_t *)calloc(num_exts, sizeof(uint64_t));
    }

{% if search_type == 0 %}
    /* Sort extension list for binary search */
    qsort(exts, num_exts, sizeof(uint64_t), compare_uint64);

{% endif %}
    *out_num_exts = num_exts;
    *out_exts = exts;
#else
    GLAD_UNUSED(context);
    GLAD_UNUSED(display);
    GLAD_UNUSED(screen);
    GLAD_UNUSED(glad_hash_string);
    *out_num_exts = 0;
    *out_exts = NULL;
#endif
    return 1;
}

static void glad_glx_free_extensions(uint64_t *exts) {
    free(exts);
}

static int glad_glx_has_extension(uint64_t *exts, uint32_t num_exts, uint64_t ext) {
#ifdef GLX_VERSION_1_1
    return glad_hash_search(exts, num_exts, ext);
#else
    GLAD_UNUSED(exts);
    GLAD_UNUSED(num_exts);
    GLAD_UNUSED(ext);
    GLAD_UNUSED(compare_uint64);
    GLAD_UNUSED(glad_hash_search);

    /* We can't detect if an extension is supported without glXQueryExtensionsString */
    return 0;
#endif
}

{% for api in feature_set.info.apis %}
static int glad_glx_find_extensions({{ template_utils.context_arg(', ') }}Display *display, int screen) {
{% if feature_set.extensions|select('supports', api)|count > 0  %}
{% if not (feature_set.extensions|select('supports', api))|index_consecutive_0_to_N %}
    static const uint16_t s_extIdx[] = {
{% for extension in feature_set.extensions|select('supports', api) %}
        {{ "{:>4}".format(extension.index) }}, /* {{ extension.name }} */
{% endfor %}
    };
{% endif %}
    uint64_t *exts = NULL;
    uint32_t num_exts = 0;
    uint32_t i;
    if (!glad_glx_get_extensions({{ 'context, ' if options.mx }}display, screen, &exts, &num_exts)) return 0;

{# If the list is a consecutive 0 to N list, we can just scan the whole thing without emitting an array. #}
{% if (feature_set.extensions|select('supports', api))|index_consecutive_0_to_N %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_ext_hashes); ++i)
        context->extArray[i] = glad_glx_has_extension(exts, num_exts, GLAD_{{ feature_set.name|api }}_ext_hashes[i]);
{% else %}
    for (i = 0; i < GLAD_ARRAYSIZE(s_extIdx); ++i) {
        const uint32_t extIdx = s_extIdx[i];
        context->extArray[extIdx] = glad_glx_has_extension(exts, num_exts, GLAD_{{ feature_set.name|api }}_ext_hashes[extIdx]);
    }
{% endif %}

    glad_glx_free_extensions(exts);

{% else %}
{%if options.mx %}
    GLAD_UNUSED(context);
{% endif %}
    GLAD_UNUSED(glad_glx_get_extensions);
    GLAD_UNUSED(glad_glx_has_extension);
    GLAD_UNUSED(glad_glx_free_extensions);
    GLAD_UNUSED(GLAD_{{ feature_set.name|api }}_ext_hashes);
{% endif %}
    return 1;
}

static int glad_glx_find_core_{{ api|lower }}({{ template_utils.context_arg(', ') }}Display **display, int *screen) {
    int major = 0, minor = 0;
    unsigned short version_value;
    if(*display == NULL) {
#ifdef GLAD_GLX_NO_X11
        GLAD_UNUSED(screen);
        return 0;
#else
        *display = XOpenDisplay(0);
        if (*display == NULL) {
            return 0;
        }
        *screen = XScreenNumberOfScreen(XDefaultScreenOfDisplay(*display));
#endif
    }
    {{'GLAD_glXQueryVersion'|ctx}}(*display, &major, &minor);
    version_value = (major << 8U) | minor;
{% for feature in feature_set.features %}
    {{ ('GLAD_' + feature.name)|ctx(name_only=True) }} = version_value >= 0x{{ '%02x%02x'|format(feature.version.major, feature.version.minor) }};
{% endfor %}
    return GLAD_MAKE_VERSION(major, minor);
}

GLAD_NO_INLINE int gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{ template_utils.context_arg(', ') }}Display *display, int screen, GLADuserptrloadfunc load, void *userptr) {
    int version;
{% if options.use_pfn_ranges %}
    uint32_t i;
{% endif %}

    {{ 'GLAD_glXQueryVersion'|ctx }} = (PFNGLXQUERYVERSIONPROC) load(userptr, "glXQueryVersion");
    if({{'GLAD_glXQueryVersion'|ctx }} == NULL) return 0;
    version = glad_glx_find_core_{{ api|lower }}({{'context, ' if options.mx }}&display, &screen);

{% if options.use_pfn_ranges %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_feature_pfn_ranges); ++i) {
        const GladPfnRange_t *range = &GLAD_{{ feature_set.name|api }}_feature_pfn_ranges[i];
        if (context->featArray[range->extension]) {
            glad_{{ spec.name }}_load_pfn_range({{'context, ' if options.mx }}load, userptr, range->start, range->count);
        }
    }
{% else %}
{% for feature, _ in loadable(feature_set.features) %}
    glad_glx_load_{{ feature.name }}({{'context, ' if options.mx }}load, userptr);
{% endfor %}
{% endif %}

    if (!glad_glx_find_extensions({{'context, ' if options.mx }}display, screen)) return 0;

{% if options.use_pfn_ranges %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_ext_pfn_ranges); ++i) {
        const GladPfnRange_t *range = &GLAD_{{ feature_set.name|api }}_ext_pfn_ranges[i];
        if (context->extArray[range->extension]) {
            glad_{{ spec.name }}_load_pfn_range({{'context, ' if options.mx }}load, userptr, range->start, range->count);
        }
    }
{% else %}
{% for extension, _ in loadable(feature_set.extensions) %}
    glad_glx_load_{{ extension.name }}({{'context, ' if options.mx }}load, userptr);
{% endfor %}
{% endif%}

{% if options.mx_global %}
    gladSet{{ feature_set.name|api }}Context(context);
{% endif %}

{% if options.alias %}
    glad_glx_resolve_aliases({{'context' if options.mx }});
{% endif %}

    return version;
}

{% if options.mx_global %}
int gladLoad{{ api|api }}UserPtr(Display *display, int screen, GLADuserptrloadfunc load, void *userptr) {
    return gladLoad{{ api|api }}ContextUserPtr(gladGet{{ feature_set.name|api }}Context(), display, screen, load, userptr);
}
{% endif %}

int gladLoad{{ api|api }}{{ 'Context' if options.mx }}({{ template_utils.context_arg(', ') }}Display *display, int screen, GLADloadfunc load) {
    return gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{'context,' if options.mx }} display, screen, glad_glx_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}

{% if options.mx_global %}
int gladLoad{{ api|api }}(Display *display, int screen, GLADloadfunc load) {
    return gladLoad{{ api|api }}Context(gladGet{{ feature_set.name|api }}Context(), display, screen, GLAD_GNUC_EXTENSION (void*) load);
}
{% endif %}
{% endfor %}

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

{% endblock %}

{% block postimpl %}
#endif
{% endblock %}
