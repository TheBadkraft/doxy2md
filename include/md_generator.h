// md_generator.h
#ifndef MD_GENERATOR_H
#define MD_GENERATOR_H

#include <sigcore.h>
#include "parser.h"

typedef struct IGenerator {
    void (*generate)(string_builder, list, const string);
} IGenerator;

extern const IGenerator MDGenerator;

#endif // MD_GENERATOR_H
