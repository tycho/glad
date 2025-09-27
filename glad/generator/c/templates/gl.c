{% extends 'base_template.c' %}

{% block extnames %}
{% endblock %}
{% block loader %}
{% if feature_set.extensions|length > 0 %}
static int glad_gl_get_extensions({{ template_utils.context_arg(', ') }}uint64_t **out_exts, uint32_t *out_num_exts) {
    uint32_t num_exts = 0;
    uint64_t *exts = NULL;
    const char *exts_str = NULL;
    const char *cur = NULL;
    const char *next = NULL;
    uint32_t len = 0, j = 0;

#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_3_0)
    if ({{ 'glGetStringi'|ctx }} != NULL && {{ 'glGetIntegerv'|ctx }} != NULL) {
        unsigned int index = 0;
        {{ 'glGetIntegerv'|ctx }}(GL_NUM_EXTENSIONS, (int*) &num_exts);
        if (num_exts > 0) {
            exts = (uint64_t *)calloc(num_exts, sizeof(uint64_t));
        }
        if (exts == NULL) {
            return 0;
        }
        for(index = 0; index < num_exts; index++) {
            const char *gl_str_tmp = (const char*) {{ 'glGetStringi'|ctx }}(GL_EXTENSIONS, index);
            size_t len = strlen(gl_str_tmp);
            exts[index] = glad_hash_string(gl_str_tmp, len);
        }
    } else
#endif
    {
        if ({{ 'glGetString'|ctx }} == NULL) {
            return 0;
        }
        exts_str = (const char *){{ 'glGetString'|ctx }}(GL_EXTENSIONS);
        if (exts_str == NULL) {
            return 0;
        }

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
    }

{% if search_type == 0 %}
    /* Sort extension list for binary search */
    qsort(exts, num_exts, sizeof(uint64_t), compare_uint64);

{% endif %}
    *out_num_exts = num_exts;
    *out_exts = exts;

    return 1;
}

static void glad_gl_free_extensions(uint64_t *exts) {
    free(exts);
}

static int glad_gl_has_extension(uint64_t *exts, uint32_t num_exts, uint64_t ext) {
    return glad_hash_search(exts, num_exts, ext);
}

{% endif %}
static GLADapiproc glad_gl_get_proc_from_userptr(void *userptr, const char* name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

{% for api in feature_set.info.apis %}
{% if feature_set.extensions|length > 0 %}
static int glad_gl_find_extensions_{{ api|lower }}({{ template_utils.context_arg() }}) {
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
    if (!glad_gl_get_extensions(context, &exts, &num_exts)) return 0;

{# If the list is a consecutive 0 to N list, we can just scan the whole thing without emitting an array. #}
{% if (feature_set.extensions|select('supports', api))|index_consecutive_0_to_N %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_ext_hashes); ++i)
        context->extArray[i] = glad_gl_has_extension(exts, num_exts, GLAD_{{ feature_set.name|api }}_ext_hashes[i]);
{% else %}
    for (i = 0; i < GLAD_ARRAYSIZE(s_extIdx); ++i) {
        const uint32_t extIdx = s_extIdx[i];
        context->extArray[extIdx] = glad_gl_has_extension(exts, num_exts, GLAD_{{ feature_set.name|api }}_ext_hashes[extIdx]);
    }
{% endif %}

    glad_gl_free_extensions(exts);

{% else %}
    GLAD_UNUSED(context);
    GLAD_UNUSED(glad_gl_get_extensions);
    GLAD_UNUSED(glad_gl_has_extension);
    GLAD_UNUSED(glad_gl_free_extensions);
    GLAD_UNUSED(GLAD_{{ feature_set.name|api }}_ext_hashes);
{% endif %}
    return 1;
}

{% endif %}
static int glad_gl_find_core_{{ api|lower }}({{ template_utils.context_arg() }}) {
    int i;
    const char* version;
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        "OpenGL SC ",
        NULL
    };
    int major = 0;
    int minor = 0;
    unsigned short version_value;
    version = (const char*) {{ 'glGetString'|ctx }}(GL_VERSION);
    if (!version) return 0;
    for (i = 0;  prefixes[i];  i++) {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0) {
            version += length;
            break;
        }
    }

    GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);

    version_value = (major << 8U) | minor;

{% for feature in feature_set.features|select('supports', api) %}
    {{ ('GLAD_' + feature.name)|ctx(name_only=True) }} = version_value >= 0x{{ '%02x%02x'|format(feature.version.major, feature.version.minor) }};
{% endfor %}

    return GLAD_MAKE_VERSION(major, minor);
}

GLAD_NO_INLINE int gladLoad{{ api|api }}ContextUserPtr({{ template_utils.context_arg(', ') }}GLADuserptrloadfunc load, void *userptr) {
    int version;
{% if options.use_pfn_ranges %}
    uint32_t i;
{% endif %}

    {{ 'glGetString'|ctx }} = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    if({{ 'glGetString'|ctx }} == NULL) return 0;
    version = glad_gl_find_core_{{ api|lower }}(context);

{% if options.use_pfn_ranges %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_feature_pfn_ranges); ++i) {
        const GladPfnRange_t *range = &GLAD_{{ feature_set.name|api }}_feature_pfn_ranges[i];
        if (context->featArray[range->extension]) {
            glad_{{ spec.name }}_load_pfn_range(context, load, userptr, range->start, range->count);
        }
    }
{% else %}
{% for feature, _ in loadable(feature_set.features, api=api) %}
    glad_gl_load_{{ feature.name }}(context, load, userptr);
{% endfor %}
{% endif %}

{% if feature_set.extensions|length > 0 %}
    if (!glad_gl_find_extensions_{{ api|lower }}(context)) return 0;

{% if options.use_pfn_ranges %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_ext_pfn_ranges); ++i) {
        const GladPfnRange_t *range = &GLAD_{{ feature_set.name|api }}_ext_pfn_ranges[i];
        if (context->extArray[range->extension]) {
            glad_{{ spec.name }}_load_pfn_range(context, load, userptr, range->start, range->count);
        }
    }
{% else %}
{% for extension, _ in loadable(feature_set.extensions, api=api) %}
    glad_gl_load_{{ extension.name }}(context, load, userptr);
{% endfor %}
{% endif%}

{% endif %}
    gladSet{{ feature_set.name|api }}Context(context);

{% if options.alias %}
    glad_gl_resolve_aliases(context);

{% endif %}
    return version;
}

int gladLoad{{ api|api }}UserPtr(GLADuserptrloadfunc load, void *userptr) {
    return gladLoad{{ api|api }}ContextUserPtr(gladGet{{ feature_set.name|api }}Context(), load, userptr);
}

int gladLoad{{ api|api }}Context({{ template_utils.context_arg(', ') }}GLADloadfunc load) {
    return gladLoad{{ api|api }}ContextUserPtr(context, glad_gl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}

int gladLoad{{ api|api }}(GLADloadfunc load) {
    return gladLoad{{ api|api }}Context(gladGet{{ feature_set.name|api }}Context(), load);
}

{% endfor %}
Glad{{ feature_set.name|api }}Context* gladGet{{ feature_set.name|api }}Context() {
    return &{{ global_context }};
}

void gladSet{{ feature_set.name|api }}Context(Glad{{ feature_set.name|api }}Context *context) {
    if (!context) return;
    if (&{{ global_context }} == context) return;
    {{ global_context }} = *context;
}

{% endblock %}
