{% import 'template_utils.h' as template_utils with context %}
/**
 * SPDX-License-Identifier: (WTFPL OR CC0-1.0) AND Apache-2.0
 */
{% block includes %}
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#define XXH_VECTOR XXH_SSE2
#include <immintrin.h>
#elif defined(__aarch64__) || defined(__arm__) || defined(_M_ARM) || defined(_M_ARM64)
#define XXH_VECTOR XXH_NEON
#include <arm_neon.h>
#endif

#define XXH_INLINE_ALL
#include "xxhash.h"

{% if not options.header_only %}
{% block glad_include %}
#include <glad/{{ feature_set.name }}.h>
{% endblock %}
{% endif %}

{% block preimpl %}
{% endblock %}
{% include 'impl_util.c' %}
{% block hashsearch %}
{% if not options.no_extension_detection %}

{% include 'hash_search.c' %}
{% endif %}
{% endblock %}
{% endblock %}

#ifdef __cplusplus
extern "C" {
#endif

{% set global_context = 'glad_' + feature_set.name + '_context' -%}
{% block variables %}
{% if options.mx_global %}
{% call template_utils.zero_initialized() %}Glad{{ feature_set.name|api }}Context {{ global_context }}{% endcall %}
{% endif %}
{% endblock %}

{%block funcnames %}
static const char *GLAD_{{ feature_set.name|api }}_fn_names[] = {
{% for command in feature_set.commands %}
    /* {{ "{:>4}".format(command.index)}} */ "{{ command.name }}"{% if not loop.last %},{% endif %}{{""}}
{% endfor %}
};

{% endblock %}
{%block extnames %}
static const char *GLAD_{{ feature_set.name|api }}_ext_names[] = {
{% for extension in feature_set.extensions %}
    /* {{ "{:>4}".format(extension.index)}} */ "{{ extension.name }}"{% if not loop.last %},{% endif %}{{""}}
{% endfor %}
};

{% endblock %}
{%block extranges %}
{% if options.use_pfn_ranges %}
static const GladPfnRange_t GLAD_{{ feature_set.name|api }}_feature_pfn_ranges[] = {
{% for extension, command_ranges in feature_ranges() %}
{% call template_utils.protect(extension) %}
    /* {{ extension.name }} */
{% for cmd_range in command_ranges %}
    { {{ "{:>4}".format(extension.index) }}, {{ "{:>4}".format(cmd_range.start) }}, {{ "{:>4}".format(cmd_range.count) }} },
{% endfor %}{% if not loop.last %}{{"\n"}}{% endif %}
{% endcall %}
{% endfor %}
};

static const GladPfnRange_t GLAD_{{ feature_set.name|api }}_ext_pfn_ranges[] = {
{% for extension, command_ranges in extension_ranges() %}
{% call template_utils.protect(extension) %}
    /* {{ extension.name }} */
{% for cmd_range in command_ranges %}
    { {{ "{:>4}".format(extension.index) }}, {{ "{:>4}".format(cmd_range.start) }}, {{ "{:>4}".format(cmd_range.count) }} },
{% endfor %}{% if not loop.last %}{{"\n"}}{% endif %}
{% endcall %}
{% endfor %}
};

{% endif %}
{% endblock %}
{% block commandidx %}
{% endblock %}
{% block exthashes %}
{% if not options.no_extension_detection %}
static uint64_t GLAD_{{ feature_set.name|api }}_ext_hashes[] = {
{% for extension in feature_set.extensions %}
    /* {{ "{:>4}".format(extension.index)}} */ {{ extension.hash }}{% if not loop.last %},{% else %} {% endif %} /* {{ extension.name }} */
{% endfor %}
};
{% endif %}
{% endblock %}
{% block extensions %}
{% if not options.mx %}

{% for extension in chain(feature_set.features, feature_set.extensions) %}
{% call template_utils.protect(extension) %}
int GLAD_{{ extension.name }} = 0;
{% endcall %}
{% endfor %}
{% endif %}
{% endblock %}
{% if not options.mx %}
{% block commands %}

{% for command in feature_set.commands|c_commands %}
{% call template_utils.protect(command) %}
{{ command.name|pfn }} glad_{{ command.name }} = NULL;
{% endcall %}
{% endfor %}
{% endblock %}
{% endif %}

{% block range_loader %}
{% if options.use_pfn_ranges %}
static void glad_{{ spec.name }}_load_pfn_range({{ template_utils.context_arg(', ') }}GLADuserptrloadfunc load, void* userptr, uint16_t pfnStart, uint32_t numPfns)
{
    uint32_t pfnIdx;

    #ifdef __clang__
    #pragma nounroll
    #endif
    for (pfnIdx = pfnStart; pfnIdx < pfnStart + numPfns; ++pfnIdx) {
        context->pfnArray[pfnIdx] = load(userptr, GLAD_{{ feature_set.name|api}}_fn_names[pfnIdx]);
    }
}

{% endif %}
{% endblock %}
{% block extension_loaders %}
{% if not options.no_extension_detection and not options.use_pfn_ranges %}
static void glad_{{ spec.name }}_load_pfns({{ template_utils.context_arg(', ') }}GLADuserptrloadfunc load, void* userptr, const uint16_t *pPfnIdx, uint32_t numPfns)
{
    uint32_t i;

    #ifdef __clang__
    #pragma nounroll
    #endif
    for (i = 0; i < numPfns; ++i) {
        const uint16_t pfnIdx = pPfnIdx[i];
        context->pfnArray[pfnIdx] = load(userptr, GLAD_{{ feature_set.name|api}}_fn_names[pfnIdx]);
    }
}

{% for extension, commands in loadable() %}
{% call template_utils.protect(extension) %}
static void glad_{{ spec.name }}_load_{{ extension.name }}({{ template_utils.context_arg(', ') }}GLADuserptrloadfunc load, void* userptr) {
    static const uint16_t s_pfnIdx[] = {
{% for command in commands|sort(attribute='index') %}
        {{ "{:>4}".format(command.index) }}{% if not loop.last %},{% else %} {% endif %} /* {{ command.name }} */
{% endfor %}
    };
{% if not options.no_extension_detection %}
    if (!{{ ('GLAD_' + extension.name)|ctx(name_only=True) }}) return;
{% endif %}
    glad_{{ spec.name }}_load_pfns({{'context, ' if options.mx }}load, userptr, s_pfnIdx, GLAD_ARRAYSIZE(s_pfnIdx));
}

{% endcall %}
{% endfor %}
{% endif %}
{% endblock %}
{% block aliasing %}
{% if options.alias %}
{% if aliases|length > 0 %}
static uint32_t glad_{{ spec.name }}_resolve_alias_group({{  template_utils.context_arg(', ') }}const GladAliasPair_t *pairs, uint32_t start_idx, uint32_t total_count) {
    void **pfnArray = context->pfnArray;
    uint16_t canonical_idx = pairs[start_idx].first;

    /* Find the end of this group (consecutive pairs with same canonical index) */
    uint32_t end_idx = start_idx;
    while (end_idx < total_count && pairs[end_idx].first == canonical_idx) {
        end_idx++;
    }

    /* Pass 1: Find any loaded secondary for this canonical */
    void *canonical_ptr = pfnArray[canonical_idx];
    if (canonical_ptr == NULL) {
        for (uint32_t i = start_idx; i < end_idx; ++i) {
            if (pfnArray[pairs[i].second] != NULL) {
                canonical_ptr = pfnArray[pairs[i].second];
                pfnArray[canonical_idx] = canonical_ptr;
                break;
            }
        }
    }

    /* Pass 2: Populate unloaded secondaries */
    if (canonical_ptr != NULL) {
        for (uint32_t i = start_idx; i < end_idx; ++i) {
            if (pfnArray[pairs[i].second] == NULL) {
                pfnArray[pairs[i].second] = canonical_ptr;
            }
        }
    }

    return end_idx - 1;  /* Return index of last processed pair */
}

{% endif %}
GLAD_NO_INLINE static void glad_{{ spec.name }}_resolve_aliases({{ template_utils.context_arg(def='void') }}) {
{%if aliases|length > 0 %}
    static const GladAliasPair_t s_aliases[] = {
{% for command in feature_set.commands|sort(attribute='name') %}
{% if aliases.get(command.name, [])|length > 0 %}
{% call template_utils.protect(command) %}
{% for alias in aliases.get(command.name, [])|reject('equalto', (command.name, command.index)) %}
{% call template_utils.protect(alias) %}
        { {{ "{:>4}".format(command.index) }}, {{ "{:>4}".format(alias[1]) }} }, /* {{ command.name }} and {{ alias[0] }} */
{% endcall %}
{% endfor %}
{% endcall %}
{% endif %}
{% endfor %}
    };
    uint32_t i;

    #ifdef __clang__
    #pragma nounroll
    #endif
    for (i = 0; i < GLAD_ARRAYSIZE(s_aliases); ++i) {
        i = glad_{{ spec.name }}_resolve_alias_group({{ 'context, ' if options.mx }}s_aliases, i, GLAD_ARRAYSIZE(s_aliases));
    }
{% else %}
{% if options.mx %}
    GLAD_UNUSED(context);
{% endif %}
{% endif %}
}
{% endif %}
{% endblock %}

{% block loader %}
{% endblock %}

{% if options.loader %}
{% block loader_impl %}
{% for api in feature_set.info.apis %}
{% include 'loader/' + api + '.c' %}
{% endfor %}
{% endblock %}
{% endif %}

#ifdef __cplusplus
}
#endif
{% block postimpl %}
{% endblock %}
