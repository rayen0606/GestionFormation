#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QPushButton>

// Première fenêtre affichée au lancement de l'application : connexion ou
// création de compte. Selon le type de compte reconnu, une des deux
// interfaces de destination est ouverte (voir main.cpp) :
//   - un administrateur (table ADMINISTRATEURS)     -> dashboard (MainWindow)
//   - un participant/étudiant (table PARTICIPANTS)  -> catalogue des cours
class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);

signals:
    void connexionAdminReussie(const QString &nomAdmin);
    void connexionEtudiantReussie(int idParticipant, const QString &nomComplet);

private slots:
    void onSeConnecter();
    void onCreerCompte();
    void onBasculerOnglet(int index);

private:
    QStackedWidget *authStack; // 0 = connexion, 1 = inscription (étudiant)
    QPushButton *btnOngletConnexion, *btnOngletInscription;
    QWidget *carteAuth;

    // Connexion (admin OU étudiant, détecté automatiquement)
    QLineEdit *loginEmailEdit, *loginMotDePasseEdit;
    QLabel *loginErreurLabel;

    // Inscription (crée uniquement un compte étudiant)
    QLineEdit *regNomEdit, *regPrenomEdit, *regEmailEdit, *regTelEdit;
    QLineEdit *regMotDePasseEdit, *regMotDePasseConfirmEdit;
    QLabel *regErreurLabel;

    QWidget* creerPanneauBranding();
    QWidget* creerPageConnexion();
    QWidget* creerPageInscription();
};

#endif // LOGINWINDOW_H
