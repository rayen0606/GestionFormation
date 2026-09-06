#include "mainwindow.h"
#include "loginwindow.h"
#include "etudiantmainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QFont>
#include <functional>
#include "connection.h"

// Feuille de style embarquee (aucun fichier externe requis) : palette
// professionnelle bleu marine / bleu vif, boutons differencies par role
// (nav / primaire / danger / contour / discret, voir styliserBouton()
// dans mainwindow.cpp), cartes arrondies, tableaux modernises.
static const char *FEUILLE_DE_STYLE = R"(
* {
    font-family: "Segoe UI", "Ubuntu", "Helvetica Neue", sans-serif;
    font-size: 10.5pt;
    color: #1E293B;
}

QMainWindow {
    background-color: #F1F5F9;
}

/* ---------- Bandeau d'en-tete ---------- */
/* Deux zones bien distinctes, jamais superposees (voir
   MainWindow::creerBandeauEnTete pour le detail et la raison) :
   - la banniere animee (QQuickWidget, shader) est totalement transparente
     au niveau QSS puisqu'elle dessine elle-meme son propre fond ;
   - #headerContentSecours est le repli statique utilise uniquement si le
     module QtQuick est indisponible sur la machine ;
   - #headerNav est la zone de navigation classique, toujours un widget Qt
     normal, avec le meme bleu marine que le reste du bandeau. */
QWidget#headerBar {
    background-color: transparent;
}
QWidget#headerContentSecours {
    background-color: #1E3A5F;
}
QWidget#headerNav {
    background-color: #1E3A5F;
}
QLabel#titreApplication {
    color: #FFFFFF;
    font-size: 18pt;
    font-weight: 700;
    letter-spacing: 0.5px;
}

/* ---------- Navigation centrale (segmented control) ---------- */
QPushButton[classe="segment"] {
    background-color: transparent;
    color: #B8C7DA;
    border: 1.5px solid #33507A;
    border-radius: 22px;
    padding: 8px 24px;
    font-size: 11pt;
    font-weight: 600;
}
QPushButton[classe="segment"]:hover {
    background-color: #24446E;
    color: #FFFFFF;
    border: 1.5px solid #3B82F6;
}
QPushButton[classe="segment"]:checked {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4F94F7, stop:1 #2563EB);
    color: #FFFFFF;
    border: 1.5px solid #3B82F6;
}
QPushButton[classe="segment"]:checked:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4285ED, stop:1 #1D4ED8);
}

/* ---------- Cartes (QGroupBox) ---------- */
QGroupBox {
    background-color: #FFFFFF;
    border: 1px solid #E2E8F0;
    border-radius: 10px;
    margin-top: 14px;
    padding: 18px 14px 14px 14px;
    font-weight: 600;
    color: #334155;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 14px;
    padding: 0 6px;
    color: #1E3A5F;
    font-size: 11pt;
    font-weight: 700;
}

/* ---------- Champs de saisie ---------- */
QLineEdit, QDateEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background-color: #FFFFFF;
    border: 1px solid #CBD5E1;
    border-radius: 6px;
    padding: 6px 10px;
    selection-background-color: #3B82F6;
}
QLineEdit:focus, QDateEdit:focus, QComboBox:focus,
QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1.5px solid #3B82F6;
}
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView {
    background-color: #FFFFFF;
    border: 1px solid #CBD5E1;
    selection-background-color: #3B82F6;
    selection-color: #FFFFFF;
    outline: none;
}
QCheckBox { spacing: 6px; }

