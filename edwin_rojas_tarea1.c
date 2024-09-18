#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MAGIC_STRING 0xaaae
#define NAME_SIZE 23

// Estructura del estudiante
typedef struct {
    uint32_t id;
    uint8_t flags;    // Banderas (género y posgrado)
    char name[NAME_SIZE + 1]; // Nombre del estudiante, con espacio para '\0'
    uint32_t age;     // Edad del estudiante
} Student;

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

// Función para imprimir el género basado en la bandera F
const char *get_gender(uint8_t flags) {
    return (flags & 0x80) ? "Femenino" : "Masculino";
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: %s <archivo_binario> <edad_minima> <edad_maxima>\n", argv[0]);
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
   

    // Leer la cantidad de estudiantes
    uint32_t student_count = read_little_endian_32(file);
    

    // Leer cantidad de cursos y matrículas
    uint32_t course_count = read_little_endian_32(file);
    uint32_t enrollment_count = read_little_endian_32(file);

   

    // Obtener el rango de edades desde los argumentos
    int min_age = atoi(argv[2]);
    int max_age = atoi(argv[3]);

    printf("\n%-25s %-10s %-10s\n", "Nombre", "Edad", "Género");
    printf("---------------------------------------------------------\n");

    // Leer y filtrar los estudiantes
    for (uint32_t i = 0; i < student_count; i++) {
        Student student;

        // Leer los datos del estudiante
        student.id = read_little_endian_32(file);
        fread(&student.flags, sizeof(uint8_t), 1, file);
        fread(student.name, sizeof(char), NAME_SIZE, file);
        student.name[NAME_SIZE] = '\0'; // Asegurar que el nombre está terminado en '\0'
        student.age = read_little_endian_32(file);
        

        // Verificar si el estudiante está dentro del rango de edades
        if (student.age >= min_age && student.age <= max_age) {
            printf(" %-25s %-10d %-10s\n", student.name, student.age, get_gender(student.flags));
        }
    }

    // Cerrar archivo
    fclose(file);

    return 0;
}
