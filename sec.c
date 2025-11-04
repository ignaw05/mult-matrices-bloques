#include <stdio.h>
#include <stdlib.h> // Necesario para malloc y free
#include <string.h> // Necesario para memcpy
#include <math.h>   // Necesario para sqrt
#include <sys/time.h> 

// 1. Definiciones de Dimensiones (constantes)
#define M_FILAS_A 6144
#define K_COMUN 6144
#define N_COLUMNAS_B 6144
// Definici�n auxiliar para c�lculos de tama�o (SAFE_ARRAYSIZE)
// La versi�n original puede ser insegura en contextos de funciones.
#define SAFE_ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

// La matriz de impresi�n ahora debe aceptar un puntero a arreglo
// para manejar el resultado C y las submatrices din�micas.
void imprimirMatriz(int rows, int cols, int *mat) {
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			printf("%d\t", mat[i * cols + j]);
		}
		printf("\n");
	}
}

int main() {
	
	struct timeval inicio, fin;
	double tiempo;
	gettimeofday(&inicio, NULL); // ?? Comienza medici�n
	
	// Dimensiones
	int M = M_FILAS_A;
	int K = K_COMUN;
	int N = N_COLUMNAS_B;
	
	// Nodos y Particiones
	int n = 4;
	// La ra�z cuadrada de 4 es 2.
	int n2 = (int)sqrt((double)n); 
	
	// Dimensiones de A y B
	int filasA = M; 
	int filasB = K;
	int columnB = N;
	int columnA = K; // La dimensi�n interna para la multiplicaci�n
	
	// ----------------------------------------------------
	// 2. Declaraci�n del Vector de Submatrices (Punteros)
	// El vector ahora contendr� punteros a bloques de memoria din�micos.
	// int (*vector[n2])[columnB];  // Vector de punteros a arreglos de 'columnB' int's
	// Nota: Como 'matriz' ser� de 'filaMatriz x columnB', es m�s seguro usar un puntero gen�rico.
	
	// VECTOR A FILAS DE A
	// VECTOR B COLUMNAS DE B
	int **vectorA = malloc(n * sizeof(int*)); // Arreglo de n2 punteros a int
	if (vectorA == NULL) return 1;
	
	int **vectorB = malloc(n * sizeof(int*)); // Arreglo de n2 punteros a int
	if (vectorB == NULL) return 1;
	
	// ----------------------------------------------------
	// 3. Inicializaci�n de Matrices Est�ticas (ahora din�micas)
	
	// Matriz A (16x8) con todos los elementos = 1
	int *matrizA = malloc(M * K * sizeof(int));
	int *matrizB = malloc(K * N * sizeof(int));
	int *matrizC = malloc(M * N * sizeof(int));
	if (matrizA == NULL || matrizB == NULL || matrizC == NULL) {
		printf("Error al asignar memoria.\n");
		return 1;
	}
	
	for (int i = 0; i < M; i++) {
		for (int j = 0; j < K; j++) {
			matrizA[i * K + j] = 1;
		}
	}
	
	// Matriz B (8x16) con todos los elementos = 1
	for (int i = 0; i < K; i++) {
		for (int j = 0; j < N; j++) {
			matrizB[i * N + j] = 5;
		}
	}
	
	// Matriz C (16x16)
	for (int i = 0; i < M; i++) {
		for (int j = 0; j < N; j++) {
			matrizC[i * N + j] = 0;
		}
	}
	
	// ----------------------------------------------------
	// 4. Divisi�n de Matriz A (Usando Malloc y Memcpy)
	
	int filaMatrizA = filasA / n2; // 16 / 2 = 8 filas por submatriz
	int I_inicio = 0;
	
	for (int J = 0; J < n2; J++){ // Bucle para las 2 particiones (J=0, J=1)
		
		vectorA[J] = (int *)malloc(filaMatrizA * columnA * sizeof(int));
		if (vectorA[J] == NULL) return 1;
		
		// Copia de los datos desde matrizA al bloque din�mico vector[J]
		memcpy(vectorA[J], 
			   &matrizA[I_inicio * columnA], 
			   filaMatrizA * columnA * sizeof(int));
		
		// Actualizaci�n del �ndice de inicio para la siguiente partici�n
		I_inicio += filaMatrizA;
	}
	
	int columnaMatrizB = columnB / n2; // 16 / 2 = 8 filas por submatriz
	int I_inicio_columna = 0;
	
	for (int Kb = 0; Kb < n2; Kb++){ // Bucle para las 2 particiones (J=0, J=1)
		
		vectorB[Kb] = (int *)malloc(filasB * columnaMatrizB * sizeof(int));
		if (vectorB[Kb] == NULL) return 1;
		
		// Copia de los datos desde matrizB al bloque din�mico vector[J]
		for (int i = 0; i < filasB; i++) {
			memcpy(
				   &vectorB[Kb][i * columnaMatrizB],           // destino (fila i del bloque)
					   &matrizB[i * N + I_inicio_columna],         // origen (fila i, columna inicial)
					   columnaMatrizB * sizeof(int)                // cantidad de columnas a copiar
					   );
		}
		
		// Actualizaci�n del �ndice de inicio para la siguiente partici�n
		I_inicio_columna += columnaMatrizB;
	}
	
	
	for (int V =0; V<n2; V++){
		for (int P=0; P<n2; P++){
			for (int I = 0; I < filaMatrizA; I++){
				for (int J = 0; J < columnaMatrizB; J++){
					matrizC[(I + V*(filaMatrizA)) * N + (J + P*(columnaMatrizB))] =
						vectorA[V][I * columnA + J] * vectorB[P][I * columnA + J]; 
				}
			}
		}
	}
	
	// printf("%i",vectorA[bloque][fila + cantColumnas*columna]) * vectorA[bloque][fila + cantColumnas*columna]);
	
	imprimirMatriz(16,16,matrizC);
	
	// Liberaci�n de memoria din�mica (es crucial)
	for (int J = 0; J < n2; J++) {
		free(vectorA[J]);
		free(vectorB[J]);
	}
	free(vectorA);
	free(vectorB);
	free(matrizA);
	free(matrizB);
	free(matrizC);
	
	gettimeofday(&fin, NULL); // ?? Termina medici�n
	tiempo = (fin.tv_sec - inicio.tv_sec) + (fin.tv_usec - inicio.tv_usec) / 1e6;
	printf("Tiempo real: %.6f segundos\n", tiempo);
	
	return 0;
}
