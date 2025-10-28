#include <stdio.h>
#include <stdlib.h> // Necesario para malloc y free
#include <string.h> // Necesario para memcpy
#include <math.h>   // Necesario para sqrt

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
    printf("Imprimiendo matriz %dx%d:\n", rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d\t", mat[i][j]);
        }
        printf("\n");
    }
}

int main() {
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
    int **vector = malloc(n * sizeof(int*)); // Arreglo de n2 punteros a int
    if (vector == NULL) return 1;

    // ----------------------------------------------------
    // 3. Inicialización de Matrices Estáticas
    
    // Matriz A (16x8) con todos los elementos = 1
    int matrizA[M][K];
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < K; j++) {
            matrizA[i][j] = 10;
        }
    }

    // Matriz B (8x16) con todos los elementos = 1
    int matrizB[K][N];
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < N; j++) {
            matrizB[i][j] = 2;
        }
    }
    
    // Matriz C (16x16)
    int matrizC[M][N];

    // ----------------------------------------------------
    // 4. División de Matriz A (Usando Malloc y Memcpy)

    int filaMatriz = filasA / n2; // 16 / 2 = 8 filas por submatriz
    int I_inicio = 0;
    
    printf("Dividiendo matrizA (%dx%d) en %d bloques de %dx%d...\n", M, K, n2, filaMatriz, columnA);

    for (int J = 0; J < n2; J++){ // Bucle para las 2 particiones (J=0, J=1)

        vector[J] = (int *)malloc(filaMatriz * columnA * sizeof(int));
        if (vector[J] == NULL) return 1;
        
        // Copia de los datos desde matrizA al bloque dinámico vector[J]
        // &matrizA[I_inicio][0] es la dirección del primer elemento de la submatriz.
        memcpy(vector[J], 
               &matrizA[I_inicio][0], 
               filaMatriz * columnA * sizeof(int));

        // Actualización del índice de inicio para la siguiente partición
        I_inicio += filaMatriz;
    }

    int filaMatrizB = columnB / n2; // 16 / 2 = 8 filas por submatriz
    int I_inicioB = 0;
    
    printf("Dividiendo matriz (%dx%d) en %d bloques de %dx%d...\n", M, K, n2, filaMatriz, columnA);

    for (int J = 0; J < n2; J++){ // Bucle para las 2 particiones (J=0, J=1)

        vector[J] = (int *)malloc(filaMatrizB * filasB * sizeof(int));
        if (vector[J] == NULL) return 1;
        
        // Copia de los datos desde matrizA al bloque dinámico vector[J]
        // &matrizA[I_inicio][0] es la dirección del primer elemento de la submatriz.
        memcpy(vector[J], 
               &matrizB[I_inicioB][0], 
               filaMatrizB * columnB * sizeof(int));

        // Actualización del índice de inicio para la siguiente partición
        I_inicioB += filaMatrizB;
    }


    // int i, j, k_idx; 

    // for (i = 0; i < M; i++) { 
    //     for (j = 0; j < N; j++) { 
    //         matrizC[i][j] = 0; 
    //         for (k_idx = 0; k_idx < K; k_idx++) { // La dimensión interna es K=8
    //             // C[i][j] = A[i][k] * B[k][j]
    //             matrizC[i][j] += matrizA[i][k_idx] * matrizB[k_idx][j];
    //         }
    //     }
    // }
    
    imprimirMatriz(8,8,vector[0]);
    imprimirMatriz(8,8,vector[1]);
    imprimirMatriz(8,8,vector[2]);
    imprimirMatriz(8,8,vector[]);

    // Liberación de memoria dinámica (es crucial)
    for (int J = 0; J < n2; J++) {
        free(vector[J]);
    }
    free(vector);
    
    return 0;
}