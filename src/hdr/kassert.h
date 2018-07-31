#ifndef __LOKIOS_KASSERT_H
#define __LOKIOS_KASSERT_H

// Compile-time assertion.
#define KASSERT(exp) static_assert(exp, #exp)

#endif /* __LOKIOS_KASSERT_H */
