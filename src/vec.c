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

static void print_copy(FileData f, VecData v) {
    //copy header
    fprintf(f.header, "void hf_%s_copy(%s vec, %s out);\n", v.prefix, v.name, v.name);

    //copy source
    fprintf(f.source, "\nvoid hf_%s_copy(%s vec, %s out) {\n", v.prefix, v.name, v.name);
    fprintf(f.source, "\tmemcpy(out, vec, sizeof(out[0]) * %d);\n", v.def.components);
    fprintf(f.source, "}\n");
}

static void print_add(FileData f, VecData v) {
    fprintf(f.header, "void hf_%s_add(%s a, %s b, %s out);\n", v.prefix, v.name, v.name, v.name);

    fprintf(f.source, "\nvoid hf_%s_add(%s a, %s b, %s out) {\n", v.prefix, v.name, v.name, v.name);
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "\tout[%d] = a[%d] + b[%d];\n", i, i, i);
    }
    fprintf(f.source, "}\n");
}

static void print_subtract(FileData f, VecData v) {
    fprintf(f.header, "void hf_%s_subtract(%s a, %s b, %s out);\n", v.prefix, v.name, v.name, v.name);

    fprintf(f.source, "\nvoid hf_%s_subtract(%s a, %s b, %s out) {\n", v.prefix, v.name, v.name, v.name);
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "\tout[%d] = a[%d] - b[%d];\n", i, i, i);
    }
    fprintf(f.source, "}\n");
}

static void print_multiply(FileData f, VecData v) {
    fprintf(f.header, "void hf_%s_multiply(%s vec, %s scalar, %s out);\n", v.prefix, v.name, v.type, v.name);

    fprintf(f.source, "\nvoid hf_%s_multiply(%s vec, %s scalar, %s out) {\n", v.prefix, v.name, v.type, v.name);
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "\tout[%d] = vec[%d] * scalar;\n", i, i);
    }
    fprintf(f.source, "}\n");
}

static void print_divide(FileData f, VecData v) {
    fprintf(f.header, "void hf_%s_divide(%s vec, %s scalar, %s out);\n", v.prefix, v.name, v.type, v.name);

    fprintf(f.source, "\nvoid hf_%s_divide(%s vec, %s scalar, %s out) {\n", v.prefix, v.name, v.type, v.name);
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "\tout[%d] = vec[%d] / scalar;\n", i, i);
    }
    fprintf(f.source, "}\n");
}

static void print_sqrmag(FileData f, VecData v) {
    fprintf(f.header, "%s hf_%s_square_magnitude(%s vec);\n", v.type, v.prefix, v.name);

    fprintf(f.source, "\n%s hf_%s_square_magnitude(%s vec) {\n", v.type, v.prefix, v.name);
    fprintf(f.source, "\treturn ");
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "vec[%d] * vec[%d]", i, i);
        if(i < (v.def.components - 1)) {
            fprintf(f.source, " + ");
        }
    }
    fprintf(f.source, ";\n");
    fprintf(f.source, "}\n");
}

static void print_mag(FileData f, VecData v) {
    char ret_type[32];
    char sqr_func[32];
    switch(v.def.type) {
        default://float and int
            sprintf(ret_type, "float");
            sprintf(sqr_func, "sqrtf");
            break;
    }
    char cast[32];
    switch(v.def.type) {
        case VECTYPE_INT://float and int
            sprintf(cast, "(float)");
            break;
        default:
            sprintf(cast, "");
            break;
    }

    fprintf(f.header, "%s hf_%s_magnitude(%s vec);\n", ret_type, v.prefix, v.name);

    fprintf(f.source, "\n%s hf_%s_magnitude(%s vec) {\n", ret_type, v.prefix, v.name);
    fprintf(f.source, "\treturn %s(%shf_%s_square_magnitude(vec));\n", sqr_func, cast, v.prefix);
    fprintf(f.source, "}\n");
}

static void print_dot(FileData f, VecData v) {
    fprintf(f.header, "%s hf_%s_dot(%s a, %s b);\n", v.type, v.prefix, v.name, v.name);

    fprintf(f.source, "\n%s hf_%s_dot(%s a, %s b) {\n", v.type, v.prefix, v.name, v.name);
    fprintf(f.source, "\treturn ");
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "a[%d] * b[%d]", i, i);
        if(i < (v.def.components - 1)) {
            fprintf(f.source, " + ");
        }
    }
    fprintf(f.source, ";\n");
    fprintf(f.source, "}\n");
}

static void print_cross(FileData f, VecData v) {
    if(v.def.components != 3) {
        return;
    }

    fprintf(f.header, "void hf_%s_cross(%s a, %s b, %s out);\n", v.prefix, v.name, v.name, v.name);

    fprintf(f.source, "\nvoid hf_%s_cross(%s a, %s b, %s out) {\n", v.prefix, v.name, v.name, v.name);
    fprintf(f.source, "\t%s tmp;\n", v.name);
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "\ttmp[%d] = a[%d] * b[%d] - a[%d] * b[%d];\n", i, (i + 1) % v.def.components, (i + 2) % v.def.components, (i + 2) % v.def.components, (i + 1) % v.def.components);
    }
    fprintf(f.source, "\tmemcpy(out, tmp, sizeof(out[0]) * %d);\n", v.def.components);
    fprintf(f.source, "}\n");
}

static void print_functions(FileData f, VecData v) {
    fprintf(f.header, "\n");
    print_copy(f, v);
    print_add(f, v);
    print_subtract(f, v);
    print_multiply(f, v);
    print_divide(f, v);
    print_sqrmag(f, v);
    print_mag(f, v);
    print_dot(f, v);
    print_cross(f, v);
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
        "#include <string.h>\n"
        "#include <math.h>\n"
    );

    FileData file_data = { header, source };

    size_t count = sizeof(defs) / sizeof(defs[0]);
    for(size_t i = 0; i < count; i++) {
        print_typedef(file_data, vec_data_create(defs[i]));
    }

    for(size_t i = 0; i < count; i++) {
        VecData v_data = vec_data_create(defs[i]);
        print_functions(file_data, v_data);
    }

    fprintf(header,
        "\n#endif//HF_VEC_H\n"
    );

    fclose(header);
    fclose(source);
}
