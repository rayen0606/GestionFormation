# Gestion d'un Centre de Formation — Qt / C++ / Oracle

Projet académique repris sur la **même structure** que `MaPharma-GestionClients`
(une classe C++ par entité gérant elle-même ses requêtes SQL préparées, une
classe `Connection` unique en `QODBC`, une `MainWindow` qui orchestre l'UI,
une seule connexion Oracle ouverte au lancement).

## Lancement et rôles

Au démarrage, l'application affiche un **écran de connexion / création de
compte** (`LoginWindow`) :
- Un **administrateur** (table `ADMINISTRATEURS`, compte par défaut
  `admin@centre.tn` / `admin123`) est redirigé vers le **dashboard** de
  gestion (`MainWindow`, modules 1 à 5 ci-dessous).
- Un **étudiant** (table `PARTICIPANTS`) se connecte ou crée son compte
  (bouton "Créer un compte", réservé aux étudiants), et est redirigé vers
  le **catalogue de cours** (`EtudiantMainWindow`, module 6 ci-dessous).

Chacune de ces deux fenêtres a un bouton "Se déconnecter" qui ramène à
l'écran de connexion.

## Modules

1. **Gestion des Formateurs** — CRUD + recherche/tri multicritères +
   statistiques (camembert par spécialité) + export PDF + calcul de charge
   horaire + contact par email.
