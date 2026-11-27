#ifndef XENO_DATA_ALIAS_H
#define XENO_DATA_ALIAS_H

/* Every interior alias shares its backing object on both targets. ELF also
 * carries a symbol size. The PE bounds generator consumes the same declarations
 * and verifies the offsets against the compiled COFF symbol table. */
#ifdef _WIN32
#define XG_ALIAS_OBJECT_INFO(name, extent) ""
#else
#define XG_ALIAS_OBJECT_INFO(name, extent) \
    ".type " #name ", @object\n.size " #name ", " #extent "\n"
#endif

#define XG_DEFINE_DATA_ALIAS(name, backing, offset, extent, metadata)         \
    _Static_assert((offset) <= sizeof(backing) &&                             \
                   (extent) <= sizeof(backing) - (offset),                    \
                   #name " extends past " #backing);                          \
    __asm__(".globl " #name "\n.set " #name ", " #backing " + " #offset "\n"   \
            metadata)

#define XG_SIZED_DATA_ALIAS(name, backing, offset, extent)  \
    XG_DEFINE_DATA_ALIAS(name, backing, offset, extent, XG_ALIAS_OBJECT_INFO(name, extent))

/* Keep an existing un-sized ELF alias. The Windows battle bridge uses only
 * the specified slice of its backing storage, including on LLP64 where a
 * guest pointer occupies four bytes of an eight-byte native pointer slot. */
#define XG_BRIDGE_DATA_ALIAS(name, backing, offset, extent) \
    XG_DEFINE_DATA_ALIAS(name, backing, offset, extent, "")

#endif