/* ---------- Boutons ---------- */
QPushButton {
    background-color: #E2E8F0;
    color: #334155;
    border: none;
    border-radius: 6px;
    padding: 8px 16px;
    font-weight: 600;
}
QPushButton:hover { background-color: #CBD5E1; }
QPushButton:pressed { background-color: #94A3B8; }

/* Barre de navigation interne (Liste / Ajouter / Statistiques) : style pilule */
QPushButton[classe="nav"] {
    background-color: #EFF6FF;
    color: #1E3A5F;
    border-radius: 17px;
    padding: 8px 20px;
}
QPushButton[classe="nav"]:hover { background-color: #DBEAFE; }
QPushButton[classe="nav"]:pressed { background-color: #BFDBFE; }

/* Action principale (Ajouter / Enregistrer) */
QPushButton[classe="primaire"] {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3B82F6, stop:1 #2563EB);
    color: #FFFFFF;
}
QPushButton[classe="primaire"]:hover {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2F72E0, stop:1 #1D4ED8);
}
QPushButton[classe="primaire"]:pressed { background-color: #1E40AF; }

/* Action destructive (Supprimer) */
QPushButton[classe="danger"] {
    background-color: #FFFFFF;
    color: #DC2626;
    border: 1.5px solid #FCA5A5;
}
QPushButton[classe="danger"]:hover { background-color: #FEF2F2; }
QPushButton[classe="danger"]:pressed { background-color: #FEE2E2; }

/* Action secondaire (Modifier / Exporter / Dupliquer / Charge horaire...) */
QPushButton[classe="contour"] {
    background-color: #FFFFFF;
    color: #1E3A5F;
    border: 1.5px solid #CBD5E1;
}
QPushButton[classe="contour"]:hover {
    background-color: #F1F5F9;
    border: 1.5px solid #94A3B8;
}

/* Action discrete (Annuler) */
QPushButton[classe="discret"] { background-color: transparent; color: #64748B; }
QPushButton[classe="discret"]:hover { background-color: #E2E8F0; color: #334155; }

QPushButton:disabled { background-color: #F1F5F9; color: #94A3B8; }

/* ---------- Tableaux ---------- */
QTableView {
    background-color: #FFFFFF;
    alternate-background-color: #F8FAFC;
    gridline-color: #E2E8F0;
    border: 1px solid #E2E8F0;
    border-radius: 8px;
    selection-background-color: #DBEAFE;
    selection-color: #1E293B;
}
QTableView::item { padding: 6px; }
QHeaderView::section {
    background-color: #F8FAFC;
    color: #475569;
    font-weight: 700;
    padding: 10px 6px;
    border: none;
    border-bottom: 2px solid #E2E8F0;
}

/* ---------- Barres de defilement ---------- */
QScrollBar:vertical { background: transparent; width: 10px; }
QScrollBar::handle:vertical { background: #CBD5E1; border-radius: 5px; min-height: 24px; }
QScrollBar::handle:vertical:hover { background: #94A3B8; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

QToolTip {
    background-color: #1E3A5F;
    color: #FFFFFF;
    border: none;
    padding: 6px 10px;
    border-radius: 4px;
}
)";

int main(int argc, char *argv[])
{
    // Recommande par Qt des qu'un QQuickWidget est utilise (fond anime du
    // bandeau) : partage le contexte OpenGL entre tous les widgets pour
    // eviter tout scintillement au premier affichage.
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication a(argc, argv);
    a.setStyle("Fusion");
    a.setFont(QFont("Segoe UI", 10));
    a.setStyleSheet(QString::fromUtf8(FEUILLE_DE_STYLE));

    Connection c;
    bool ok = c.createconnect();

    if (!ok) {
        QMessageBox::critical(nullptr, QObject::tr("Connexion échouée"),
            QObject::tr("Impossible de se connecter à la base de données Oracle.\n"
                        "Vérifiez les paramètres dans connection.cpp."));
        return -1;
    }

    // Ecran de connexion affiche au demarrage : selon le compte reconnu,
    // route vers le dashboard (administrateur) ou le catalogue de cours
    // (etudiant). Le bouton "Se deconnecter" de chacune de ces fenetres
    // revient a cet ecran de connexion.
    std::function<void()> afficherConnexion;
    afficherConnexion = [&afficherConnexion]() {
        LoginWindow *login = new LoginWindow();
        login->setAttribute(Qt::WA_DeleteOnClose);

        QObject::connect(login, &LoginWindow::connexionAdminReussie, login,
            [login, &afficherConnexion](const QString &) {
                MainWindow *dashboard = new MainWindow();
                dashboard->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dashboard, &MainWindow::deconnexionDemandee, dashboard,
                    [dashboard, &afficherConnexion]() {
                        // Le nouvel ecran de connexion doit exister AVANT de
                        // fermer le dashboard, sinon Qt quitte l'application
                        // (plus aucune fenetre visible entre les deux).
                        afficherConnexion();
                        dashboard->close();
                    });
                dashboard->show();
                login->close();
            });

        QObject::connect(login, &LoginWindow::connexionEtudiantReussie, login,
            [login, &afficherConnexion](int idParticipant, const QString &nomComplet) {
                EtudiantMainWindow *portail = new EtudiantMainWindow(idParticipant, nomComplet);
                portail->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(portail, &EtudiantMainWindow::deconnexionDemandee, portail,
                    [portail, &afficherConnexion]() {
                        afficherConnexion();
                        portail->close();
                    });
                portail->show();
                login->close();
            });

        login->show();
    };

    afficherConnexion();

    return a.exec();
}
