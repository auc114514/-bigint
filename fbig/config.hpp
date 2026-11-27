#ifndef FBIG_CONFIG_HPP
#define FBIG_CONFIG_HPP

#if defined(_MSVC_LANG)
    #define FBIG_CPP_STANDARD _MSVC_LANG
#else
    #define FBIG_CPP_STANDARD __cplusplus
#endif

#if FBIG_CPP_STANDARD >= 201402L
    #define FBIG_CONSTEXPR14 constexpr
#else
    #define FBIG_CONSTEXPR14 inline
#endif

#if defined(__SIZEOF_INT128__)
    #define FBIG_HAS_INT128 1
#else
    #define FBIG_HAS_INT128 0
#endif

#endif
