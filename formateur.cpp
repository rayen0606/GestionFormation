#include "formateur.h"
#include <QSqlError>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>

Formateur::Formateur()
{
    id_formateur = 0;
    nom_formateur = "";
    prenom_formateur = "";
    email_formateur = "";
    telephone_formateur = "";
    specialite = "";
    date_embauche = "";
    tarif_horaire = 0.0;
}

Formateur::Formateur(int id, QString nom, QString prenom, QString email, QString tel,
                      QString specialite, QString dateEmbauche, double tarifHoraire)
{
    this->id_formateur = id;
    this->nom_formateur = nom;
    this->prenom_formateur = prenom;
    this->email_formateur = email;
    this->telephone_formateur = tel;
    this->specialite = specialite;
    this->date_embauche = dateEmbauche;
    this->tarif_horaire = tarifHoraire;
}

int Formateur::prochainId()
{
    QSqlQuery query;
    query.prepare("SELECT seq_formateurs.NEXTVAL FROM DUAL");
    if (query.exec() && query.next())
        return query.value(0).toInt();
    return -1;
}

bool Formateur::ajouter()
{
    QSqlQuery query;
    query.prepare("INSERT INTO FORMATEURS "
                  "(id_formateur, nom_formateur, prenom_formateur, email_formateur, "
                  " telephone_formateur, specialite, date_embauche, tarif_horaire) "
                  "VALUES (:id, :nom, :prenom, :email, :tel, :specialite, "
                  " TO_DATE(:date_embauche, 'YYYY-MM-DD'), :tarif)");

    query.bindValue(":id", id_formateur);
    query.bindValue(":nom", nom_formateur);
    query.bindValue(":prenom", prenom_formateur);
    query.bindValue(":email", email_formateur);
    query.bindValue(":tel", telephone_formateur);
    query.bindValue(":specialite", specialite);
    query.bindValue(":date_embauche", date_embauche);
    query.bindValue(":tarif", tarif_horaire);

    if (!query.exec()) {
        qDebug() << "Erreur ajout formateur :" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel* Formateur::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT id_formateur, nom_formateur, prenom_formateur, email_formateur, "
                     "telephone_formateur, specialite, TO_CHAR(date_embauche,'YYYY-MM-DD'), tarif_horaire "
                     "FROM FORMATEURS ORDER BY id_formateur");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prénom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Spécialité"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Date d'embauche"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Tarif horaire"));

    return model;
}

bool Formateur::existe(int id)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM FORMATEURS WHERE id_formateur = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next())
        return query.value(0).toInt() > 0;
    return false;
}

