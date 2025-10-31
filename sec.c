#include <stdio.h>
#include <stdlib.h> // Necesario para malloc y free
#include <string.h> // Necesario para memcpy
#include <math.h>   // Necesario para sqrt
#include <sys/time.h> 

// 1. Definiciones de Dimensiones (constantes)
#define M_FILAS_A 16
#define K_COMUN 8
#define N_COLUMNAS_B 16

// Definición auxiliar para cálculos de tamaño (SAFE_ARRAYSIZE)
// La versión original puede ser insegura en contextos de funciones.
#define SAFE_ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

// La matriz de impresión ahora debe aceptar un puntero a arreglo
// para manejar el resultado C y las submatrices dinámicas.
void imprimirMatriz(int rows, int cols, int mat[rows][cols]) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d\t", mat[i][j]);
        }
        printf("\n");
    }
}

int main() {

    struct timeval inicio, fin;
    double tiempo;
    gettimeofday(&inicio, NULL); // ⏱️ Comienza medición

    // Dimensiones
    int M = M_FILAS_A;
    int K = K_COMUN;
    int N = N_COLUMNAS_B;
    
    // Nodos y Particiones
    int n = 4;
    // La raíz cuadrada de 4 es 2.
    int n2 = (int)sqrt((double)n); 
    
    // Dimensiones de A y B
    int filasA = M; 
    int filasB = K;
    int columnB = N;
    int columnA = K; // La dimensión interna para la multiplicación

    // ----------------------------------------------------
    // 2. Declaración del Vector de Submatrices (Punteros)
    // El vector ahora contendrá punteros a bloques de memoria dinámicos.
    // int (*vector[n2])[columnB];  // Vector de punteros a arreglos de 'columnB' int's
    // Nota: Como 'matriz' será de 'filaMatriz x columnB', es más seguro usar un puntero genérico.

    // VECTOR A FILAS DE A
    // VECTOR B COLUMNAS DE B
    int **vectorA = malloc(n * sizeof(int*)); // Arreglo de n2 punteros a int
    if (vectorA == NULL) return 1;

    int **vectorB = malloc(n * sizeof(int*)); // Arreglo de n2 punteros a int
    if (vectorB == NULL) return 1;

    // ----------------------------------------------------
    // 3. Inicialización de Matrices Estáticas
    
    // Matriz A (16x8) con todos los elementos = 1
    int matrizA[M][K];
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < K; j++) {
            matrizA[i][j] = 1;
        }
    }

    // Matriz B (8x16) con todos los elementos = 1
    int matrizB[K][N];
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < N; j++) {
            matrizB[i][j] = 5;
        }
    }
    
    // Matriz C (16x16)
    int matrizC[filasA][columnB]; 

    // ----------------------------------------------------
    // 4. División de Matriz A (Usando Malloc y Memcpy)

    int filaMatrizA = filasA / n2; // 16 / 2 = 8 filas por submatriz
    int I_inicio = 0;
    

    for (int J = 0; J < n2; J++){ // Bucle para las 2 particiones (J=0, J=1)

        vectorA[J] = (int *)malloc(filaMatrizA * columnA * sizeof(int));
        if (vectorA[J] == NULL) return 1;
        
        // Copia de los datos desde matrizA al bloque dinámico vector[J]
        // &matrizA[I_inicio][0] es la dirección del primer elemento de la submatriz.
        memcpy(vectorA[J], 
               &matrizA[I_inicio][0], 
               filaMatrizA * columnA * sizeof(int));

        // Actualización del índice de inicio para la siguiente partición
        I_inicio += filaMatrizA;
    }

    int columnaMatrizB = columnB / n2; // 16 / 2 = 8 filas por submatriz
    int I_inicio_columna = 0;
    
    for (int K = 0; K < n2; K++){ // Bucle para las 2 particiones (J=0, J=1)

        vectorB[K] = (int *)malloc(filasB * columnaMatrizB * sizeof(int));
        if (vectorB[K] == NULL) return 1;
        
        // Copia de los datos desde matrizA al bloque dinámico vector[J]
        // &matrizA[I_inicio][0] es la dirección del primer elemento de la submatriz.
        for (int i = 0; i < filasB; i++) {
        memcpy(
            &vectorB[K][i * columnaMatrizB],           // destino (fila i del bloque)
            &matrizB[i][I_inicio_columna],            // origen (fila i, columna inicial)
            columnaMatrizB * sizeof(int)              // cantidad de columnas a copiar
        );
}

        // Actualización del índice de inicio para la siguiente partición
        I_inicio_columna += columnaMatrizB;
    }
    

    for (int V =0; V<n2; V++){
        for (int P=0; P<n2; P++){
            for (int I = 0; I < filaMatrizA; I++){
                for (int J = 0; J < columnaMatrizB; J++){
                    matrizC[I + V*(filaMatrizA)][J + P*(columnaMatrizB)] = vectorA[V][I + columnA*J]*vectorB[P][I + columnA*J]; 
                }
            }
        }
    }

    // printf("%i",vectorA[bloque][fila + cantColumnas*columna]) * vectorA[bloque][fila + cantColumnas*columna]);

    imprimirMatriz(16,16,matrizC);

    // Liberación de memoria dinámica (es crucial)
    for (int J = 0; J < n2; J++) {
        free(vectorA[J]);
        free(vectorB[J]);
    }
    free(vectorA);
    free(vectorB);

    gettimeofday(&fin, NULL); // ⏱️ Termina medición
    tiempo = (fin.tv_sec - inicio.tv_sec) + (fin.tv_usec - inicio.tv_usec) / 1e6;
    printf("Tiempo real: %.6f segundos\n", tiempo);
    
    return 0;
}