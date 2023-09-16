#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "shared.h"

typedef struct MatDimensions_s {
    int w;
    int h;
} MatDims;

typedef struct MatData_s {
    char name[32];
    char prefix[32];

    MatDims dim;
} MatData;

static MatDims matrix_dims[] = {
    { 1, 2 },
    { 1, 3 },
    { 1, 4 },

    { 2, 1 },
    { 2, 2 },
    { 2, 3 },
    { 2, 4 },

    { 3, 1 },
    { 3, 2 },
    { 3, 3 },
    { 3, 4 },

    { 4, 1 },
    { 4, 2 },
    { 4, 3 },
    { 4, 4 },
};

//checks wether a matrix dimension is available from the list of dimensions
static bool check_compatibility(int w, int h) {
    size_t count = sizeof(matrix_dims) / sizeof(matrix_dims[0]);
    for(size_t i = 0; i < count; i++) {
        MatDims dim = matrix_dims[i];
        if(dim.w == w && dim.h == h) {
            return true;
        }
    }
    return false;
}

static MatData mat_data_create(MatDims dim) {
    MatData data;
    data.dim = dim;
    if(dim.w == dim.h) {
        sprintf(data.name, "HF_Mat%df", dim.w);
        sprintf(data.prefix, "mat%df", dim.w);
    }
    else {
        sprintf(data.name, "HF_Mat%dx%df", dim.w, dim.h);
        sprintf(data.prefix, "mat%dx%df", dim.w, dim.h);
    }

    return data;
}

static void print_identity(FileData f_data, MatData m_data) {
    if(m_data.dim.w != m_data.dim.h) {
        return;
    }

    //identity header
    fprintf(f_data.header,
        "void hf_%s_identity(%s out);\n",
        m_data.prefix, m_data.name
    );

    //identity source
    fprintf(f_data.source,
        "\n"
        "void hf_%s_identity(%s out) {\n"
        "\tfloat values[] = {\n",
        m_data.prefix, m_data.name
    );

    for(int row = 0; row < m_data.dim.h; row++) {
        fprintf(f_data.source, "\t\t");
        for(int col = 0; col < m_data.dim.w; col++) {
            fprintf(f_data.source, col == row ? "1.f," : "0.f,");
            if(col < (m_data.dim.w - 1)) {
                fprintf(f_data.source, " ");
            }
        }
        fprintf(f_data.source, "\n");
    }

    fprintf(f_data.source,
        "\t};\n"
        "\tmemcpy(out, values, sizeof(float) * %d);\n"
        "}\n",
        m_data.dim.w * m_data.dim.h
    );
}

static void print_transpose(FileData f_data, MatData m_data) {
    MatData other_data = mat_data_create((MatDims) { .w = m_data.dim.h, .h = m_data.dim.w });
    if(!check_compatibility(other_data.dim.w, other_data.dim.h)) {
        return;
    }

    //transpose header
    fprintf(f_data.header,
        "void hf_%s_transpose(%s mat, %s out);\n",
        m_data.prefix, m_data.name, other_data.name
    );

    //transpose source
    fprintf(f_data.source,
        "\n"
        "void hf_%s_transpose(%s mat, %s out) {\n"
        "\t%s tmp;\n",
        m_data.prefix, m_data.name, other_data.name, other_data.name
    );

    fprintf(f_data.source,
        "\tfor(int i = 0; i < %d; i++) {\n"
        "\t\tfor(int j = 0; j < %d; j++) {\n"
        "\t\t\ttmp[i][j] = mat[j][i];\n"
        "\t\t}\n"
        "\t}\n"
        "\tmemcpy(out, tmp, sizeof(float) * %d);\n",
        m_data.dim.w, m_data.dim.h, m_data.dim.w * m_data.dim.h
    );

    fprintf(f_data.source, "}\n");
}