bool Formateur::modifier(int id, const QString &nom, const QString &prenom, const QString &email,
                          const QString &tel, const QString &specialite, const QString &dateEmbauche,
                          double tarifHoraire)
{
    QSqlQuery query;
    query.prepare("UPDATE FORMATEURS SET nom_formateur = :nom, prenom_formateur = :prenom, "
                  "email_formateur = :email, telephone_formateur = :tel, specialite = :specialite, "
                  "date_embauche = TO_DATE(:date_embauche, 'YYYY-MM-DD'), tarif_horaire = :tarif "
                  "WHERE id_formateur = :id");

    query.bindValue(":id", id);
    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":email", email);
    query.bindValue(":tel", tel);
    query.bindValue(":specialite", specialite);
    query.bindValue(":date_embauche", dateEmbauche);
    query.bindValue(":tarif", tarifHoraire);

    if (!query.exec()) {
        qDebug() << "Erreur modification formateur :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Formateur::supprimer(int id)
{
    // Contrainte d'intégrité : on empêche la suppression d'un formateur
    // qui a encore des cours affectés (protège la clé étrangère COURS.id_formateur).
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM COURS WHERE id_formateur = :id");
    check.bindValue(":id", id);
    if (check.exec() && check.next() && check.value(0).toInt() > 0) {
        qDebug() << "Suppression refusée : des cours dépendent de ce formateur.";
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM FORMATEURS WHERE id_formateur = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur suppression formateur :" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel* Formateur::chercherFormateur(const QString &nomCritere,
                                              const QString &specialiteCritere,
                                              const QString &emailCritere)
{
    QSqlQueryModel *model = new QSqlQueryModel();

    QString queryString = "SELECT id_formateur, nom_formateur, prenom_formateur, email_formateur, "
                           "telephone_formateur, specialite, TO_CHAR(date_embauche,'YYYY-MM-DD'), tarif_horaire "
                           "FROM FORMATEURS WHERE 1=1";

    if (!nomCritere.trimmed().isEmpty())
        queryString += " AND (LOWER(nom_formateur) LIKE :nom OR LOWER(prenom_formateur) LIKE :nom)";
    if (!specialiteCritere.trimmed().isEmpty())
        queryString += " AND LOWER(specialite) LIKE :specialite";
    if (!emailCritere.trimmed().isEmpty())
        queryString += " AND LOWER(email_formateur) LIKE :email";

    QSqlQuery query;
    query.prepare(queryString);

    if (!nomCritere.trimmed().isEmpty())
        query.bindValue(":nom", "%" + nomCritere.toLower() + "%");
    if (!specialiteCritere.trimmed().isEmpty())
        query.bindValue(":specialite", "%" + specialiteCritere.toLower() + "%");
    if (!emailCritere.trimmed().isEmpty())
        query.bindValue(":email", "%" + emailCritere.toLower() + "%");

    if (!query.exec()) {
        qDebug() << "Erreur recherche formateurs :" << query.lastError().text();
        delete model;
        return nullptr;
    }

    model->setQuery(query);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prénom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Spécialité"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Date d'embauche"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Tarif horaire"));

    return model;
}

QSqlQueryModel* Formateur::trierFormateurs(const QString &colonne, bool ordreAscendant)
{
    QString col = "nom_formateur";
    if (colonne == "Nom") col = "nom_formateur";
    else if (colonne == "Spécialité") col = "specialite";
    else if (colonne == "Tarif horaire") col = "tarif_horaire";
    else if (colonne == "Date d'embauche") col = "date_embauche";

    QString queryString = "SELECT id_formateur, nom_formateur, prenom_formateur, email_formateur, "
                           "telephone_formateur, specialite, TO_CHAR(date_embauche,'YYYY-MM-DD'), tarif_horaire "
                           "FROM FORMATEURS ORDER BY " + col + (ordreAscendant ? " ASC" : " DESC");

    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(queryString);

    if (model->lastError().isValid()) {
        qDebug() << "Erreur tri formateurs :" << model->lastError().text();
        delete model;
        return nullptr;
    }

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prénom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Spécialité"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Date d'embauche"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Tarif horaire"));

    return model;
}

QSqlQueryModel* Formateur::chercherEtTrier(const QString &nomCritere,
                                            const QString &specialiteCritere,
                                            const QString &emailCritere,
                                            const QString &colonneTri,
                                            bool ordreAscendant)
{
    // Utilisée par la recherche dynamique : combine les filtres (WHERE)
    // et le tri (ORDER BY) en une seule requête, recalculée à chaque
    // changement d'un champ de filtre ou de tri côté interface.
    QString col = "nom_formateur";
    if (colonneTri == "Nom") col = "nom_formateur";
    else if (colonneTri == "Spécialité") col = "specialite";
    else if (colonneTri == "Tarif horaire") col = "tarif_horaire";
    else if (colonneTri == "Date d'embauche") col = "date_embauche";

    QString queryString = "SELECT id_formateur, nom_formateur, prenom_formateur, email_formateur, "
                           "telephone_formateur, specialite, TO_CHAR(date_embauche,'YYYY-MM-DD'), tarif_horaire "
                           "FROM FORMATEURS WHERE 1=1";

    if (!nomCritere.trimmed().isEmpty())
        queryString += " AND (LOWER(nom_formateur) LIKE :nom OR LOWER(prenom_formateur) LIKE :nom)";
    if (!specialiteCritere.trimmed().isEmpty())
        queryString += " AND LOWER(specialite) LIKE :specialite";
    if (!emailCritere.trimmed().isEmpty())
        queryString += " AND LOWER(email_formateur) LIKE :email";

    queryString += " ORDER BY " + col + (ordreAscendant ? " ASC" : " DESC");

    QSqlQuery query;
    query.prepare(queryString);

    if (!nomCritere.trimmed().isEmpty())
        query.bindValue(":nom", "%" + nomCritere.toLower() + "%");
    if (!specialiteCritere.trimmed().isEmpty())
        query.bindValue(":specialite", "%" + specialiteCritere.toLower() + "%");
    if (!emailCritere.trimmed().isEmpty())
        query.bindValue(":email", "%" + emailCritere.toLower() + "%");

    QSqlQueryModel *model = new QSqlQueryModel();

    if (!query.exec()) {
        qDebug() << "Erreur recherche/tri formateurs :" << query.lastError().text();
        delete model;
        return nullptr;
    }

    model->setQuery(query);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prénom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Spécialité"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Date d'embauche"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Tarif horaire"));

    return model;
}

QSqlQueryModel* Formateur::specialitesDistinctes()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT DISTINCT specialite FROM FORMATEURS ORDER BY specialite");
    return model;
}

QSqlQueryModel* Formateur::statistiquesParSpecialite()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT specialite, COUNT(*) AS nb FROM FORMATEURS "
                     "GROUP BY specialite ORDER BY specialite");
    return model;
}

int Formateur::calculerChargeHoraire(int idFormateur, int *nbCours)
{
    QSqlQuery query;
    query.prepare("SELECT NVL(SUM(duree_heures),0), COUNT(*) FROM COURS WHERE id_formateur = :id");
    query.bindValue(":id", idFormateur);

    if (query.exec() && query.next()) {
        if (nbCours) *nbCours = query.value(1).toInt();
        return query.value(0).toInt();
    }
    if (nbCours) *nbCours = 0;
    return 0;
}

bool Formateur::contacterParEmail(const QString &email, const QString &sujet, const QString &message)
{
    QUrl mailUrl("mailto:" + email);
    QUrlQuery params;
    params.addQueryItem("subject", sujet);
    params.addQueryItem("body", message);
    mailUrl.setQuery(params);

    return QDesktopServices::openUrl(mailUrl);
}
