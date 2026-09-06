#include "participant.h"
#include <QSqlError>
#include <QDebug>

Participant::Participant()
{
    id_participant = 0;
    nom_participant = "";
    prenom_participant = "";
    email_participant = "";
    telephone_participant = "";
    date_naissance = "";
}

Participant::Participant(int id, QString nom, QString prenom, QString email, QString tel, QString dateNaissance)
{
    this->id_participant = id;
    this->nom_participant = nom;
    this->prenom_participant = prenom;
    this->email_participant = email;
    this->telephone_participant = tel;
    this->date_naissance = dateNaissance;
}

int Participant::prochainId()
{
    QSqlQuery query;
    query.prepare("SELECT seq_participants.NEXTVAL FROM DUAL");
    if (query.exec() && query.next())
        return query.value(0).toInt();
    return -1;
}

bool Participant::ajouter()
{
    QSqlQuery query;
    query.prepare("INSERT INTO PARTICIPANTS "
                  "(id_participant, nom_participant, prenom_participant, email_participant, "
                  " telephone_participant, date_naissance) "
                  "VALUES (:id, :nom, :prenom, :email, :tel, "
                  " CASE WHEN :date1 IS NULL THEN NULL ELSE TO_DATE(:date2, 'YYYY-MM-DD') END)");

    query.bindValue(":id", id_participant);
    query.bindValue(":nom", nom_participant);
    query.bindValue(":prenom", prenom_participant);
    query.bindValue(":email", email_participant);
    query.bindValue(":tel", telephone_participant);
    QVariant d = date_naissance.isEmpty() ? QVariant(QVariant::String) : QVariant(date_naissance);
    query.bindValue(":date1", d);
    query.bindValue(":date2", d);

    if (!query.exec()) {
        qDebug() << "Erreur ajout participant :" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel* Participant::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT id_participant, nom_participant, prenom_participant, email_participant, "
                     "telephone_participant, NVL(TO_CHAR(date_naissance,'YYYY-MM-DD'), '') "
                     "FROM PARTICIPANTS ORDER BY id_participant");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prénom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Date de naissance"));

    return model;
}

bool Participant::existe(int id)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM PARTICIPANTS WHERE id_participant = :id");
    query.bindValue(":id", id);
    if (query.exec() && query.next())
        return query.value(0).toInt() > 0;
    return false;
}

bool Participant::modifier(int id, const QString &nom, const QString &prenom, const QString &email,
                            const QString &tel, const QString &dateNaissance)
{
    QSqlQuery query;
    query.prepare("UPDATE PARTICIPANTS SET nom_participant = :nom, prenom_participant = :prenom, "
                  "email_participant = :email, telephone_participant = :tel, "
                  "date_naissance = CASE WHEN :date1 IS NULL THEN NULL ELSE TO_DATE(:date2, 'YYYY-MM-DD') END "
                  "WHERE id_participant = :id");

    query.bindValue(":id", id);
    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":email", email);
    query.bindValue(":tel", tel);
    QVariant d = dateNaissance.isEmpty() ? QVariant(QVariant::String) : QVariant(dateNaissance);
    query.bindValue(":date1", d);
    query.bindValue(":date2", d);

    if (!query.exec()) {
        qDebug() << "Erreur modification participant :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Participant::supprimer(int id)
{
    // Contrainte d'intégrité : on empêche la suppression d'un participant
    // qui a encore des inscriptions (protège la clé étrangère INSCRIPTIONS.id_participant).
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM INSCRIPTIONS WHERE id_participant = :id");
    check.bindValue(":id", id);
    if (check.exec() && check.next() && check.value(0).toInt() > 0) {
        qDebug() << "Suppression refusée : des inscriptions dépendent de ce participant.";
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM PARTICIPANTS WHERE id_participant = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur suppression participant :" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel* Participant::chercherEtTrier(const QString &texteCritere, const QString &colonneTri, bool ordreAscendant)
{
    QString col = "nom_participant";
    if (colonneTri == "Nom") col = "nom_participant";
    else if (colonneTri == "Email") col = "email_participant";
    else if (colonneTri == "Date de naissance") col = "date_naissance";

    QString queryString = "SELECT id_participant, nom_participant, prenom_participant, email_participant, "
                           "telephone_participant, NVL(TO_CHAR(date_naissance,'YYYY-MM-DD'), '') "
                           "FROM PARTICIPANTS WHERE 1=1";

    if (!texteCritere.trimmed().isEmpty())
        queryString += " AND (LOWER(nom_participant) LIKE :texte OR LOWER(prenom_participant) LIKE :texte "
                        "OR LOWER(email_participant) LIKE :texte)";

    queryString += " ORDER BY " + col + (ordreAscendant ? " ASC" : " DESC");

    QSqlQuery query;
    query.prepare(queryString);
    if (!texteCritere.trimmed().isEmpty())
        query.bindValue(":texte", "%" + texteCritere.toLower() + "%");

    QSqlQueryModel *model = new QSqlQueryModel();
    if (!query.exec()) {
        qDebug() << "Erreur recherche/tri participants :" << query.lastError().text();
        delete model;
        return nullptr;
    }

    model->setQuery(query);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prénom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Date de naissance"));

    return model;
}

QSqlQueryModel* Participant::listePourCombo()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT id_participant, nom_participant || ' ' || prenom_participant AS nom_complet "
                     "FROM PARTICIPANTS ORDER BY nom_participant");
    return model;
}