static void print_determinant(FileData f_data, MatData m_data) {
    if(m_data.dim.w != m_data.dim.h) {
        return;
    }

    //determinant header
    fprintf(f_data.header,
        "float hf_%s_determinant(%s mat);\n",
        m_data.prefix, m_data.name
    );

    //determinant source
    fprintf(f_data.source,
        "\n"
        "float hf_%s_determinant(%s mat) {\n",
        m_data.prefix, m_data.name
    );

    if(m_data.dim.w == 2) {//hand made logic
        fprintf(f_data.source,
            "\n"
            "\treturn mat[0][0] * mat[1][1] - mat[1][0] * mat[0][1];\n"
        );
    }
    else {//recursive logic, det(mat_nxn) = mat[0][0] * det(mat_n-1xn-1) - mat[1][0] * det(mat_n-1xn-1) + (...)
        MatData n_minus_one = mat_data_create((MatDims) { .w = m_data.dim.w - 1, .h = m_data.dim.h - 1 });

        fprintf(f_data.source, "\t%s", n_minus_one.name);
        for(int i = 0; i < m_data.dim.w; i++) {
            fprintf(f_data.source, " mat_%d", i);
            if(i < (m_data.dim.w - 1)) {
                fprintf(f_data.source, ",");
            }
        }
        fprintf(f_data.source, ";\n");
        for(int i = 0; i < m_data.dim.w ; i++) {
            for(int j = 0; j < (m_data.dim.w - 1); j++) {
                int col = i <= j ? (j + 1) : j;
                fprintf(f_data.source, "\tmemcpy(&mat_%d[%d][0], &mat[%d][1], sizeof(float) * %d);\n", i, j, col, m_data.dim.w - 1);
            }
        }

        fprintf(f_data.source, "\treturn");
        for(int i = 0; i < m_data.dim.w; i++) {
            fprintf(f_data.source, (i % 2) == 0 ? "\n\t\t+(" : "\n\t\t-(");
            fprintf(f_data.source, "mat[%d][0] * hf_%s_determinant(mat_%d)", i, n_minus_one.prefix, i);
            fprintf(f_data.source, ")");
        }
        fprintf(f_data.source, "\n\t;\n");
    }
    fprintf(f_data.source, "}\n");
}

static void print_minor(FileData f_data, MatData m_data) {
    if(m_data.dim.w != m_data.dim.h) {
        return;
    }
    //minor header
    fprintf(f_data.header,
        "float hf_%s_minor(%s mat, int i, int j);\n",
        m_data.prefix, m_data.name
    );

    //minor source
    fprintf(f_data.source,
        "\n"
        "float hf_%s_minor(%s mat, int i, int j) {\n",
        m_data.prefix, m_data.name
    );

    if(m_data.dim.w == 2) {
        fprintf(f_data.source, "\treturn mat[1 - i][1 - j];\n");
    }
    else {
        MatData m_n_minus_one = mat_data_create((MatDims) { .w = m_data.dim.w - 1, .h = m_data.dim.h - 1 });
        fprintf(f_data.source, "\t%s mat_sub;\n", m_n_minus_one.name);
        fprintf(f_data.source,
        "\tint row = 0;\n"
        "\tfor(int k = 0; k < %d; k++) {\n"
        "\t\tif(k == i) {\n"
        "\t\t\tcontinue;\n"
        "\t\t}\n"
        "\t\tint col = 0;\n"
        "\t\tfor(int l = 0; l < %d; l++) {\n"
        "\t\t\tif(l == j) {\n"
        "\t\t\t\tcontinue;\n"
        "\t\t\t}\n"
        "\t\t\tmat_sub[row][col] = mat[k][l];\n"
        "\t\t\tcol++;\n"
        "\t\t}\n"
        "\t\trow++;\n"
        "\t}\n",
        m_data.dim.w, m_data.dim.h);
        fprintf(f_data.source, "\treturn hf_%s_determinant(mat_sub);\n", m_n_minus_one.prefix);
    }

    fprintf(f_data.source, "}\n");
}

static void print_inverse(FileData f_data, MatData m_data) {
    if(m_data.dim.w != m_data.dim.h) {
        return;
    }
    //minor header
    fprintf(f_data.header,
        "void hf_%s_inverse(%s mat, %s out);\n",
        m_data.prefix, m_data.name, m_data.name
    );

    //minor source
    fprintf(f_data.source,
        "\n"
        "void hf_%s_inverse(%s mat, %s out) {\n",
        m_data.prefix, m_data.name, m_data.name
    );

    fprintf(f_data.source,
        "\tfloat det = hf_%s_determinant(mat);\n"
        "\tif(det == 0.0f) {\n"
        "\t\treturn;\n"
        "\t}\n",
        m_data.prefix
    );

    fprintf(f_data.source,
        "\tfor(int i = 0; i < %d; i++) {\n"
        "\t\tfor(int j = 0; j < %d; j++) {\n"
        "\t\t\tout[j][i] = ((i + j) %% 2 == 0 ? 1.f : -1.f) * hf_%s_minor(mat, i, j);\n"
        "\t\t}\n"
        "\t}\n",
        m_data.dim.w, m_data.dim.h, m_data.prefix
    );
    fprintf(f_data.source,
        "\thf_%s_multiply(out, 1.f / det, out);\n",
        m_data.prefix
    );

    fprintf(f_data.source, "}\n");
}

