# Rapport technique — Bataille navale multijoueurs

## Introduction et objectifs

Ce projet répond au sujet du module « Systèmes d'exploitation avancés » : réaliser un jeu de bataille navale en console, avec plusieurs joueurs représentés chacun par un thread et des mises à jour de grilles synchronisées. Le jeu accepte de deux à quatre joueurs. Les passages du sujet concernant un serveur de chat semblent appartenir à un autre exercice ; aucun composant réseau n'a été ajouté.

## Conception logicielle

`engine.c` contient les règles du jeu et ne réalise aucune entrée/sortie. Chaque `Board` possède une matrice de navires, une matrice de cases déjà visées, cinq structures `Ship` et un compteur de segments restants. Chaque `Player` contient sa grille, son identifiant POSIX `pthread_t`, son état vivant et une boîte de commande. `Game` rassemble les joueurs, le tour, le gagnant et les primitives de synchronisation. `ui.c` lit les commandes et affiche les grilles ; `main.c` démarre et arrête la partie.

Les flottes sont placées automatiquement avant le démarrage des threads. Un petit générateur pseudo-aléatoire local permet de rejouer une partie avec une graine fixe. Chaque position candidate est vérifiée avant de poser le navire ; une limite d'essais évite une boucle infinie en cas d'échec imprévu.

## Pthreads et synchronisation

Le thread principal est l'unique propriétaire de `stdin` et de l'affichage. Il envoie une commande de tir au worker du joueur actif. Ce worker valide la cible, les coordonnées et l'absence de tir répété. Si la commande est valide, il la transmet au worker du défenseur. Celui-ci modifie sa propre grille, calcule le résultat, met à jour l'élimination et le tour, puis réveille le thread principal.

Un mutex unique protège tout l'état partagé. Les workers attendent sur leur variable de condition avec une boucle qui vérifie la présence d'une commande ou l'arrêt. Le principal attend le résultat de la même façon. Une seule commande est en vol à la fois ; les boîtes ne peuvent donc pas être écrasées. La validation et la modification d'un tir se déroulent sous le même verrou, ce qui empêche deux mises à jour contradictoires. Aucun thread ne lit la console pendant qu'il détient le mutex. À l'arrêt ou à la fin de l'entrée, le principal signale tous les workers, les rejoint et détruit les ressources POSIX.

## Logique du jeu

Chaque flotte a des navires de longueurs 5, 4, 3, 3 et 2 sur une grille 10 × 10. Une attaque valide marque une case une seule fois, puis produit « raté », « touché » ou « coulé ». Quand le dernier segment d'une flotte est touché, son joueur est éliminé ; le tour ignore ensuite ce joueur. Quand il ne reste qu'un joueur vivant, le tir décisif annonce sa victoire. Une commande invalide ou répétée conserve le tour. Les grilles adverses affichées ne montrent que les tirs connus.

## Défis et limites

Le point délicat est de faire travailler réellement les threads de joueur tout en évitant les lectures concurrentes de `stdin`. Les boîtes de commande et les conditions résolvent ce conflit sans attente active. Le mutex unique simplifie la cohérence au prix d'une exécution sérialisée des transitions de jeu ; cela correspond au jeu à tours. Une console physique commune ne protège pas la grille privée contre les regards des autres joueurs. Le mode `--demo` utilise volontairement les positions cachées pour exercer une partie complète ; il est séparé du jeu normal.

## Vérification

La cible `make test` compile le programme, exécute les tests du moteur (200 graines de placement pour quatre joueurs, validation des tirs, répétitions, navires coulés, élimination et victoire), puis les scénarios d'intégration (partie automatique, entrée invalide, EOF et tir répété). Les commandes exactes figurent dans le README. Le résultat observé était : `Engine tests passed.` et `Integration tests passed.` Trois cents parties de démonstration supplémentaires (100 graines pour chacun de 2, 3 et 4 joueurs) se sont terminées sans blocage. Une partie et les tests du moteur ont aussi passé AddressSanitizer/UndefinedBehaviorSanitizer avec la détection des fuites désactivée ; LeakSanitizer ne fonctionne pas dans l'environnement d'exécution utilisé.

## Conclusion

Le programme fournit un jeu console jouable et une démonstration complète du partage d'état entre threads POSIX, avec arrêt propre et règles testées. Les défauts éventuels du sujet liés au « serveur de chat » devront être clarifiés auprès de l'enseignante pour l'évaluation.
