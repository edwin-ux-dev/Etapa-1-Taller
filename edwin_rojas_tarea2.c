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
    char name[NAME_SIZE + 1]; // Nombre del estudiante, con espacio para '\0'
    uint32_t age;     // Edad del estudiante
} Student;

// Estructura del curso
typedef struct {
    uint32_t id;
    char name[COURSE_NAME_SIZE + 1]; // Nombre del curso
    uint32_t credit_hours; // Horas de crédito del curso
} Course;

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
    uint8_t bytes[4];
    fread(bytes, sizeof(uint8_t), 4, file);
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

// Función para liberar memoria y cerrar el archivo
void cleanup(FILE *file, Student *students, Course *courses, Enrollment *enrollments) {
    if (file) fclose(file);
    if (students) free(students);
    if (courses) free(courses);
    if (enrollments) free(enrollments);
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

    // Verificar cadena mágica
    uint16_t magic = read_big_endian_16(file);
    if (magic != MAGIC_STRING) {
        printf("Archivo binario no válido. Se esperaba la cadena mágica: 0xaaae, pero se leyó: 0x%04x\n", magic);
        cleanup(file, NULL, NULL, NULL);
        return 1;
    }

    // Leer cantidad de estudiantes, cursos y matrículas
    uint32_t student_count = read_little_endian_32(file);
    uint32_t course_count = read_little_endian_32(file);
    uint32_t enrollment_count = read_little_endian_32(file);

    // Reservar memoria
    Student *students = malloc(student_count * sizeof(Student));
    Course *courses = malloc(course_count * sizeof(Course));
    Enrollment *enrollments = malloc(enrollment_count * sizeof(Enrollment));

    if (!students || !courses || !enrollments) {
        printf("Error al asignar memoria.\n");
        cleanup(file, students, courses, enrollments);
        return 1;
    }

    // Leer estudiantes
    for (uint32_t i = 0; i < student_count; i++) {
        students[i].id = read_little_endian_32(file);
        fread(&students[i].flags, sizeof(uint8_t), 1, file);
        fread(students[i].name, sizeof(char), NAME_SIZE, file);
        students[i].name[NAME_SIZE] = '\0'; // Asegurar terminación de cadena
        students[i].age = read_little_endian_32(file);
    }

    // Leer cursos
    for (uint32_t i = 0; i < course_count; i++) {
        courses[i].id = read_little_endian_32(file);
        fread(courses[i].name, sizeof(char), COURSE_NAME_SIZE, file);
        courses[i].name[COURSE_NAME_SIZE] = '\0'; // Asegurar terminación de cadena
        courses[i].credit_hours = read_little_endian_32(file);
    }

    // Leer matrículas
    for (uint32_t i = 0; i < enrollment_count; i++) {
        enrollments[i].student_id = read_little_endian_32(file);
        enrollments[i].course_id = read_little_endian_32(file);
        enrollments[i].year = read_little_endian_32(file);
        enrollments[i].semester = read_little_endian_32(file);
    }

    // Crear un mapa para encontrar estudiantes por ID rápidamente
    uint32_t *student_age_map = malloc(student_count * sizeof(uint32_t));
    if (!student_age_map) {
        printf("Error al asignar memoria para el mapa de estudiantes.\n");
        cleanup(file, students, courses, enrollments);
        return 1;
    }
    for (uint32_t i = 0; i < student_count; i++) {
        student_age_map[students[i].id] = students[i].age;
    }

    // Imprimir encabezado de la tabla
    printf("\n%-35s %-15s\n", "Nombre del Curso", "Edad Promedio");
    printf("-----------------------------------------------------------\n");

    // Calcular la edad promedio de los estudiantes inscritos en cada curso
    for (uint32_t i = 0; i < course_count; i++) {
        uint32_t course_id = courses[i].id;
        int total_age = 0;
        int student_count_in_course = 0;

        // Recorrer matrículas y contar edades de los estudiantes por curso
        for (uint32_t j = 0; j < enrollment_count; j++) {
            if (enrollments[j].course_id == course_id) {
                uint32_t student_id = enrollments[j].student_id;
                total_age += student_age_map[student_id];
                student_count_in_course++;
            }
        }

        if (student_count_in_course > 0) {
            double average_age = (double)total_age / student_count_in_course;
            printf("%-35s %-15.2f\n", courses[i].name, average_age);
        } else {
            printf("%-35s %-15s\n", courses[i].name, "No hay estudiantes");
        }
    }

    // Liberar memoria y cerrar el archivo
    free(student_age_map);
    cleanup(file, students, courses, enrollments);

    return 0;
}
