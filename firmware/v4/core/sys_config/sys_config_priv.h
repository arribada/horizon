#ifndef _SYS_CONFIG_PRIV_H_
#define _SYS_CONFIG_PRIV_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SYS_CONFIG_TAG(TAG, DATA, COMPULSORY)  SYS_CONFIG_TAG_GET_SET(TAG, DATA, COMPULSORY, NULL, NULL)

#define SYS_CONFIG_TAG_GET_SET(TAG, DATA, COMPULSORY, GETTER, SETTER) \
{                                                                     \
    .tag = TAG,                                                       \
    .data = (void *) &DATA,                                           \
    .length = sizeof(DATA) - sizeof(sys_config_hdr_t),                \
    .compulsory = COMPULSORY,                                         \
    .getter = GETTER,                                                 \
    .setter = SETTER                                                  \
}

#define SYS_CONFIG_REQUIRED_IF_MATCH(TAG, REQUIRED, ADDRESS, VALUE) \
{                                                                   \
    .tag = TAG,                                                     \
    .tag_dependant = REQUIRED,                                      \
    .address = (void *) &ADDRESS,                                   \
    .bitmask = (__typeof__(ADDRESS)) 0xFFFFFFFF,                    \
    .value = VALUE                                                  \
}

#define SYS_CONFIG_REQUIRED_IF_MATCH_BITMASK(TAG, REQUIRED, ADDRESS, BITMASK, VALUE) \
{                                                                                    \
    .tag = TAG,                                                                      \
    .tag_dependant = REQUIRED,                                                       \
    .address = (void *) &ADDRESS,                                                    \
    .bitmask = (__typeof__(ADDRESS)) BITMASK,                                        \
    .value = VALUE                                                                   \
}

typedef struct __attribute__((__packed__))
{
    uint16_t tag;
    void * data;
    size_t length;
    bool compulsory;
    void (*getter)(void);
    void (*setter)(void);
} sys_config_lookup_table_t;

typedef struct __attribute__((__packed__))
{
    uint16_t tag;
    uint16_t tag_dependant;
    void * address;
    uint32_t bitmask;
    uint32_t value;
} dependancy_lookup_table_t;

#define NUM_OF_TAGS         (sizeof(sys_config_lookup_table) / sizeof(sys_config_lookup_table_t))
#define NUM_OF_DEPENDENCIES (sizeof(dependancy_lookup_table) / sizeof(dependancy_lookup_table_t))

#define TAG_SET_FLAG(TAG_LOOKUP) (((sys_config_hdr_t *)TAG_LOOKUP.data)->set)

#endif /* _SYS_CONFIG_PRIV_H_ */