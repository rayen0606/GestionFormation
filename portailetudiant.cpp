#include "portailetudiant.h"
#include "inscription.h"
#include "facture.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QVariant>
#include <QDebug>

static QString hasher(const QString &motDePasse)
{
    return QString::fromUtf8(QCryptographicHash::hash(motDePasse.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString PortailEtudiant::creerCompte(const QString &nom, const QString &prenom, const QString &email,
                                      const QString &telephone, const QString &motDePasse,
                                      int &idParticipantOut)
{
    QSqlQuery check;
    check.prepare("SELECT id_participant, mot_de_passe_hash FROM PARTICIPANTS WHERE LOWER(email_participant) = LOWER(:email)");
    check.bindValue(":email", email.trimmed());
    if (!check.exec())
        return QCoreApplication::translate("PortailEtudiant", "Erreur interne lors de la vérification de l'email.");

    QString hash = hasher(motDePasse);

    if (check.next()) {
        int idExistant = check.value(0).toInt();
        QString hashExistant = check.value(1).toString();
        if (!hashExistant.isEmpty())
            return QCoreApplication::translate("PortailEtudiant",
                "Un compte existe déjà avec cet email. Connectez-vous plutôt.");

        // Participant déjà connu (ajouté côté administration) mais sans
        // compte : on active le compte sur cette fiche existante.
        QSqlQuery maj;
        maj.prepare("UPDATE PARTICIPANTS SET mot_de_passe_hash = :hash, "
                    "nom_participant = :nom, prenom_participant = :prenom, "
                    "telephone_participant = NVL(:tel, telephone_participant) "
                    "WHERE id_participant = :id");
        maj.bindValue(":hash", hash);
        maj.bindValue(":nom", nom);
        maj.bindValue(":prenom", prenom);
        maj.bindValue(":tel", telephone.trimmed().isEmpty() ? QVariant(QVariant::String) : QVariant(telephone.trimmed()));
        maj.bindValue(":id", idExistant);
        if (!maj.exec()) {
            qDebug() << "Erreur activation compte :" << maj.lastError().text();
            return QCoreApplication::translate("PortailEtudiant", "Erreur lors de la création du compte.");
        }
        idParticipantOut = idExistant;
        return QString();
    }

    // Nouveau participant.
    QSqlQuery seq;
    seq.prepare("SELECT seq_participants.NEXTVAL FROM DUAL");
    if (!seq.exec() || !seq.next())
        return QCoreApplication::translate("PortailEtudiant", "Erreur interne (séquence).");
    int id = seq.value(0).toInt();

    QSqlQuery insert;
    insert.prepare("INSERT INTO PARTICIPANTS (id_participant, nom_participant, prenom_participant, "
                   "email_participant, telephone_participant, mot_de_passe_hash) "
                   "VALUES (:id, :nom, :prenom, :email, :tel, :hash)");
    insert.bindValue(":id", id);
    insert.bindValue(":nom", nom);
    insert.bindValue(":prenom", prenom);
    insert.bindValue(":email", email.trimmed());
    insert.bindValue(":tel", telephone.trimmed());
    insert.bindValue(":hash", hash);

    if (!insert.exec()) {
        qDebug() << "Erreur création compte :" << insert.lastError().text();
        return QCoreApplication::translate("PortailEtudiant", "Erreur lors de la création du compte.");
    }
    idParticipantOut = id;
    return QString();
}

bool PortailEtudiant::seConnecter(const QString &email, const QString &motDePasse,
                                   int &idParticipantOut, QString &nomCompletOut)
{
    QSqlQuery query;
    query.prepare("SELECT id_participant, nom_participant || ' ' || prenom_participant "
                  "FROM PARTICIPANTS WHERE LOWER(email_participant) = LOWER(:email) "
                  "AND mot_de_passe_hash = :hash");
    query.bindValue(":email", email.trimmed());
    query.bindValue(":hash", hasher(motDePasse));

    if (!query.exec()) {
        qDebug() << "Erreur connexion étudiant :" << query.lastError().text();
        return false;
    }
    if (!query.next()) return false;

    idParticipantOut = query.value(0).toInt();
    nomCompletOut = query.value(1).toString();
    return true;
}

QSqlQueryModel* PortailEtudiant::catalogueCours(int idParticipant)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    QSqlQuery query;
    query.prepare(
        "SELECT c.id_cours, c.titre_cours, c.niveau, c.duree_heures, c.prix, "
        "NVL(c.description_cours, ''), "
        "CASE WHEN EXISTS ("
        "  SELECT 1 FROM INSCRIPTIONS i JOIN FACTURES f ON f.id_inscription = i.id_inscription "
        "  WHERE i.id_participant = :id AND i.id_cours = c.id_cours AND f.statut_paiement = 'Payee'"
        ") THEN 1 ELSE 0 END AS paye "
        "FROM COURS c ORDER BY c.titre_cours");
    query.bindValue(":id", idParticipant);

    if (!query.exec()) {
        qDebug() << "Erreur chargement catalogue :" << query.lastError().text();
        return model;
    }
    model->setQuery(query);
    return model;
}

bool PortailEtudiant::aPaye(int idParticipant, int idCours)
{
    QSqlQuery query;
    query.prepare(
        "SELECT COUNT(*) FROM INSCRIPTIONS i JOIN FACTURES f ON f.id_inscription = i.id_inscription "
        "WHERE i.id_participant = :p AND i.id_cours = :c AND f.statut_paiement = 'Payee'");
    query.bindValue(":p", idParticipant);
    query.bindValue(":c", idCours);
    if (query.exec() && query.next())
        return query.value(0).toInt() > 0;
    return false;
}

QString PortailEtudiant::payerCours(int idParticipant, int idCours, const QString &modePaiement)
{
    if (aPaye(idParticipant, idCours))
        return QCoreApplication::translate("PortailEtudiant", "Ce cours est déjà payé.");

    // Récupère l'inscription existante, ou en crée une nouvelle.
    QSqlQuery findInsc;
    findInsc.prepare("SELECT id_inscription FROM INSCRIPTIONS WHERE id_participant = :p AND id_cours = :c");
    findInsc.bindValue(":p", idParticipant);
    findInsc.bindValue(":c", idCours);
    if (!findInsc.exec())
        return QCoreApplication::translate("PortailEtudiant", "Erreur interne lors du paiement.");

    int idInscription = -1;
    if (findInsc.next()) {
        idInscription = findInsc.value(0).toInt();
    } else {
        QString erreur = Inscription::inscrire(idParticipant, idCours);
        if (!erreur.isEmpty()) return erreur;

        QSqlQuery refetch;
        refetch.prepare("SELECT id_inscription FROM INSCRIPTIONS WHERE id_participant = :p AND id_cours = :c");
        refetch.bindValue(":p", idParticipant);
        refetch.bindValue(":c", idCours);
        if (!refetch.exec() || !refetch.next())
            return QCoreApplication::translate("PortailEtudiant", "Erreur interne lors de l'inscription.");
        idInscription = refetch.value(0).toInt();
    }

    Inscription::changerStatut(idInscription, "Confirmee");

    // Récupère la facture existante (générée mais non payée), ou en crée une.
    QSqlQuery findFact;
    findFact.prepare("SELECT id_facture FROM FACTURES WHERE id_inscription = :id");
    findFact.bindValue(":id", idInscription);
    if (!findFact.exec())
        return QCoreApplication::translate("PortailEtudiant", "Erreur interne lors de la facturation.");

    int idFacture = -1;
    if (findFact.next()) {
        idFacture = findFact.value(0).toInt();
    } else {
        QString erreur = Facture::genererDepuisInscription(idInscription);
        if (!erreur.isEmpty()) return erreur;

        QSqlQuery refetch;
        refetch.prepare("SELECT id_facture FROM FACTURES WHERE id_inscription = :id");
        refetch.bindValue(":id", idInscription);
        if (!refetch.exec() || !refetch.next())
            return QCoreApplication::translate("PortailEtudiant", "Erreur interne lors de la facturation.");
        idFacture = refetch.value(0).toInt();
    }

    if (!Facture::marquerPayee(idFacture, modePaiement))
        return QCoreApplication::translate("PortailEtudiant", "Erreur lors de l'enregistrement du paiement.");

    return QString(); // succès
}
