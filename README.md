# Bataille navale multijoueurs (SEA)

Jeu de bataille navale en console pour 2 à 4 joueurs, écrit en C11 avec les threads POSIX. Chaque joueur possède une grille 10 × 10 et une flotte de cinq navires (longueurs 5, 4, 3, 3 et 2). Les navires sont placés automatiquement, sans chevauchement. Le dernier joueur dont la flotte est encore à flot gagne.

## Prérequis

Ubuntu Linux, GCC, `make` et la bibliothèque POSIX threads fournie avec le système. Aucune bibliothèque externe n'est nécessaire.

## Compiler et lancer

Depuis le dossier du projet :

```sh
make
./battleship             # 2 joueurs, graine aléatoire
./battleship 4           # 4 joueurs
./battleship 3 42        # 3 joueurs, graine fixe pour reproduire la partie
make test                # tests du moteur et tests d'intégration
make clean               # supprimer les exécutables générés
```

La démonstration automatique termine une partie sans saisie :

```sh
./battleship --demo 4 42
```

Elle lit les positions cachées pour choisir ses tirs. Elle sert uniquement à vérifier rapidement une partie complète et **ne représente pas le comportement des joueurs humains**.

## Manuel de jeu

Au début de son tour, le joueur voit sa propre grille et les tirs déjà connus sur les adversaires vivants. Les coordonnées et numéros de joueur affichés commencent à 1. Entrez le numéro de l'adversaire, la ligne, puis la colonne :

```text
Entrez cible ligne colonne (ex. 2 4 7) : 2 4 7
Joueur 1 -> joueur 2, case (4,7) : touché !
```

`S` désigne un segment de votre navire, `X` une case touchée, `o` un tir raté et `.` une case inconnue. Les navires ennemis non touchés restent cachés. Un tir hors grille, sur soi-même, sur un joueur éliminé ou sur une case déjà visée est refusé sans perdre le tour. Un tir accepté fait passer le tour au prochain joueur vivant. `Ctrl-D` arrête proprement la partie.

La console est partagée : les joueurs doivent se passer le terminal à chaque tour. Elle ne peut pas garantir que les autres personnes ne voient pas la grille personnelle affichée.

## Threads et synchronisation

Le thread principal est le seul à lire `stdin` et à écrire les messages. Chaque joueur a un worker POSIX : il valide ses tirs sortants et traite les tirs entrants contre sa propre grille. Un seul mutex protège les grilles, les boîtes de commandes, le tour, le résultat et l'état de fin. Les workers dorment sur des variables de condition lorsqu'ils n'ont rien à faire. Les transitions de tir et de victoire sont atomiques sous le mutex. À la fin, le programme réveille et joint tous les workers.

## Fichiers

| Chemin | Rôle |
|---|---|
| `include/game.h` | Structures et constantes communes |
| `include/engine.h`, `src/engine.c` | Placement, validation et règles de tir |
| `include/workers.h`, `src/workers.c` | Threads, boîtes de commandes et synchronisation |
| `include/ui.h`, `src/ui.c` | Affichage et lecture de la console |
| `src/main.c` | Démarrage, boucle de jeu et nettoyage |
| `tests/test_engine.c` | Tests des règles et du placement |
| `tests/test_game.sh` | Parties automatiques et erreurs de saisie |
| `docs/rapport_projet.pdf` | Rapport technique |

Le PDF du sujet contient des références isolées à un « serveur de chat » et à des « clients ». Ce programme suit l'objectif explicite du sujet et du courriel : un jeu de bataille navale en console, sans réseau.
