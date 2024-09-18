#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC_STRING 0xaaae
#define NAME_SIZE 23
#define COURSE_NAME_SIZE 32

// Estructura del estudiante
typedef struct {
    uint32_t id;
    uint8_t flags;    // Banderas (género y posgrado)
    uint32_t age;     // Edad del estudiante
} Student;

// Estructura de la matrícula
typedef struct {
    uint32_t student_id;
    uint32_t course_id;
    uint32_t year;
    uint32_t semester;
} Enrollment;

// Función para leer un valor de 16 bits en formato big endian
uint16_t read_big_endian_16(FILE *file) {
    uint8_t byte1, byte2;
    fread(&byte1, sizeof(uint8_t), 1, file);
    fread(&byte2, sizeof(uint8_t), 1, file);
    return (byte1 << 8) | byte2;
}

// Función para leer un valor de 32 bits en formato little endian
uint32_t read_little_endian_32(FILE *file) {
    uint8_t byte1, byte2, byte3, byte4;
    fread(&byte1, sizeof(uint8_t), 1, file);
    fread(&byte2, sizeof(uint8_t), 1, file);
    fread(&byte3, sizeof(uint8_t), 1, file);
    fread(&byte4, sizeof(uint8_t), 1, file);
    return (uint32_t)(byte1) | (uint32_t)(byte2 << 8) | (uint32_t)(byte3 << 16) | (uint32_t)(byte4 << 24);
}

// Función que verifica si el estudiante es femenino
int is_female(uint8_t flags) {
    return (flags & 0x80) != 0;
}

// Función que verifica si el estudiante es de posgrado
int is_graduate(uint8_t flags) {
    return (flags & 0x40) != 0;
}

// Estructura para almacenar el conteo por año y semestre
typedef struct {
    uint32_t year;
    uint32_t semester;
    int male_undergrad;
    int female_undergrad;
    int male_graduate;
    int female_graduate;
} SemesterRecord;

// Función para verificar si ya existe un registro de año y semestre
int find_semester_record(SemesterRecord *records, uint32_t year, uint32_t semester, int count) {
    for (int i = 0; i < count; i++) {
        if (records[i].year == year && records[i].semester == semester) {
            return i;
        }
    }
    return -1; // No encontrado
}

// Función para validar que los valores del año y semestre tienen sentido
int is_valid_year_semester(uint32_t year, uint32_t semester) {
    // Rango razonable para el año (asume que el año es entre 1900 y 2100)
    if (year < 1900 || year > 2100) {
        return 0;
    }
    // Semestre debe ser 1, 2, 3 o 4
    if (semester < 1 || semester > 4) {
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <archivo_binario>\n", argv[0]);
        return 1;
    }

    // Abrir archivo binario
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return 1;
    }

    // Leer y verificar la cadena mágica en formato big endian
    uint16_t magic = read_big_endian_16(file);
    if (magic != MAGIC_STRING) {
        printf("Archivo binario no válido. Se esperaba la cadena mágica: 0xaaae, pero se leyó: 0x%04x\n", magic);
        fclose(file);
        return 1;
    }

    // Leer la cantidad de estudiantes, cursos y matrículas
    uint32_t student_count = read_little_endian_32(file);
    uint32_t course_count = read_little_endian_32(file);
    uint32_t enrollment_count = read_little_endian_32(file);

    // Leer los estudiantes
    Student *students = malloc(student_count * sizeof(Student));
    for (uint32_t i = 0; i < student_count; i++) {
        students[i].id = read_little_endian_32(file);
        fread(&students[i].flags, sizeof(uint8_t), 1, file);
        fseek(file, NAME_SIZE, SEEK_CUR); // Saltar el nombre del estudiante
        students[i].age = read_little_endian_32(file);
    }

    // Leer las matrículas
    Enrollment *enrollments = malloc(enrollment_count * sizeof(Enrollment));
    for (uint32_t i = 0; i < enrollment_count; i++) {
        enrollments[i].student_id = read_little_endian_32(file);
        enrollments[i].course_id = read_little_endian_32(file);
        enrollments[i].year = read_little_endian_32(file);
        enrollments[i].semester = read_little_endian_32(file);
    }

    // Arreglo dinámico para almacenar los registros por semestre
    SemesterRecord *records = malloc(enrollment_count * sizeof(SemesterRecord));
    int record_count = 0;

    // Procesar las matrículas
    for (uint32_t i = 0; i < enrollment_count; i++) {
        uint32_t student_id = enrollments[i].student_id;
        uint32_t year = enrollments[i].year;
        uint32_t semester = enrollments[i].semester;

        // Validar año y semestre antes de continuar
        if (!is_valid_year_semester(year, semester)) {
            continue; // Saltar registros no válidos
        }

        // Buscar el estudiante
        Student *student = NULL;
        for (uint32_t j = 0; j < student_count; j++) {
            if (students[j].id == student_id) {
                student = &students[j];
                break;
            }
        }

        if (student != NULL) {
            // Verificar si ya existe un registro para este semestre
            int record_index = find_semester_record(records, year, semester, record_count);
            if (record_index == -1) {
                // Si no existe, crear uno nuevo
                records[record_count].year = year;
                records[record_count].semester = semester;
                records[record_count].male_undergrad = 0;
                records[record_count].female_undergrad = 0;
                records[record_count].male_graduate = 0;
                records[record_count].female_graduate = 0;
                record_index = record_count;
                record_count++;
            }

            // Clasificar al estudiante
            if (is_graduate(student->flags)) {
                if (is_female(student->flags)) {
                    records[record_index].female_graduate++;
                } else {
                    records[record_index].male_graduate++;
                }
            } else { // Pregrado
                if (is_female(student->flags)) {
                    records[record_index].female_undergrad++;
                } else {
                    records[record_index].male_undergrad++;
                }
            }
        }
    }

    // Imprimir los resultados en formato de tabla
    printf("\n%-10s %-10s %-20s %-20s %-20s %-20s\n", "Año", "Semestre", "Hombres Pregrado", "Mujeres Pregrado", "Hombres Posgrado", "Mujeres Posgrado");
    printf("--------------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < record_count; i++) {
        printf("%-10u %-10u %-20d %-20d %-20d %-20d\n", 
               records[i].year, 
               records[i].semester, 
               records[i].male_undergrad, 
               records[i].female_undergrad, 
               records[i].male_graduate, 
               records[i].female_graduate);
    }

    // Liberar memoria y cerrar el archivo
    free(students);
    free(enrollments);
    free(records);
    fclose(file);

    return 0;
}