2. **Gestion des Cours** — CRUD + recherche/tri multicritères + statistiques
   (histogramme par niveau) + export PDF + vérification de disponibilité du
   formateur (anti-conflit d'horaire) + duplication de session.
3. **Participants & Inscriptions** — CRUD des participants + inscription
   d'un participant à un cours (statut En attente / Confirmée / Annulée),
   avec suivi dédié.
4. **Planning** — calendrier des séances (`QCalendarWidget`) : les jours
   avec séance sont surlignés, planification/suppression d'une séance
   (cours, horaire, salle, capacité) directement depuis le jour sélectionné.
5. **Facturation** — génération d'une facture depuis une inscription
   confirmée (montant = prix du cours), suivi du paiement (En attente /
   Payée / Annulée, mode de paiement), indicateurs (total facturé, encaissé,
   impayé).
6. **Espace Étudiant** (fenêtre principale distincte, `EtudiantMainWindow`,
   ouverte automatiquement après connexion/inscription) :
   - **Catalogue** de tous les cours affiché en grille de cartes (titre,
     niveau, durée, description, prix).
   - **Paiement** (simulé) directement depuis une carte : crée/confirme
     l'inscription, génère la facture, la marque payée — et débloque aussitôt
     les boutons **Lire** (texte affiché dans l'application, autres formats
     ouverts avec l'application associée du système) et **Télécharger**.

## Structure du dépôt

```
GestionCentreFormation/
├── GestionCentreFormation.pro   # fichier projet Qt
├── main.cpp                     # point d'entrée, ouvre l'unique connexion
├── connection.h / .cpp          # classe Connection (Oracle, driver QOCI)
├── formateur.h / .cpp           # entité Formateur : CRUD + recherche/tri + métiers
├── cours.h / .cpp               # entité Cours : CRUD + recherche/tri + métiers
├── participant.h / .cpp         # entité Participant : CRUD + recherche/tri
├── inscription.h / .cpp         # association Participant <-> Cours (statuts)
├── session.h / .cpp             # séances planifiées (planning/calendrier)
├── facture.h / .cpp             # facturation liée à une inscription
├── administrateur.h / .cpp       # authentification de l'équipe du centre
├── loginwindow.h / .cpp          # écran de connexion/inscription (démarrage)
├── portailetudiant.h / .cpp      # compte étudiant (SHA-256), catalogue, paiement
├── etudiantmainwindow.h / .cpp   # fenêtre étudiante (catalogue en grille, paiement)
├── mainwindow.h / .cpp          # UI (QTabWidget + QStackedWidget), aucune requête SQL ici
├── sql/create_tables.sql        # DDL Oracle (tables, séquences, contraintes, jeu de données)
└── docs/MCD_MLD_MPD.md          # modélisation conceptuelle / logique / physique
```

## Choix techniques imposés respectés

- **Une seule connexion** à la base (classe `Connection`, ouverte dans `main.cpp`).
- **Requêtes préparées** (`QSqlQuery::prepare` + `bindValue`) partout.
- **Toutes les requêtes SQL vivent dans les classes `Formateur` et `Cours`**,
  jamais directement derrière un `connect()`/bouton dans `mainwindow.cpp`.
- **Clé primaire / clé étrangère** : `COURS.id_formateur` référence
  `FORMATEURS.id_formateur` ; la suppression d'un formateur ayant des cours
  associés est bloquée côté C++ (et protégée côté base par la contrainte FK).
- **GUI ergonomique** : un `QTabWidget` (un onglet par module) contenant pour
  chaque module un `QStackedWidget` (pages Liste / Modifier / Statistiques) —
  aucune boîte de dialogue modale utilisée pour la saisie.
- **Graphiques dynamiques** : les pages Statistiques sont reconstruites à
  chaque ouverture à partir de la base (QtCharts), donc toujours à jour.
- **PDF personnalisé** : export via `QPainter`/`QPrinter` avec mise en page
  propre (titres, en-têtes de colonnes, alternance de couleurs, pagination) —
  ce n'est pas une capture d'écran.

## Prérequis pour builder

- **Qt 5 ou Qt 6** (modules `sql`, `printsupport`, `charts`, `widgets`,
  **`quick`, `quickwidgets`, `qml`** — nécessaires au fond animé du bandeau,
  voir section Design ci-dessous). Le code s'adapte automatiquement aux deux
  versions majeures (voir la garde `#if QT_VERSION` dans `mainwindow.cpp`
  pour l'espace de nommage de QtCharts, différent entre Qt 5 et Qt 6).
  - Sous Ubuntu/Debian (Qt 6) : `sudo apt install qt6-base-dev
    qt6-declarative-dev qt6-charts-dev`.
  - Avec le Qt Maintenance Tool (Windows/macOS) : cocher le composant
    **"Qt Quick"** en plus de votre kit habituel (inclus par défaut dans la
    plupart des installations récentes de Qt 6, comme Qt 6.11 MinGW).
- Driver `QODBC` (fourni de base avec Qt) + un DSN ODBC Oracle configuré sur
  le poste (Panneau de configuration → Outils d'administration → Sources de
  données ODBC, sous Windows ; `odbc.ini`/`odbcinst.ini` sous Linux).
- Exécuter `sql/create_tables.sql` sur votre schéma Oracle avant le premier
  lancement.
- Adapter le nom du DSN, l'utilisateur et le mot de passe dans
  `connection.cpp` (variables `dsn`, `user`, `pass`).

## Design : animations et fond anime (shader)

L'interface a été enrichie de plusieurs effets, tous natifs Qt (aucune
dépendance externe) :

- **Transitions de page** (`animerApparition` / `basculerAvecAnimation`,
  dans `mainwindow.cpp`) : fondu + léger glissement vertical à chaque
  changement d'onglet ou de sous-page (Liste/Ajouter/Modifier/Stats).
- **Cartes interactives** (`ombrerCarte(..., true)`) : les cartes du
  tableau de bord (KPI, prochains cours, activité récente) réagissent au
  survol avec une élévation animée (ombre + décalage).
- **Compteurs animés** (`animerCompteur`) : les 4 indicateurs-clés du
  tableau de bord défilent vers leur nouvelle valeur au lieu de changer
  brutalement.
- **Fond anime du bandeau d'en-tête** (`creerBandeauEnTete()` dans
  `mainwindow.cpp`) : un dégradé "aurora" animé, généré par un **shader
  GLSL exécuté sur le GPU**, superposé au titre et à la navigation grâce à
  un `QStackedLayout` en mode `StackAll`. Si le module QtQuick n'est pas
  disponible sur la machine cible, l'application bascule automatiquement
  sur un fond uni — aucune fonctionnalité n'est perdue, seul l'effet visuel
  est absent.

  Fichiers concernés (`resources/shaders/`) :
  - `HeaderBackground.qml` — la scène QML (anime le temps, passe les
    couleurs au shader).
  - `HeaderBackground.frag` — le **code source** du shader (GLSL 4.40,
    à éditer si vous voulez modifier l'effet).
  - `HeaderBackground.frag.qsb` — la version **précompilée** du shader,
    utilisée telle quelle au runtime (Qt 6 impose ce format binaire
    portable — GLSL/HLSL/MSL/SPIR-V — pour `ShaderEffect`, il n'accepte
    plus de source GLSL brute comme en Qt 5).

  **Si vous modifiez `HeaderBackground.frag`**, il faut le recompiler avec
  l'outil `qsb` (fourni par le module Qt Shader Tools, paquet
  `qt6-shadertools-dev` sous Ubuntu, ou installé avec le kit Qt 6 standard
  sous Windows/macOS) :
  ```bash
  qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 \
      -o resources/shaders/HeaderBackground.frag.qsb \
      resources/shaders/HeaderBackground.frag
  ```
  Ce n'est nécessaire que si vous éditez le shader : le `.qsb` fourni dans
  ce dépôt suffit pour builder et lancer l'application telle quelle.

## Git / GitHub

```bash
git init
git add .
git commit -m "Initial commit : structure du projet Gestion Centre de Formation"
git branch -M main
git remote add origin <url-de-votre-depot>
git push -u origin main
```

Faites ensuite un commit par étape significative (CRUD Formateurs, CRUD
Cours, recherche/tri, statistiques, export PDF, métiers) pour bien montrer
l'historique de versions attendu par l'énoncé.
