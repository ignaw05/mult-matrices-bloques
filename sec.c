#include <stdio.h>
#include <stdlib.h> // Necesario para malloc y free
#include <string.h> // Necesario para memcpy
#include <math.h>   // Necesario para sqrt
#include <sys/time.h> 

// 1. Definiciones de Dimensiones (constantes)
#define M_FILAS_A 1024
#define K_COMUN 512
#define N_COLUMNAS_B 1024
#define SAFE_ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

void imprimirMatriz(int rows, int cols, int *mat) {
	int i, j;
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			printf("%d\t", mat[i * cols + j]);
		}
		printf("\n");
	}
}

int main() {
	struct timeval inicio, fin;
	double tiempo;

	int M = M_FILAS_A;
	int K = K_COMUN;
	int N = N_COLUMNAS_B;
	int n = 4;
	int n2 = (int)sqrt((double)n); 
	int filasA = M; 
	int filasB = K;
	int columnB = N;
	int columnA = K; 
	int i, j, J, Kb, V, P, I, J2, k;

	int **vectorA = malloc(n * sizeof(int*));
	if (vectorA == NULL) return 1;
	
	int **vectorB = malloc(n * sizeof(int*));
	if (vectorB == NULL) return 1;
	
	int *matrizA = malloc(M * K * sizeof(int));
	int *matrizB = malloc(K * N * sizeof(int));
	int *matrizC = malloc(M * N * sizeof(int));
	if (matrizA == NULL || matrizB == NULL || matrizC == NULL) {
		printf("Error al asignar memoria.\n");
		return 1;
	}
	
	for (i = 0; i < M; i++) {
		for (j = 0; j < K; j++) {
			matrizA[i * K + j] = 1;
		}
	}
	
	for (i = 0; i < K; i++) {
		for (j = 0; j < N; j++) {
			matrizB[i * N + j] = 1;
		}
	}
	
	for (i = 0; i < M; i++) {
		for (j = 0; j < N; j++) {
			matrizC[i * N + j] = 0;
		}
	}
	
	gettimeofday(&inicio, NULL);
	int filaMatrizA = filasA / n2;
	int I_inicio = 0;
	
	for (J = 0; J < n2; J++){
		vectorA[J] = (int *)malloc(filaMatrizA * columnA * sizeof(int));
		if (vectorA[J] == NULL) return 1;
		memcpy(vectorA[J], &matrizA[I_inicio * columnA], filaMatrizA * columnA * sizeof(int));
		I_inicio += filaMatrizA;
	}
	
	int columnaMatrizB = columnB / n2;
	int I_inicio_columna = 0;
	
	for (Kb = 0; Kb < n2; Kb++){
		vectorB[Kb] = (int *)malloc(filasB * columnaMatrizB * sizeof(int));
		if (vectorB[Kb] == NULL) return 1;
		
		for (i = 0; i < filasB; i++) {
			memcpy(&vectorB[Kb][i * columnaMatrizB],
				   &matrizB[i * N + I_inicio_columna],
				   columnaMatrizB * sizeof(int));
		}
		I_inicio_columna += columnaMatrizB;
	}
	
	for (V = 0; V < n2; V++) {
		for (P = 0; P < n2; P++) {
			for (I = 0; I < filaMatrizA; I++) {
				for (J2 = 0; J2 < columnaMatrizB; J2++) {
					int suma = 0;
					for (k = 0; k < columnA; k++) {
						suma += vectorA[V][I * columnA + k] * vectorB[P][k * columnaMatrizB + J2];
					}
					matrizC[(I + V * filaMatrizA) * N + (J2 + P * columnaMatrizB)] = suma;
				}
			}
		}
	}
	gettimeofday(&fin, NULL);
	tiempo = (fin.tv_sec - inicio.tv_sec) + (fin.tv_usec - inicio.tv_usec) / 1e6;
	imprimirMatriz(64, 64, matrizC);
	
	for (J = 0; J < n2; J++) {
		free(vectorA[J]);
		free(vectorB[J]);
	}
	free(vectorA);
	free(vectorB);
	free(matrizA);
	free(matrizB);
	free(matrizC);
	

	printf("Tiempo real: %.6f segundos\n", tiempo);
	
	return 0;
}
