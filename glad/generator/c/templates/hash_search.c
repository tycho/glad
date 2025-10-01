{% if feature_set.extensions|length > 0 %}

#ifndef GLAD_IMPL_UTIL_HASHSEARCH_C_
#define GLAD_IMPL_UTIL_HASHSEARCH_C_

#ifdef __cplusplus
extern "C" {
#endif

{# Configure the search algorithm used to find matching hashes in arrays. #}
{# 0 = binary search, 1 = explicit SIMD, 2 = auto-vectorized, 3 = naive linear search #}
{% if search_type == 0 %}
GLAD_NO_INLINE static bool glad_hash_search(const uint64_t *arr, uint32_t size, uint64_t target) {
    /* Binary search for matching hash */
    int32_t low = 0;
    int32_t high = (int32_t)size - 1;

    while (low <= high) {
        int32_t mid = low + (high - low) / 2;

        if (arr[mid] == target) {
            return true;
        } else if (arr[mid] < target) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return false;
}

GLAD_NO_INLINE static void glad_sort_hashes(uint64_t *a, size_t n) {
    /* Ciura gap sequence; we’ll skip the big ones at runtime. */
    static const size_t gaps[] = {701, 301, 132, 57, 23, 10, 4, 1};
    size_t gi = 0;

    if (!a || n < 2)
        return;

    while (gi < GLAD_ARRAYSIZE(gaps) && gaps[gi] >= n)
        gi++;

    for (; gi < GLAD_ARRAYSIZE(gaps); ++gi) {
        size_t gap = gaps[gi], i;
        for (i = gap; i < n; ++i) {
            uint64_t v = a[i];
            size_t j = i;
            /* gapped insertion sort */
            while (j >= gap && a[j - gap] > v) {
                a[j] = a[j - gap];
                j -= gap;
            }
            a[j] = v;
        }
    }
}

{% endif %}
{% if search_type == 1 %}
static bool glad_hash_search_linear_slow(const uint64_t *arr, uint32_t size, uint64_t target) {
    uint32_t i;
    for (i = 0; i < size; ++i) {
        if (arr[i] == target)
            return true;
    }
    return false;
}

GLAD_NO_INLINE static bool glad_hash_search(const uint64_t *arr, uint32_t size, uint64_t target) {
    /* Optimized SIMD-based linear search. */
    bool found = false;
    uint32_t i = 0;

    if (size >= 2) {
#if defined(__SSE4_1__)
        /* SSE4.1 intrinsics for uint64x2 */
        __m128i target_simd = _mm_set1_epi64x(target);

        for (; i < size - 1; i += 2) {
            __m128i arr_simd = _mm_loadu_si128((__m128i*)&arr[i]);
            __m128i cmp_result = _mm_cmpeq_epi64(arr_simd, target_simd);
            if (_mm_movemask_epi8(cmp_result) != 0)
                return true;
        }
#elif defined(__ARM_NEON)
        /* ARM NEON intrinsics for uint64x2 */
        uint64x2_t target_simd = vld1q_dup_u64(&target);

        for (; i < size - 1; i += 2) {
            uint64x2_t arr_simd = vld1q_u64(&arr[i]);
            uint64x2_t cmp_result = vceqq_u64(arr_simd, target_simd);

            if (vgetq_lane_u64(cmp_result, 0) || vgetq_lane_u64(cmp_result, 1))
                return true;
        }
#else
        /* Clang auto-vectorized fallback for uint64x2 (no early loop termination though) */
        for (; i < size; ++i) {
            if (arr[i] == target)
                found = true;
        }
#endif
    }

    if (found || i == size)
        return found;

    /* Remainder loop, if needed */
    return glad_hash_search_linear_slow(&arr[i], size - i, target);
}

{% endif %}
{% if search_type == 2 %}
GLAD_NO_INLINE static bool glad_hash_search(const uint64_t *arr, uint32_t size, uint64_t target) {
    /* This linear search works well with auto-vectorization, but it will scan
     * the entire array and not stop early on a match */
    uint32_t i;
    bool found = false;
    for (i = 0; i < size; ++i) {
        if (arr[i] == target)
            found = true;
    }
    return found;
}

{% endif %}
{% if search_type == 3 %}
GLAD_NO_INLINE static bool glad_hash_search(const uint64_t *arr, uint32_t size, uint64_t target) {
    /* This is a very straightforward linear search. It does not auto-vectorize,
     * but it is very compact. */
    uint32_t i;
    for (i = 0; i < size; ++i) {
        if (arr[i] == target)
            return true;
    }
    return false;
}

{% endif %}
GLAD_NO_INLINE static uint64_t glad_hash_string(const char *str, size_t length)
{
    return XXH3_64bits(str, length);
}

#ifdef __cplusplus
}
#endif

#endif /* GLAD_IMPL_HASHSEARCH_C_ */
{% endif %}
