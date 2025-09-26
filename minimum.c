/* minimum.c */

#include <pthread.h>           
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 100000000
#define BLOCK_SIZE 2048
#define NB_EXECUTIONS 10

double *A, *B, *C;

// Stratégies de répartition
typedef enum {
    STRAT_CYCLIQUE,
    STRAT_CYCLIQUE_BLOC,
    STRAT_FARMING
} Strat;

// Structure pour passer les arguments aux threads
typedef struct {
    int id;
    int nb_threads;
    int methode;
    int *blocs_traites;
} ThreadArgs;

int index_global = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Fonction exécutée par les threads
void *calcul_min(void *args) {
    ThreadArgs *arg = (ThreadArgs *) args;
    int id = arg->id;
    int nb_threads = arg->nb_threads;
    int methode = arg->methode;
    int blocs_count = 0;

    if (methode == STRAT_CYCLIQUE) {
        for (int i = id; i < SIZE; i += nb_threads) {
            if (A[i] < B[i]) {
                C[i] = A[i];
            } else {
                C[i] = B[i];
            }
        }
    } else if (methode == STRAT_CYCLIQUE_BLOC) {
        for (int i = id * BLOCK_SIZE; i < SIZE; i += nb_threads * BLOCK_SIZE) {
            for (int j = 0; j < BLOCK_SIZE && (i + j) < SIZE; j++) {
                if (A[i + j] < B[i + j]) {
                    C[i + j] = A[i + j];
                } else {
                    C[i + j] = B[i + j];
                }
            }
        }
    } else if (methode == STRAT_FARMING) {
        int i;
        while (1) {
            pthread_mutex_lock(&mutex);
            i = index_global;
            index_global += BLOCK_SIZE;
            pthread_mutex_unlock(&mutex);

            if (i >= SIZE) break;

            for (int j = 0; j < BLOCK_SIZE && (i + j) < SIZE; j++) {
                if (A[i + j] < B[i + j]) {
                    C[i + j] = A[i + j];
                } else {
                    C[i + j] = B[i + j];
                }
            }
            blocs_count++;
        }
        arg->blocs_traites[id] = blocs_count;
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // Vérification du nombre d'arguments
    if (argc != 4) {
        printf("Usage: %s <methode_repartition> <nb_threads> <migratio>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int methode = atoi(argv[1]);
    int nb_threads = atoi(argv[2]);
    int migration = atoi(argv[3]);

    // Vérification des valeurs
    if (nb_threads < 1 || nb_threads > 1024) {
        printf("Erreur : nombre de threads invalide (doit être entre 1 et 1024).\n");
        return EXIT_FAILURE;
    }

    if (methode < 0 || methode > 2) {
        printf("Erreur : méthode de répartition invalide (0: cyclique, 1: bloc, 2: farming).\n");
        return EXIT_FAILURE;
    }

    // Allocation mémoire
    A = (double *)malloc(SIZE * sizeof(double));
    B = (double *)malloc(SIZE * sizeof(double));
    C = (double *)malloc(SIZE * sizeof(double));

    if(A == NULL || B == NULL || C == NULL) {
        printf("Erreur d'allocation mémoire.\n");
        return EXIT_FAILURE;
    }

    // Initialisation des tableaux avec des valeurs aléatoires
    srand(time(NULL));
    for (int i = 0; i < SIZE; i++) {
        A[i] = (double)(rand() % 1000);
        B[i] = (double)(rand() % 1000);
    }

    // Déclaration des threads
    pthread_t threads[nb_threads];
    ThreadArgs args[nb_threads];

    // Tableau pour stocker les temps d'exécution
    double total_time = 0.0;

    for (int exec = 0; exec < NB_EXECUTIONS; exec++) {
        index_global = 0; 

        // Mesure du temps d'exécution
        struct timeval start, end;
        gettimeofday(&start, NULL);

        // Initialisation des arguments et création des threads
        int blocs_traites[nb_threads];
        for (int i = 0; i < nb_threads; i++) {
            args[i].id = i;
            args[i].nb_threads = nb_threads;
            args[i].methode = methode;
            args[i].blocs_traites = blocs_traites;
            blocs_traites[i] = 0;

            if (pthread_create(&threads[i], NULL, calcul_min, &args[i]) != 0) {
                printf("Erreur lors de la création du thread %d\n", i);
                return EXIT_FAILURE;
            }
        }

        // Attente des threads
        for (int i = 0; i < nb_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        gettimeofday(&end, NULL);
        double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
        time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
        total_time += time_taken;
    }

    // Calcul du temps moyen
    double avg_time = total_time / NB_EXECUTIONS;

    printf("methode,threads,migration,temps_moyen\n");
    printf("%d,%d,%d,%f\n", methode, nb_threads, migration, avg_time);

    // Affichage des statistiques pour farming
    if (methode == STRAT_FARMING) {
        int min_blocs = SIZE, max_blocs = 0;
        for (int i = 0; i < nb_threads; i++) {
            if (args[i].blocs_traites[i] < min_blocs) min_blocs = args[i].blocs_traites[i];
            if (args[i].blocs_traites[i] > max_blocs) max_blocs = args[i].blocs_traites[i];
        }
        printf("farming_stats,%d,%d,%d,%f,%d,%d\n", methode, nb_threads, migration, avg_time, min_blocs, max_blocs);
    }

    // Libération de la mémoire
    free(A);
    free(B);
    free(C);

    return 0;
}
