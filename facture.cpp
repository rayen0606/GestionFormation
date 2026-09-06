#include "facture.h"
#include "inscription.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QCoreApplication>

QString Facture::genererDepuisInscription(int idInscription)
{
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM FACTURES WHERE id_inscription = :id");
    check.bindValue(":id", idInscription);
    if (check.exec() && check.next() && check.value(0).toInt() > 0)
        return QCoreApplication::translate("Facture", "Une facture existe déjà pour cette inscription.");

    QString participant, coursTitre;
    double prix = 0.0;
    if (!Inscription::infosPourFacture(idInscription, participant, coursTitre, prix))
        return QCoreApplication::translate("Facture", "Inscription introuvable.");

    QSqlQuery seq;
    seq.prepare("SELECT seq_factures.NEXTVAL FROM DUAL");
    if (!seq.exec() || !seq.next())
        return QCoreApplication::translate("Facture", "Erreur interne (séquence).");
    int id = seq.value(0).toInt();

    QSqlQuery query;
    query.prepare("INSERT INTO FACTURES (id_facture, id_inscription, montant, statut_paiement) "
                  "VALUES (:id, :insc, :montant, 'En attente')");
    query.bindValue(":id", id);
    query.bindValue(":insc", idInscription);
    query.bindValue(":montant", prix);

    if (!query.exec()) {
        qDebug() << "Erreur génération facture :" << query.lastError().text();
        return QCoreApplication::translate("Facture", "Erreur lors de la génération de la facture.");
    }
    return QString(); // succès
}

QSqlQueryModel* Facture::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(
        "SELECT f.id_facture, p.nom_participant || ' ' || p.prenom_participant AS participant, "
        "c.titre_cours, f.montant, TO_CHAR(f.date_facture,'DD/MM/YYYY') AS date_facture, "
        "f.statut_paiement, NVL(f.mode_paiement, '-') "
        "FROM FACTURES f "
        "JOIN INSCRIPTIONS i ON i.id_inscription = f.id_inscription "
        "JOIN PARTICIPANTS p ON p.id_participant = i.id_participant "
        "JOIN COURS c ON c.id_cours = i.id_cours "
        "ORDER BY f.id_facture DESC");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Participant"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Cours"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Montant (DT)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Date"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Statut"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Mode de paiement"));
    return model;
}

QSqlQueryModel* Facture::chercher(const QString &texteCritere, const QString &statutCritere)
{
    QString queryString =
        "SELECT f.id_facture, p.nom_participant || ' ' || p.prenom_participant AS participant, "
        "c.titre_cours, f.montant, TO_CHAR(f.date_facture,'DD/MM/YYYY') AS date_facture, "
        "f.statut_paiement, NVL(f.mode_paiement, '-') "
        "FROM FACTURES f "
        "JOIN INSCRIPTIONS i ON i.id_inscription = f.id_inscription "
        "JOIN PARTICIPANTS p ON p.id_participant = i.id_participant "
        "JOIN COURS c ON c.id_cours = i.id_cours WHERE 1=1";

    if (!texteCritere.trimmed().isEmpty())
        queryString += " AND (LOWER(p.nom_participant) LIKE :texte OR LOWER(p.prenom_participant) LIKE :texte "
                        "OR LOWER(c.titre_cours) LIKE :texte)";
    if (!statutCritere.isEmpty() && statutCritere != QObject::tr("Tous"))
        queryString += " AND f.statut_paiement = :statut";

    queryString += " ORDER BY f.id_facture DESC";

    QSqlQuery query;
    query.prepare(queryString);
    if (!texteCritere.trimmed().isEmpty())
        query.bindValue(":texte", "%" + texteCritere.toLower() + "%");
    if (!statutCritere.isEmpty() && statutCritere != QObject::tr("Tous"))
        query.bindValue(":statut", statutCritere);

    QSqlQueryModel *model = new QSqlQueryModel();
    if (!query.exec()) {
        qDebug() << "Erreur recherche factures :" << query.lastError().text();
        delete model;
        return nullptr;
    }

    model->setQuery(query);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Participant"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Cours"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Montant (DT)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Date"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Statut"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Mode de paiement"));
    return model;
}

QSqlQueryModel* Facture::inscriptionsSansFacture()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(
        "SELECT i.id_inscription, p.nom_participant || ' ' || p.prenom_participant || ' — ' || c.titre_cours "
        "FROM INSCRIPTIONS i "
        "JOIN PARTICIPANTS p ON p.id_participant = i.id_participant "
        "JOIN COURS c ON c.id_cours = i.id_cours "
        "WHERE i.statut = 'Confirmee' "
        "AND NOT EXISTS (SELECT 1 FROM FACTURES f WHERE f.id_inscription = i.id_inscription) "
        "ORDER BY i.id_inscription DESC");
    return model;
}

bool Facture::marquerPayee(int id, const QString &modePaiement)
{
    QSqlQuery query;
    query.prepare("UPDATE FACTURES SET statut_paiement = 'Payee', mode_paiement = :mode, "
                  "date_paiement = SYSDATE WHERE id_facture = :id");
    query.bindValue(":mode", modePaiement);
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug() << "Erreur paiement facture :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Facture::annuler(int id)
{
    QSqlQuery query;
    query.prepare("UPDATE FACTURES SET statut_paiement = 'Annulee' WHERE id_facture = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug() << "Erreur annulation facture :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Facture::supprimer(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM FACTURES WHERE id_facture = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug() << "Erreur suppression facture :" << query.lastError().text();
        return false;
    }
    return true;
}

void Facture::statistiques(double &totalFacture, double &totalEncaisse, double &totalImpaye)
{
    totalFacture = totalEncaisse = totalImpaye = 0.0;
    QSqlQuery query;
    if (query.exec("SELECT NVL(SUM(montant),0) FROM FACTURES WHERE statut_paiement != 'Annulee'") && query.next())
        totalFacture = query.value(0).toDouble();
    if (query.exec("SELECT NVL(SUM(montant),0) FROM FACTURES WHERE statut_paiement = 'Payee'") && query.next())
        totalEncaisse = query.value(0).toDouble();
    if (query.exec("SELECT NVL(SUM(montant),0) FROM FACTURES WHERE statut_paiement = 'En attente'") && query.next())
        totalImpaye = query.value(0).toDouble();
}
