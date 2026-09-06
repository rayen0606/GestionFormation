#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

// Classe responsable de l'unique connexion à la base de données Oracle.
// Conformément au cahier des charges : une seule connexion est ouverte
// pour toute l'application (pattern utilisé ici volontairement identique
// à celui du projet MaPharma-GestionClients).
class Connection
{
    QSqlDatabase db;

public:
    Connection();

    // Ouvre la connexion Oracle via un DSN ODBC (driver QODBC).
    // Adapter le nom du DSN / utilisateur / mot de passe à votre poste.
    bool createconnect();

    void closeConnection();
};

#endif // CONNECTION_H