static void print_scalar(FileData f, MatData m) {
    //header
    fprintf(f.header,
        "void hf_%s_multiply(%s mat, float scalar, %s out);\n",
        m.prefix, m.name, m.name
    );

    //source
    fprintf(f.source,
        "\nvoid hf_%s_multiply(%s mat, float scalar, %s out) {\n"
        "\tfor(int i = 0; i < %d; i++) {\n"
        "\t\tfor(int j = 0; j < %d; j++) {\n"
        "\t\t\tout[i][j] = mat[i][j] * scalar;\n"
        "\t\t}\n"
        "\t}\n"
        "}\n",
        m.prefix, m.name, m.name,
        m.dim.w,
        m.dim.h
    );
}

static void print_add(FileData f, MatData m) {
    //header
    fprintf(f.header,
        "void hf_%s_add(%s a, %s b, %s out);\n",
        m.prefix, m.name, m.name, m.name
    );

    //source
    fprintf(f.source,
        "\n"
        "void hf_%s_add(%s a, %s b, %s out) {\n"//0
        "\tfor(int i = 0; i < %d; i++) {\n"//1
        "\t\tfor(int j = 0; j < %d; j++) {\n"//2
        "\t\t\tout[i][j] = a[i][j] + b[i][j];\n"//3
        "\t\t}\n"//4
        "\t}\n"//5
        "}\n"
        ,
        m.prefix, m.name, m.name, m.name,//0
        m.dim.w,//2
        m.dim.h//3
    );
}

static void print_multiply(FileData f, MatData a, MatData b) {
    MatData data_res = mat_data_create((MatDims) { a.dim.w, b.dim.h });
    if(!check_compatibility(data_res.dim.w, data_res.dim.h)) {
        return;
    }

    //header
    fprintf(f.header,
        "void hf_%s_multiply_%s(%s a, %s b, %s out);\n",
        a.prefix, b.prefix, a.name, b.name, data_res.name
    );

    //source
    fprintf(f.source,
        "\n"
        "void hf_%s_multiply_%s(%s a, %s b, %s out) {\n"//0
        "\t%s tmp;\n"//1
        "\tfor(int i = 0; i < %d; i++) {\n"//2
        "\t\tfor(int j = 0; j < %d; j++) {\n"//3
        "\t\t\tfloat val = 0.f;\n"//4
        "\t\t\tfor(int k = 0; k < %d; k++) {\n"//5
        "\t\t\t\tval += a[i][k] * b[k][j];\n"//6
        "\t\t\t}\n"//7
        "\t\t\ttmp[i][j] = val;\n"//8
        "\t\t}\n"//9
        "\t}\n"//10
        "\tmemcpy(out, tmp, sizeof(float) * %d);\n"//11
        "}\n"
        ,
        a.prefix, b.prefix, a.name, b.name, data_res.name,//0
        data_res.name,//1
        data_res.dim.w,//2
        data_res.dim.h,//3
        a.dim.w,//5
        data_res.dim.w * data_res.dim.h//11
    );
}

static void print_typedef(FileData f, MatData m) {
    fprintf(f.header,
        "typedef float %s[%d][%d];\n",
        m.name, m.dim.w, m.dim.h
    );
}

static void print_functions(FileData f, MatData m) {
    print_identity(f, m);
    print_transpose(f, m);
    print_determinant(f, m);
    print_minor(f, m);
    print_inverse(f, m);

    print_add(f, m);
    print_scalar(f, m);
    size_t num_mats = sizeof(matrix_dims) / sizeof(matrix_dims[0]);
    for(size_t i = 0; i < num_mats; i++) {//generate multiply functions for each compatible matrix dimension
        MatDims dim_b = matrix_dims[i];
        if(m.dim.h == dim_b.w) {//compatible
            MatData b = mat_data_create(dim_b);
            print_multiply(f, m, b);
        }
    }
    fprintf(f.header, "\n");
}

void create_mat(void) {
    FILE* header = fopen("./hf_mat.h", "w");
    fprintf(header,
        "#ifndef HF_MAT_H\n"
        "#define HF_MAT_H\n"
        "\n"
    );

    FILE* source = fopen("./hf_mat.c", "w");
    fprintf(source,
        "#include \"../include/hf_mat.h\"\n\n"
        "#include <string.h>\n"
    );

    FileData file_data = { header, source };

    size_t num_mats = sizeof(matrix_dims) / sizeof(MatDims);
    for(size_t i = 0; i < num_mats; i++) {
        MatData mat_data = mat_data_create(matrix_dims[i]);
        print_typedef(file_data, mat_data);
    }
    fprintf(header, "\n");

    for(size_t i = 0; i < num_mats; i++) {
        MatData mat_data = mat_data_create(matrix_dims[i]);
        print_functions(file_data, mat_data);
    }

    fprintf(header,
        "#endif//HF_MAT_H\n"
    );

    fclose(header);
    fclose(source);
}
