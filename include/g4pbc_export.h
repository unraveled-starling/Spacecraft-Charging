
#ifndef G4PBC_EXPORT_H
#define G4PBC_EXPORT_H

#ifdef G4PBC_STATIC_DEFINE
#  define G4PBC_EXPORT
#  define G4PBC_NO_EXPORT
#else
#  ifndef G4PBC_EXPORT
#    ifdef g4pbc_EXPORTS
        /* We are building this library */
#      define G4PBC_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define G4PBC_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef G4PBC_NO_EXPORT
#    define G4PBC_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef G4PBC_DEPRECATED
#  define G4PBC_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef G4PBC_DEPRECATED_EXPORT
#  define G4PBC_DEPRECATED_EXPORT G4PBC_EXPORT G4PBC_DEPRECATED
#endif

#ifndef G4PBC_DEPRECATED_NO_EXPORT
#  define G4PBC_DEPRECATED_NO_EXPORT G4PBC_NO_EXPORT G4PBC_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef G4PBC_NO_DEPRECATED
#    define G4PBC_NO_DEPRECATED
#  endif
#endif

#endif /* G4PBC_EXPORT_H */
