#ifndef FORMATEUR_H
#define FORMATEUR_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>

class Formateur
{
    int id_formateur;
    QString nom_formateur;
    QString prenom_formateur;
    QString email_formateur;
    QString telephone_formateur;
    QString specialite;
    QString date_embauche;   // stocké au format "yyyy-MM-dd"
    double  tarif_horaire;

public:
    Formateur();
    Formateur(int id, QString nom, QString prenom, QString email, QString tel,
              QString specialite, QString dateEmbauche, double tarifHoraire);

    // ---- Getters ----
    int getId() const { return id_formateur; }
    QString getNom() const { return nom_formateur; }
    QString getPrenom() const { return prenom_formateur; }
    QString getEmail() const { return email_formateur; }
    QString getTelephone() const { return telephone_formateur; }
    QString getSpecialite() const { return specialite; }
    QString getDateEmbauche() const { return date_embauche; }
    double getTarifHoraire() const { return tarif_horaire; }

    // ---- Setters ----
    void setId(int id) { id_formateur = id; }
    void setNom(const QString &n) { nom_formateur = n; }
    void setPrenom(const QString &p) { prenom_formateur = p; }
    void setEmail(const QString &e) { email_formateur = e; }
    void setTelephone(const QString &t) { telephone_formateur = t; }
    void setSpecialite(const QString &s) { specialite = s; }
    void setDateEmbauche(const QString &d) { date_embauche = d; }
    void setTarifHoraire(double t) { tarif_horaire = t; }

    // ---- CRUD (fonctionnalités de base) ----
    bool ajouter();
    QSqlQueryModel* afficher();
    bool modifier(int id, const QString &nom, const QString &prenom, const QString &email,
                  const QString &tel, const QString &specialite, const QString &dateEmbauche,
                  double tarifHoraire);
    bool supprimer(int id);
    bool existe(int id);

    // ---- Métier 1 : recherche multicritère (nom, spécialité, email) ----
    QSqlQueryModel* chercherFormateur(const QString &nomCritere,
                                       const QString &specialiteCritere,
                                       const QString &emailCritere);

    // ---- Métier 1 bis : tri multicritère ----
    QSqlQueryModel* trierFormateurs(const QString &colonne, bool ordreAscendant);

    // ---- Recherche + tri combinés, pour la recherche dynamique (filtres live) ----
    QSqlQueryModel* chercherEtTrier(const QString &nomCritere,
                                     const QString &specialiteCritere,
                                     const QString &emailCritere,
                                     const QString &colonneTri,
                                     bool ordreAscendant);

    // Liste des spécialités distinctes en base, pour peupler dynamiquement
    // le filtre "Spécialité" (combo box) selon les données réelles.
    QSqlQueryModel* specialitesDistinctes();

    // ---- Métier 2 : statistiques (répartition par spécialité) ----
    QSqlQueryModel* statistiquesParSpecialite();

    // ---- Métier 4a (utile) : charge horaire totale du formateur ----
    // Retourne le nombre total d'heures de cours assurées par le formateur
    // et, via nbCours, le nombre de cours qui lui sont affectés.
    int calculerChargeHoraire(int idFormateur, int *nbCours = nullptr);

    // ---- Métier 4b (utile) : contact rapide du formateur ----
    bool contacterParEmail(const QString &email, const QString &sujet, const QString &message);

    // Retourne l'id du prochain formateur (via séquence Oracle)
    int prochainId();
};

#endif // FORMATEUR_H
