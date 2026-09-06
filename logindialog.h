#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>

// Écran de connexion affiché avant la fenêtre principale. Vérifie les
// identifiants via Utilisateur::authentifier() (voir utilisateur.h).
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

    QString utilisateurConnecte() const { return m_login; }
    QString roleConnecte() const { return m_role; }

private slots:
    void onSeConnecter();

private:
    QLineEdit *loginEdit;
    QLineEdit *motDePasseEdit;
    QLabel *erreurLabel;

    QString m_login;
    QString m_role;
};

#endif // LOGINDIALOG_H
