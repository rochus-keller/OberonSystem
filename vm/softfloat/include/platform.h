/* Berkeley SoftFloat platform configuration.
 * INLINE is set here instead of on the compiler command line because its value
 * contains a space, which not all build systems can pass through.
 */

#ifndef INLINE
#define INLINE static inline
#endif
