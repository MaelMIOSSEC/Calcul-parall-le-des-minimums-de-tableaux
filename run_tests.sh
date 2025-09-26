#!/bin/bash

# Compilation du programme
gcc minimum.c -o minimum -pthread

# Vérification de la réussite de la compilation
if [ $? -ne 0 ]; then
    echo "Erreur de compilation du programme C."
    exit 1
fi

# Création du fichier CSV
RESULTS="results.csv"
echo "methode,threads,migration,temps_moyen" > $RESULTS

# Paramètres à tester
METHODS=(0 1 2)                      # 0 = cyclique, 1 = bloc, 2 = farming
THREADS=(1 2 4 8 16 32 64 128 256 512 1024)  # Puissances de 2
MIGRATION=(0 1)                      # 0 = non, 1 = oui

# Boucle pour tester toutes les combinaisons
for methode in "${METHODS[@]}"; do
    for thread in "${THREADS[@]}"; do
        for migration in "${MIGRATION[@]}"; do
            echo "Exécution : Méthode=$methode, Threads=$thread, Migration=$migration"

            # Démarrage du chrono
            START=$(date +%s)

            # Exécution du programme
            output=$(./minimum $methode $thread $migration)

            # Fin du chrono
            END=$(date +%s)

            # Extraction du temps moyen
            time_avg=$(echo "$output" | grep -Eo '[0-9]+\.[0-9]+' | tail -n 1)

            # Stockage des résultats dans le fichier CSV (sans espaces)
            echo "$methode,$thread,$migration,$time_avg" >> $RESULTS
        done
    done
done

echo "Tests terminés ! Résultats sauvegardés dans $RESULTS"
