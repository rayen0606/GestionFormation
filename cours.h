#ifndef COURS_H
#define COURS_H

#include <QString>
#include <QByteArray>
#include <QSqlQuery>
#include <QSqlQueryModel>

class Cours
{
    int id_cours;
    QString titre_cours;
    QString description_cours;
    int duree_heures;
    QString niveau;          // Debutant / Intermediaire / Avance
    double prix;
    QString date_debut;      // "yyyy-MM-dd"
    int id_formateur;        // clé étrangère -> FORMATEURS
    QString nom_fichier;     // nom du fichier joint, avec son extension (ex: "support.pdf")
    QByteArray contenu_fichier; // contenu binaire du fichier joint (stocké en BLOB) -- tous types (txt, csv, pdf, doc...)

public:
    Cours();
    Cours(int id, QString titre, QString description, int duree, QString niveau,
          double prix, QString dateDebut, int idFormateur);

    // ---- Getters ----
    int getId() const { return id_cours; }
    QString getTitre() const { return titre_cours; }
    QString getDescription() const { return description_cours; }
    int getDureeHeures() const { return duree_heures; }
    QString getNiveau() const { return niveau; }
    double getPrix() const { return prix; }
    QString getDateDebut() const { return date_debut; }
    int getIdFormateur() const { return id_formateur; }
    QString getNomFichier() const { return nom_fichier; }
    QByteArray getContenuFichier() const { return contenu_fichier; }

    // ---- Setters ----
    void setId(int id) { id_cours = id; }
    void setTitre(const QString &t) { titre_cours = t; }
    void setDescription(const QString &d) { description_cours = d; }
    void setDureeHeures(int d) { duree_heures = d; }
    void setNiveau(const QString &n) { niveau = n; }
    void setPrix(double p) { prix = p; }
    void setDateDebut(const QString &d) { date_debut = d; }
    void setIdFormateur(int id) { id_formateur = id; }
    void setNomFichier(const QString &n) { nom_fichier = n; }
    void setContenuFichier(const QByteArray &c) { contenu_fichier = c; }

    // ---- CRUD (fonctionnalités de base) ----
    bool ajouter();
    QSqlQueryModel* afficher();
    bool modifier(int id, const QString &titre, const QString &description, int duree,
                  const QString &niveau, double prix, const QString &dateDebut, int idFormateur);
    bool supprimer(int id);
    bool existe(int id);

    // ---- Métier 1 : recherche multicritère (titre, niveau, formateur) ----
    QSqlQueryModel* chercherCours(const QString &titreCritere,
                                   const QString &niveauCritere,
                                   const QString &formateurCritere);

    // ---- Métier 1 bis : tri multicritère ----
    QSqlQueryModel* trierCours(const QString &colonne, bool ordreAscendant);

    // ---- Recherche + tri combinés, pour la recherche dynamique (filtres live) ----
    QSqlQueryModel* chercherEtTrier(const QString &titreCritere,
                                     const QString &niveauCritere,
                                     const QString &formateurCritere,
                                     const QString &colonneTri,
                                     bool ordreAscendant);

    // ---- Métier 2 : statistiques (répartition par niveau) ----
    QSqlQueryModel* statistiquesParNiveau();

    // ---- Métier 4a (utile) : vérifie qu'un formateur n'a pas déjà un cours
    //      à la même date (évite les conflits d'emploi du temps) ----
    bool verifierDisponibiliteFormateur(int idFormateur, const QString &dateDebut, int idCoursAExclure = -1);

    // ---- Métier 4b (utile) : duplique un cours (nouvelle session, même contenu) ----
    bool dupliquerCours(int idCoursSource, const QString &nouvelleDateDebut);

    // ---- Métier 5 (utile) : le formateur joint un fichier (texte, PDF,
    //      CSV, Word...) à un cours, depuis l'ajout ou la modification,
    //      et peut ensuite le relire depuis l'application ----
    bool joindreFichier(int idCours, const QString &nomFichier, const QByteArray &contenuFichier);
    bool lireFichier(int idCours, QString &nomFichierOut, QByteArray &contenuFichierOut);

    int prochainId();

    // Modèle "id -> titre" pour peupler les combo-box (inscriptions, planning).
    QSqlQueryModel* listePourCombo();
};

#endif // COURS_H
