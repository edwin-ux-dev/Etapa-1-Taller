#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define NAME_SIZE 23

typedef struct {
    uint32_t id;
    uint8_t flags;    
    char name[NAME_SIZE + 1]; 
    uint32_t age;     
} Student;

uint16_t read_big_endian_16(FILE *file) {
    uint8_t bytes[2];
    fread(bytes, sizeof(uint8_t), 2, file);
    return (bytes[0] << 8) | bytes[1];
}

uint32_t read_little_endian_32(FILE *file) {
    uint8_t bytes[4];
    fread(bytes, sizeof(uint8_t), 4, file);
    return (uint32_t)(bytes[0]) | (uint32_t)(bytes[1] << 8) | (uint32_t)(bytes[2] << 16) | (uint32_t)(bytes[3] << 24);
}

const char *get_gender(uint8_t flags) {
    return (flags & 0x80) ? "Femenino" : "Masculino";
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: %s <archivo_binario> <edad_minima> <edad_maxima>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error al abrir el archivo");
        return 1;
    }

    read_big_endian_16(file);  // Leer y descartar la cadena mágica
    uint32_t student_count = read_little_endian_32(file);
    read_little_endian_32(file); // Leer y descartar cantidad de cursos
    read_little_endian_32(file); // Leer y descartar cantidad de matrículas

    int min_age = atoi(argv[2]);
    int max_age = atoi(argv[3]);

    printf("\n%-25s %-10s %-10s\n", "Nombre", "Edad", "Género");
    printf("---------------------------------------------------------\n");

    for (uint32_t i = 0; i < student_count; i++) {
        Student student;
        fread(&student.id, sizeof(uint32_t), 1, file);
        fread(&student.flags, sizeof(uint8_t), 1, file);
        fread(student.name, sizeof(char), NAME_SIZE, file);
        student.name[NAME_SIZE] = '\0';
        student.age = read_little_endian_32(file);

        if (student.age >= min_age && student.age <= max_age) {
            printf(" %-25s %-10d %-10s\n", student.name, student.age, get_gender(student.flags));
        }
    }

    fclose(file);
    return 0;
}
