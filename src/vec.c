#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "shared.h"

typedef enum vec_type_e {
    vec_type_int,
    vec_type_float,
    vec_type_double,
} vec_type;

typedef struct vec_def_s {
    vec_type type;
    int components;
} vec_def;

typedef struct vec_data_s {
    char name[32];
    char prefix[32];
    char type[32];

    vec_def def;
} vec_data;

static vec_data vec_data_create(vec_def def) {
    vec_data out;

    char type_suffix[32];
    switch(def.type) {
        case vec_type_float:
            strcpy(type_suffix, "f");
            sprintf(out.type, "float");
            break;
        case vec_type_double:
            strcpy(type_suffix, "d");
            sprintf(out.type, "double");
            break;
        default://fallback to int
            strcpy(type_suffix, "i");
            sprintf(out.type, "int");
            break;
    }
    sprintf(out.name, "hf_vec%d%s", def.components, type_suffix);
    sprintf(out.prefix, "vec%d%s", def.components, type_suffix);

    out.def = def;
    return out;
}

static vec_def defs[] = {
    { vec_type_int, 2 },
    { vec_type_float, 2 },
    { vec_type_double, 2 },

    { vec_type_int, 3 },
    { vec_type_float, 3 },
    { vec_type_double, 3 },

    { vec_type_int, 4 },
    { vec_type_float, 4 },
    { vec_type_double, 4 },
};

static void print_typedef(FileData f, vec_data v) {
    fprintf(f.header, "typedef %s %s[%d];\n", v.type, v.name, v.def.components);
}

static void print_copy(FileData f, vec_data v) {
    //copy header
    fprintf(f.header, "void hf_%s_copy(%s vec, %s out);\n", v.prefix, v.name, v.name);

    //copy source
    fprintf(f.source, "\nvoid hf_%s_copy(%s vec, %s out) {\n", v.prefix, v.name, v.name);
    fprintf(f.source, "\tmemcpy(out, vec, sizeof(out[0]) * %d);\n", v.def.components);
    fprintf(f.source, "}\n");
}

static void print_add(FileData f, vec_data v) {
    fprintf(f.header, "void hf_%s_add(%s a, %s b, %s out);\n", v.prefix, v.name, v.name, v.name);

    fprintf(f.source, "\nvoid hf_%s_add(%s a, %s b, %s out) {\n", v.prefix, v.name, v.name, v.name);
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "\tout[%d] = a[%d] + b[%d];\n", i, i, i);
    }
    fprintf(f.source, "}\n");
}

static void print_subtract(FileData f, vec_data v) {
    fprintf(f.header, "void hf_%s_subtract(%s a, %s b, %s out);\n", v.prefix, v.name, v.name, v.name);

    fprintf(f.source, "\nvoid hf_%s_subtract(%s a, %s b, %s out) {\n", v.prefix, v.name, v.name, v.name);
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "\tout[%d] = a[%d] - b[%d];\n", i, i, i);
    }
    fprintf(f.source, "}\n");
}

static void print_multiply(FileData f, vec_data v) {
    fprintf(f.header, "void hf_%s_multiply(%s vec, %s scalar, %s out);\n", v.prefix, v.name, v.type, v.name);

    fprintf(f.source, "\nvoid hf_%s_multiply(%s vec, %s scalar, %s out) {\n", v.prefix, v.name, v.type, v.name);
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "\tout[%d] = vec[%d] * scalar;\n", i, i);
    }
    fprintf(f.source, "}\n");
}

static void print_divide(FileData f, vec_data v) {
    fprintf(f.header, "void hf_%s_divide(%s vec, %s scalar, %s out);\n", v.prefix, v.name, v.type, v.name);

    fprintf(f.source, "\nvoid hf_%s_divide(%s vec, %s scalar, %s out) {\n", v.prefix, v.name, v.type, v.name);
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "\tout[%d] = vec[%d] / scalar;\n", i, i);
    }
    fprintf(f.source, "}\n");
}

static void print_normalize(FileData f, vec_data v) {
    switch(v.def.type) {//not available for int types
        case vec_type_int:
            return;
        default:
            break;
    }

    fprintf(f.header, "void hf_%s_normalize(%s vec, %s out);\n", v.prefix, v.name, v.name);

    fprintf(f.source, "\nvoid hf_%s_normalize(%s vec, %s out) {\n", v.prefix, v.name, v.name);
    fprintf(f.source, "\thf_%s_divide(vec, hf_%s_magnitude(vec), out);\n", v.prefix, v.prefix);
    fprintf(f.source, "}\n");
}

static void print_lerp(FileData f, vec_data v) {
    char literal_suffix[32];
    switch(v.def.type) {
        case vec_type_float:
            strcpy(literal_suffix, ".f");
            break;
        default:
            return;
    }

    fprintf(f.header, "void hf_%s_lerp(%s a, %s b, %s t, %s out);\n", v.prefix, v.name, v.name, v.type, v.name);

    fprintf(f.source, "\nvoid hf_%s_lerp(%s a, %s b, %s t, %s out) {\n", v.prefix, v.name, v.name, v.type, v.name);
    for(int i = 0; i < v.def.components; i++) {
        fprintf(f.source, "\tout[%d] = a[%d] * (1%s - t) + b[%d] * t;\n", i, i, literal_suffix, i);
    }
    fprintf(f.source, "}\n");
}

static void print_sqrmag(FileData f, vec_data v) {
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

static void print_mag(FileData f, vec_data v) {
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
        case vec_type_int://float and int
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

static void print_dot(FileData f, vec_data v) {
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

static void print_cross(FileData f, vec_data v) {
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

static void print_functions(FileData f, vec_data v) {
    fprintf(f.header, "\n");
    print_copy(f, v);
    print_add(f, v);
    print_subtract(f, v);
    print_multiply(f, v);
    print_divide(f, v);
    print_normalize(f, v);
    print_lerp(f, v);
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
        vec_data v_data = vec_data_create(defs[i]);
        print_functions(file_data, v_data);
    }

    fprintf(header,
        "\n#endif//HF_VEC_H\n"
    );

    fclose(header);
    fclose(source);
}
