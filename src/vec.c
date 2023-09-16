#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "shared.h"

typedef enum VecType_e {
    VECTYPE_FLOAT,
    VECTYPE_INT,
} VecType;

typedef struct VecDef_s {
    VecType type;
    int components;
} VecDef;

typedef struct VecData_s {
    char name[32];
    char prefix[32];
    char type[32];

    VecDef def;
} VecData;

static VecData vec_data_create(VecDef def) {
    VecData out;

    char type_suffix[32];
    switch(def.type) {
        case VECTYPE_FLOAT:
            strcpy(type_suffix, "f");
            sprintf(out.type, "float");
            break;
        default://fallback to int
            strcpy(type_suffix, "i");
            sprintf(out.type, "int");
            break;
    }
    sprintf(out.name, "HF_Vec%d%s", def.components, type_suffix);
    sprintf(out.prefix, "vec%d%s", def.components, type_suffix);

    out.def = def;
    return out;
}

static VecDef defs[] = {
    { VECTYPE_FLOAT, 2 },
    { VECTYPE_INT, 2 },

    { VECTYPE_FLOAT, 3 },
    { VECTYPE_INT, 3 },
};

static void print_typedef(FileData f, VecData v) {
    fprintf(f.header, "typedef %s %s[%d];\n", v.type, v.name, v.def.components);
}

void create_vec(void) {
    (void)defs;

    FILE* header = fopen("./hf_vec.h", "w");
    fprintf(header,
        "#ifndef HF_VEC_H\n"
        "#define HF_VEC_H\n"
        "\n"
    );

    FILE* source = fopen("./hf_vec.c", "w");
    fprintf(source,
        "#include \"../include/hf_vec.h\"\n\n"
        "#include <math.h>\n"
    );

    FileData file_data = { header, source };

    size_t count = sizeof(defs) / sizeof(defs[0]);
    for(size_t i = 0; i < count; i++) {
        print_typedef(file_data, vec_data_create(defs[i]));
    }
    fprintf(file_data.header, "\n");

    fprintf(header,
        "#endif//HF_VEC_H\n"
    );

    fclose(header);
    fclose(source);
}
