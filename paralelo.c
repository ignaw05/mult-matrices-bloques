#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#define M_FILAS_A      1024
#define K_COMUN        1024
#define N_COLUMNAS_B   1024

static void imprimirMatrizRecorte(const char* nombre, int M, int N, int* C) {
    int maxR = (M < 16) ? M : 16;
    int maxC = (N < 16) ? N : 16;
    printf("\n== %s (recorte %dx%d) ==\n", nombre, maxR, maxC);
    for (int i = 0; i < maxR; ++i) {
        for (int j = 0; j < maxC; ++j) {
            printf("%d\t", C[i * N + j]);
        }
        printf("\n");
    }
}

int main(int argc, char** argv) {
    int rank, size;
    int M = M_FILAS_A, K = K_COMUN, N = N_COLUMNAS_B;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 4) {
        if (rank == 0) {
            fprintf(stderr, "Se requieren al menos 4 procesos\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Verificar que size es un cuadrado perfecto */
    int n = (int)sqrt((double)size);
    if (n * n != size) {
        if (rank == 0) {
            fprintf(stderr, "El número de procesos debe ser un cuadrado perfecto (4, 9, 16, 25, ...)\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Calcular tamaño base de bloques (división entera) */
    int base_block_rows_A = M / n;
    int base_block_cols_B = N / n;
    int base_block_K = K / n;
    
    /* Calcular residuos */
    int remainder_M = M % n;
    int remainder_N = N % n;
    int remainder_K = K % n;

    /* Matrices globales (solo en el master) */
    int *A = NULL, *B = NULL, *C = NULL;

    /* ========== MASTER: Inicialización ========== */
    if (rank == 0) {
        A = (int*) malloc(M * K * sizeof(int));
        B = (int*) malloc(K * N * sizeof(int));
        C = (int*) calloc(M * N, sizeof(int));
        
        if (!A || !B || !C) {
            fprintf(stderr, "Master: error de memoria\n");
            MPI_Abort(MPI_COMM_WORLD, 2);
        }

        /* Inicializar con 1s */
        for (int i = 0; i < M * K; ++i) A[i] = 1;
        for (int i = 0; i < K * N; ++i) B[i] = 1;
        
        printf("=================================================\n");
        printf("Multiplicación de matrices por bloques con MPI\n");
        printf("=================================================\n");
        printf("Dimensiones: A[%d x %d] * B[%d x %d] = C[%d x %d]\n", M, K, K, N, M, N);
        printf("Procesos: %d (%dx%d grid)\n", size, n, n);
        printf("Tamaño base de bloques: [%d x %d] (residuos: M=%d, K=%d, N=%d)\n",
               base_block_rows_A, base_block_cols_B, remainder_M, remainder_K, remainder_N);
        printf("=================================================\n\n");
    }

    /* ========== INICIO DE MEDICIÓN ========== */
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();

    /* Posición del proceso en la grilla */
    int my_row = rank / n;
    int my_col = rank % n;

    /* Calcular el tamaño exacto del bloque para este proceso */
    int my_block_rows_A = base_block_rows_A + (my_row < remainder_M ? 1 : 0);
    int my_block_cols_B = base_block_cols_B + (my_col < remainder_N ? 1 : 0);
    
    /* Calcular offset de inicio para este proceso */
    int start_row_A = my_row * base_block_rows_A + (my_row < remainder_M ? my_row : remainder_M);
    int start_col_B = my_col * base_block_cols_B + (my_col < remainder_N ? my_col : remainder_N);

    /* Buffers locales */
    int *A_rows = (int*) malloc(my_block_rows_A * K * sizeof(int));
    int *B_cols = (int*) malloc(K * my_block_cols_B * sizeof(int));
    int *C_local = (int*) calloc(my_block_rows_A * my_block_cols_B, sizeof(int));

    if (!A_rows || !B_cols || !C_local) {
        fprintf(stderr, "Rank %d: error de memoria local\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 3);
    }

    /* ========== DISTRIBUCIÓN: Master envía bloques ========== */
    if (rank == 0) {
        /* Master distribuye a todos los procesos */
        for (int dest = 0; dest < size; ++dest) {
            int dest_row = dest / n;
            int dest_col = dest % n;
            
            /* Calcular tamaño del bloque para el destino */
            int dest_block_rows = base_block_rows_A + (dest_row < remainder_M ? 1 : 0);
            int dest_block_cols = base_block_cols_B + (dest_col < remainder_N ? 1 : 0);
            
            /* Calcular offset de inicio */
            int dest_start_row = dest_row * base_block_rows_A + (dest_row < remainder_M ? dest_row : remainder_M);
            int dest_start_col = dest_col * base_block_cols_B + (dest_col < remainder_N ? dest_col : remainder_N);
            
            /* Buffer temporal para el bloque de filas de A */
            int *temp_A = (int*) malloc(dest_block_rows * K * sizeof(int));
            for (int i = 0; i < dest_block_rows; ++i) {
                memcpy(&temp_A[i * K],
                       &A[(dest_start_row + i) * K],
                       K * sizeof(int));
            }
            
            /* Buffer temporal para el bloque de columnas de B */
            int *temp_B = (int*) malloc(K * dest_block_cols * sizeof(int));
            for (int k = 0; k < K; ++k) {
                memcpy(&temp_B[k * dest_block_cols],
                       &B[k * N + dest_start_col],
                       dest_block_cols * sizeof(int));
            }
            
            if (dest == 0) {
                /* Master se copia a sí mismo */
                memcpy(A_rows, temp_A, dest_block_rows * K * sizeof(int));
                memcpy(B_cols, temp_B, K * dest_block_cols * sizeof(int));
            } else {
                /* Enviar tamaños primero */
                int sizes[4] = {dest_block_rows, dest_block_cols, dest_start_row, dest_start_col};
                MPI_Send(sizes, 4, MPI_INT, dest, 0, MPI_COMM_WORLD);
                
                /* Enviar datos */
                MPI_Send(temp_A, dest_block_rows * K, MPI_INT, dest, 1, MPI_COMM_WORLD);
                MPI_Send(temp_B, K * dest_block_cols, MPI_INT, dest, 2, MPI_COMM_WORLD);
            }
            
            free(temp_A);
            free(temp_B);
        }
    } else {
        /* Procesos no-master reciben tamaños y datos */
        int sizes[4];
        MPI_Recv(sizes, 4, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        my_block_rows_A = sizes[0];
        my_block_cols_B = sizes[1];
        start_row_A = sizes[2];
        start_col_B = sizes[3];
        
        /* Realocar buffers con el tamaño correcto */
        free(A_rows);
        free(B_cols);
        free(C_local);
        
        A_rows = (int*) malloc(my_block_rows_A * K * sizeof(int));
        B_cols = (int*) malloc(K * my_block_cols_B * sizeof(int));
        C_local = (int*) calloc(my_block_rows_A * my_block_cols_B, sizeof(int));
        
        MPI_Recv(A_rows, my_block_rows_A * K, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(B_cols, K * my_block_cols_B, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    /* ========== CÓMPUTO LOCAL ========== */
    for (int i = 0; i < my_block_rows_A; ++i) {
        for (int j = 0; j < my_block_cols_B; ++j) {
            int sum = 0;
            for (int k = 0; k < K; ++k) {
                sum += A_rows[i * K + k] * B_cols[k * my_block_cols_B + j];
            }
            C_local[i * my_block_cols_B + j] = sum;
        }
    }

    /* ========== RECOLECCIÓN ========== */
    if (rank == 0) {
        /* Master copia su propio resultado */
        for (int i = 0; i < my_block_rows_A; ++i) {
            memcpy(&C[i * N], &C_local[i * my_block_cols_B], my_block_cols_B * sizeof(int));
        }
        
        /* Master recibe de todos los demás procesos */
        for (int src = 1; src < size; ++src) {
            int sizes[4];
            MPI_Recv(sizes, 4, MPI_INT, src, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            int src_block_rows = sizes[0];
            int src_block_cols = sizes[1];
            int src_start_row = sizes[2];
            int src_start_col = sizes[3];
            
            int *recv_buf = (int*) malloc(src_block_rows * src_block_cols * sizeof(int));
            MPI_Recv(recv_buf, src_block_rows * src_block_cols, MPI_INT, src, 4, 
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            /* Colocar el bloque en su posición correcta en C */
            for (int i = 0; i < src_block_rows; ++i) {
                int row_global = src_start_row + i;
                memcpy(&C[row_global * N + src_start_col],
                       &recv_buf[i * src_block_cols],
                       src_block_cols * sizeof(int));
            }
            free(recv_buf);
        }
    } else {
        /* Enviar tamaños y posición */
        int sizes[4] = {my_block_rows_A, my_block_cols_B, start_row_A, start_col_B};
        MPI_Send(sizes, 4, MPI_INT, 0, 3, MPI_COMM_WORLD);
        
        /* Enviar resultado */
        MPI_Send(C_local, my_block_rows_A * my_block_cols_B, MPI_INT, 0, 4, MPI_COMM_WORLD);
    }

    /* ========== FIN DE MEDICIÓN ========== */
    MPI_Barrier(MPI_COMM_WORLD);
    double t_end = MPI_Wtime();
    double t_local = t_end - t_start;
    double t_max;
    MPI_Reduce(&t_local, &t_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    /* ========== RESULTADOS Y VERIFICACIÓN ========== */
    if (rank == 0) {
        imprimirMatrizRecorte("C", M, N, C);
        printf("\n=================================================\n");
        printf("Tiempo total (paralelo): %.6f segundos\n", t_max);
        printf("=================================================\n");
        
        /* Verificación */
        printf("\nVerificando resultados...\n");
        int errors = 0;
        for (int i = 0; i < M && errors < 10; ++i) {
            for (int j = 0; j < N && errors < 10; ++j) {
                if (C[i * N + j] != K) {
                    printf("ERROR en C[%d,%d] = %d (esperado %d)\n", i, j, C[i*N+j], K);
                    errors++;
                }
            }
        }
        
        if (errors == 0) {
            printf("✓ Resultado CORRECTO: todos los elementos = %d\n", K);
        } else {
            printf("✗ Se encontraron %d errores\n", errors);
        }
    }

    /* Limpieza */
    free(A_rows);
    free(B_cols);
    free(C_local);
    
    if (rank == 0) {
        free(A);
        free(B);
        free(C);
    }

    MPI_Finalize();
    return 0;
}