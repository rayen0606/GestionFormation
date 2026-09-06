#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>

// Gère les personnes inscrites aux cours (table PARTICIPANTS).
class Participant
{
    int id_participant;
    QString nom_participant;
    QString prenom_participant;
    QString email_participant;
    QString telephone_participant;
    QString date_naissance; // "yyyy-MM-dd", peut être vide

public:
    Participant();
    Participant(int id, QString nom, QString prenom, QString email, QString tel, QString dateNaissance);

    // ---- Getters ----
    int getId() const { return id_participant; }
    QString getNom() const { return nom_participant; }
    QString getPrenom() const { return prenom_participant; }
    QString getEmail() const { return email_participant; }
    QString getTelephone() const { return telephone_participant; }
    QString getDateNaissance() const { return date_naissance; }

    // ---- Setters ----
    void setId(int id) { id_participant = id; }
    void setNom(const QString &n) { nom_participant = n; }
    void setPrenom(const QString &p) { prenom_participant = p; }
    void setEmail(const QString &e) { email_participant = e; }
    void setTelephone(const QString &t) { telephone_participant = t; }
    void setDateNaissance(const QString &d) { date_naissance = d; }

    // ---- CRUD ----
    bool ajouter();
    QSqlQueryModel* afficher();
    bool modifier(int id, const QString &nom, const QString &prenom, const QString &email,
                  const QString &tel, const QString &dateNaissance);
    bool supprimer(int id);
    bool existe(int id);

    // ---- Recherche + tri combinés (filtre live) ----
    QSqlQueryModel* chercherEtTrier(const QString &texteCritere, const QString &colonneTri, bool ordreAscendant);

    // Modèle "id -> Nom Prénom" pour peupler les combo-box (inscriptions).
    QSqlQueryModel* listePourCombo();

    int prochainId();
};

#endif // PARTICIPANT_H
