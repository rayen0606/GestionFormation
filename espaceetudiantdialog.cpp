#include "espaceetudiantdialog.h"
#include "portailetudiant.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QSqlQueryModel>
#include <QComboBox>
#include <QFrame>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

EspaceEtudiantDialog::EspaceEtudiantDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Espace Étudiant"));
    resize(900, 620);

    QVBoxLayout *layout = new QVBoxLayout(this);
    stack = new QStackedWidget();
    stack->addWidget(creerPageAuthentification()); // index 0
    stack->addWidget(creerPageCatalogue());        // index 1
    layout->addWidget(stack);
}

// =====================================================================
//                        AUTHENTIFICATION
// =====================================================================

QWidget* EspaceEtudiantDialog::creerPageAuthentification()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->addStretch();

    QLabel *titre = new QLabel(tr("<h2>Espace Étudiant</h2>"));
    titre->setAlignment(Qt::AlignCenter);
    layout->addWidget(titre);

    QHBoxLayout *onglets = new QHBoxLayout();
    QPushButton *btnOngletConnexion = new QPushButton(tr("Se connecter"));
    QPushButton *btnOngletInscription = new QPushButton(tr("Créer un compte"));
    btnOngletConnexion->setCheckable(true);
    btnOngletInscription->setCheckable(true);
    btnOngletConnexion->setChecked(true);
    onglets->addStretch();
    onglets->addWidget(btnOngletConnexion);
    onglets->addWidget(btnOngletInscription);
    onglets->addStretch();
    layout->addLayout(onglets);

    authStack = new QStackedWidget();
    authStack->addWidget(creerPageConnexion());   // index 0
    authStack->addWidget(creerPageInscription());  // index 1

    connect(btnOngletConnexion, &QPushButton::clicked, this, [this, btnOngletConnexion, btnOngletInscription]() {
        btnOngletConnexion->setChecked(true);
        btnOngletInscription->setChecked(false);
        authStack->setCurrentIndex(0);
    });
    connect(btnOngletInscription, &QPushButton::clicked, this, [this, btnOngletConnexion, btnOngletInscription]() {
        btnOngletConnexion->setChecked(false);
        btnOngletInscription->setChecked(true);
        authStack->setCurrentIndex(1);
    });

    QHBoxLayout *centrage = new QHBoxLayout();
    centrage->addStretch();
    QWidget *conteneur = new QWidget();
    conteneur->setMaximumWidth(400);
    QVBoxLayout *conteneurLayout = new QVBoxLayout(conteneur);
    conteneurLayout->addWidget(authStack);
    centrage->addWidget(conteneur);
    centrage->addStretch();
    layout->addLayout(centrage);

    layout->addStretch();
    return page;
}

QWidget* EspaceEtudiantDialog::creerPageConnexion()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QFormLayout *form = new QFormLayout();
    loginEmailEdit = new QLineEdit();
    loginEmailEdit->setPlaceholderText(tr("exemple@mail.tn"));
    loginMotDePasseEdit = new QLineEdit();
    loginMotDePasseEdit->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Email :"), loginEmailEdit);
    form->addRow(tr("Mot de passe :"), loginMotDePasseEdit);
    layout->addLayout(form);

    loginErreurLabel = new QLabel();
    loginErreurLabel->setStyleSheet("color:#B91C1C;");
    loginErreurLabel->setWordWrap(true);
    layout->addWidget(loginErreurLabel);

    QPushButton *btnConnexion = new QPushButton(tr("Se connecter"));
    btnConnexion->setMinimumHeight(38);
    connect(btnConnexion, &QPushButton::clicked, this, &EspaceEtudiantDialog::onSeConnecter);
    connect(loginMotDePasseEdit, &QLineEdit::returnPressed, this, &EspaceEtudiantDialog::onSeConnecter);
    layout->addWidget(btnConnexion);
    layout->addStretch();
    return page;
}

QWidget* EspaceEtudiantDialog::creerPageInscription()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QFormLayout *form = new QFormLayout();
    regNomEdit = new QLineEdit();
    regPrenomEdit = new QLineEdit();
    regEmailEdit = new QLineEdit();
    regEmailEdit->setPlaceholderText(tr("exemple@mail.tn"));
    regTelEdit = new QLineEdit();
    regTelEdit->setPlaceholderText(tr("8 chiffres"));
    regMotDePasseEdit = new QLineEdit();
    regMotDePasseEdit->setEchoMode(QLineEdit::Password);
    regMotDePasseConfirmEdit = new QLineEdit();
    regMotDePasseConfirmEdit->setEchoMode(QLineEdit::Password);

    form->addRow(tr("Nom :"), regNomEdit);
    form->addRow(tr("Prénom :"), regPrenomEdit);
    form->addRow(tr("Email :"), regEmailEdit);
    form->addRow(tr("Téléphone :"), regTelEdit);
    form->addRow(tr("Mot de passe :"), regMotDePasseEdit);
    form->addRow(tr("Confirmer :"), regMotDePasseConfirmEdit);
    layout->addLayout(form);

    regErreurLabel = new QLabel();
    regErreurLabel->setStyleSheet("color:#B91C1C;");
    regErreurLabel->setWordWrap(true);
    layout->addWidget(regErreurLabel);

    QPushButton *btnCreer = new QPushButton(tr("Créer mon compte"));
    btnCreer->setMinimumHeight(38);
    connect(btnCreer, &QPushButton::clicked, this, &EspaceEtudiantDialog::onCreerCompte);
    layout->addWidget(btnCreer);
    layout->addStretch();
    return page;
}

void EspaceEtudiantDialog::onSeConnecter()
{
    QString email = loginEmailEdit->text().trimmed();
    QString motDePasse = loginMotDePasseEdit->text();

    if (email.isEmpty() || motDePasse.isEmpty()) {
        loginErreurLabel->setText(tr("Veuillez saisir votre email et votre mot de passe."));
        return;
    }

    int idParticipant = -1;
    QString nomComplet;
    if (!PortailEtudiant::seConnecter(email, motDePasse, idParticipant, nomComplet)) {
        loginErreurLabel->setText(tr("Email ou mot de passe incorrect."));
        return;
    }

    loginErreurLabel->clear();
    idParticipantConnecte = idParticipant;
    bienvenueLabel->setText(tr("Bonjour, %1").arg(nomComplet));
    rafraichirCatalogue();
    stack->setCurrentIndex(1);
}

void EspaceEtudiantDialog::onCreerCompte()
{
    static const QRegularExpression emailRegex(R"(^[\w.+-]+@[\w-]+\.[A-Za-z]{2,}$)");
    static const QRegularExpression nomRegex(R"(^[A-Za-zÀ-ÿ\s'-]{2,50}$)");
    static const QRegularExpression telRegex(R"(^\d{8}$)");

    QString nom = regNomEdit->text().trimmed();
    QString prenom = regPrenomEdit->text().trimmed();
    QString email = regEmailEdit->text().trimmed();
    QString tel = regTelEdit->text().trimmed();
    QString motDePasse = regMotDePasseEdit->text();
    QString confirmation = regMotDePasseConfirmEdit->text();

    if (!nomRegex.match(nom).hasMatch() || !nomRegex.match(prenom).hasMatch()) {
        regErreurLabel->setText(tr("Le nom et le prénom ne doivent contenir que des lettres."));
        return;
    }
    if (!emailRegex.match(email).hasMatch()) {
        regErreurLabel->setText(tr("Adresse email invalide."));
        return;
    }
    if (!tel.isEmpty() && !telRegex.match(tel).hasMatch()) {
        regErreurLabel->setText(tr("Le téléphone doit contenir exactement 8 chiffres."));
        return;
    }
    if (motDePasse.length() < 6) {
        regErreurLabel->setText(tr("Le mot de passe doit contenir au moins 6 caractères."));
        return;
    }
    if (motDePasse != confirmation) {
        regErreurLabel->setText(tr("Les deux mots de passe ne correspondent pas."));
        return;
    }

    int idParticipant = -1;
    QString erreur = PortailEtudiant::creerCompte(nom, prenom, email, tel, motDePasse, idParticipant);
    if (!erreur.isEmpty()) {
        regErreurLabel->setText(erreur);
        return;
    }

    regErreurLabel->clear();
    idParticipantConnecte = idParticipant;
    bienvenueLabel->setText(tr("Bonjour, %1 %2").arg(prenom, nom));
    QMessageBox::information(this, tr("Compte créé"), tr("Votre compte a été créé avec succès !"));
    rafraichirCatalogue();
    stack->setCurrentIndex(1);
}

void EspaceEtudiantDialog::onSeDeconnecter()
{
    idParticipantConnecte = -1;
    loginEmailEdit->clear();
    loginMotDePasseEdit->clear();
    loginErreurLabel->clear();
    regNomEdit->clear(); regPrenomEdit->clear(); regEmailEdit->clear(); regTelEdit->clear();
    regMotDePasseEdit->clear(); regMotDePasseConfirmEdit->clear(); regErreurLabel->clear();
    stack->setCurrentIndex(0);
}

// =====================================================================
//                       CATALOGUE EN GRILLE
// =====================================================================

QWidget* EspaceEtudiantDialog::creerPageCatalogue()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);

    QHBoxLayout *ligneHaut = new QHBoxLayout();
    bienvenueLabel = new QLabel();
    bienvenueLabel->setStyleSheet("font-size:13pt; font-weight:600; color:#1E3A5F;");
    QPushButton *btnDeconnexion = new QPushButton(tr("Se déconnecter"));
    connect(btnDeconnexion, &QPushButton::clicked, this, &EspaceEtudiantDialog::onSeDeconnecter);
    ligneHaut->addWidget(bienvenueLabel);
    ligneHaut->addStretch();
    ligneHaut->addWidget(btnDeconnexion);
    layout->addLayout(ligneHaut);

    QLabel *info = new QLabel(tr("Parcourez le catalogue et payez pour débloquer la lecture et le téléchargement."));
    layout->addWidget(info);

    catalogueScroll = new QScrollArea();
    catalogueScroll->setWidgetResizable(true);
    catalogueScroll->setFrameShape(QFrame::NoFrame);

    catalogueConteneur = new QWidget();
    catalogueGrille = new QGridLayout(catalogueConteneur);
    catalogueGrille->setSpacing(16);
    catalogueScroll->setWidget(catalogueConteneur);

    layout->addWidget(catalogueScroll);
    return page;
}

QWidget* EspaceEtudiantDialog::creerCarteCours(int idCours, const QString &titre, const QString &niveau,
                                                int duree, double prix, const QString &description, bool paye)
{
    QFrame *carte = new QFrame();
    carte->setFrameShape(QFrame::StyledPanel);
    carte->setMinimumSize(260, 220);
    carte->setMaximumWidth(300);
    carte->setStyleSheet(
        "QFrame { background:#FFFFFF; border:1px solid #DCE3EA; border-radius:10px; }"
        "QFrame:hover { border:1px solid #2F6FED; }");

    QVBoxLayout *layout = new QVBoxLayout(carte);

    QLabel *titreLabel = new QLabel(titre);
    titreLabel->setWordWrap(true);
    titreLabel->setStyleSheet("font-size:12pt; font-weight:700; color:#1E3A5F;");
    layout->addWidget(titreLabel);

    QLabel *metaLabel = new QLabel(tr("%1 · %2 h").arg(niveau).arg(duree));
    metaLabel->setStyleSheet("color:#6B7A90; font-size:9pt;");
    layout->addWidget(metaLabel);

    QLabel *descLabel = new QLabel(description.isEmpty() ? tr("Aucune description.") : description);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color:#374151; font-size:9pt;");
    descLabel->setMaximumHeight(70);
    layout->addWidget(descLabel, 1);

    QLabel *prixLabel = new QLabel(tr("%1 DT").arg(QString::number(prix, 'f', 2)));
    prixLabel->setStyleSheet("font-size:11pt; font-weight:700; color:#1E3A5F;");
    layout->addWidget(prixLabel);

    if (paye) {
        QLabel *badge = new QLabel(tr("✔ Payé"));
        badge->setStyleSheet("color:#15803D; font-weight:600;");
        layout->addWidget(badge);

        QHBoxLayout *actions = new QHBoxLayout();
        QPushButton *btnLire = new QPushButton(tr("Lire"));
        QPushButton *btnTelecharger = new QPushButton(tr("Télécharger"));
        btnLire->setStyleSheet("background:#2F6FED; color:white; border-radius:6px; padding:6px;");
        btnTelecharger->setStyleSheet("border:1px solid #2F6FED; color:#2F6FED; border-radius:6px; padding:6px;");
        connect(btnLire, &QPushButton::clicked, this, [this, idCours]() { onOuvrirCours(idCours); });
        connect(btnTelecharger, &QPushButton::clicked, this, [this, idCours]() { onTelechargerCours(idCours); });
        actions->addWidget(btnLire);
        actions->addWidget(btnTelecharger);
        layout->addLayout(actions);
    } else {
        QPushButton *btnPayer = new QPushButton(tr("Payer et débloquer"));
        btnPayer->setStyleSheet("background:#1E3A5F; color:white; border-radius:6px; padding:8px; font-weight:600;");
        connect(btnPayer, &QPushButton::clicked, this, [this, idCours, titre, prix]() { onPayerCours(idCours, titre, prix); });
        layout->addWidget(btnPayer);
    }

    return carte;
}

void EspaceEtudiantDialog::rafraichirCatalogue()
{
    // Vide la grille existante avant de la reconstruire.
    QLayoutItem *item;
    while ((item = catalogueGrille->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    if (idParticipantConnecte < 0) return;

    QSqlQueryModel *model = PortailEtudiant::catalogueCours(idParticipantConnecte);
    const int colonnes = 3;
    for (int i = 0; i < model->rowCount(); ++i) {
        int idCours = model->data(model->index(i, 0)).toInt();
        QString titre = model->data(model->index(i, 1)).toString();
        QString niveau = model->data(model->index(i, 2)).toString();
        int duree = model->data(model->index(i, 3)).toInt();
        double prix = model->data(model->index(i, 4)).toDouble();
        QString description = model->data(model->index(i, 5)).toString();
        bool paye = model->data(model->index(i, 6)).toInt() == 1;

        QWidget *carte = creerCarteCours(idCours, titre, niveau, duree, prix, description, paye);
        catalogueGrille->addWidget(carte, i / colonnes, i % colonnes);
    }

    if (model->rowCount() == 0) {
        QLabel *vide = new QLabel(tr("Aucun cours disponible pour le moment."));
        catalogueGrille->addWidget(vide, 0, 0);
    }
}

// =====================================================================
//                             PAIEMENT
// =====================================================================

void EspaceEtudiantDialog::onPayerCours(int idCours, const QString &titre, double prix)
{
    QDialog dialogue(this);
    dialogue.setWindowTitle(tr("Paiement"));
    dialogue.setMinimumWidth(360);

    QVBoxLayout *layout = new QVBoxLayout(&dialogue);
    QLabel *recap = new QLabel(tr("<b>%1</b><br>Montant à régler : %2 DT")
        .arg(titre, QString::number(prix, 'f', 2)));
    layout->addWidget(recap);

    QFormLayout *form = new QFormLayout();
    QComboBox *modeCombo = new QComboBox();
    modeCombo->addItems({tr("Carte bancaire"), tr("Virement"), tr("Espèces"), tr("Chèque")});
    form->addRow(tr("Mode de paiement :"), modeCombo);
    layout->addLayout(form);

    QDialogButtonBox *boutons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    boutons->button(QDialogButtonBox::Ok)->setText(tr("Confirmer le paiement"));
    connect(boutons, &QDialogButtonBox::accepted, &dialogue, &QDialog::accept);
    connect(boutons, &QDialogButtonBox::rejected, &dialogue, &QDialog::reject);
    layout->addWidget(boutons);

    if (dialogue.exec() != QDialog::Accepted) return;

    QString erreur = PortailEtudiant::payerCours(idParticipantConnecte, idCours, modeCombo->currentText());
    if (!erreur.isEmpty()) {
        QMessageBox::warning(this, tr("Paiement impossible"), erreur);
        return;
    }

    QMessageBox::information(this, tr("Paiement réussi"),
        tr("Le paiement a été effectué. Vous pouvez maintenant lire et télécharger ce cours."));
    rafraichirCatalogue();
}

// =====================================================================
//                    LECTURE / TÉLÉCHARGEMENT
// =====================================================================

void EspaceEtudiantDialog::onOuvrirCours(int idCours)
{
    ouvrirOuTelechargerCours(idCours, false);
}

void EspaceEtudiantDialog::onTelechargerCours(int idCours)
{
    ouvrirOuTelechargerCours(idCours, true);
}

void EspaceEtudiantDialog::ouvrirOuTelechargerCours(int idCours, bool telecharger)
{
    if (!PortailEtudiant::aPaye(idParticipantConnecte, idCours)) {
        QMessageBox::warning(this, tr("Accès refusé"), tr("Vous devez d'abord payer ce cours."));
        return;
    }

    QString nomFichier; QByteArray contenu;
    if (!cours.lireFichier(idCours, nomFichier, contenu)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible de récupérer le support de cours."));
        return;
    }
    if (nomFichier.isEmpty() || contenu.isEmpty()) {
        QMessageBox::information(this, tr("Aucun support"),
            tr("Aucun support n'a encore été mis en ligne pour ce cours."));
        return;
    }

    if (telecharger) {
        QString cheminDestination = QFileDialog::getSaveFileName(this, tr("Enregistrer le support de cours"), nomFichier);
        if (cheminDestination.isEmpty()) return;

        QFile fichier(cheminDestination);
        if (!fichier.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Erreur"), tr("Impossible d'enregistrer le fichier."));
            return;
        }
        fichier.write(contenu);
        fichier.close();
        QMessageBox::information(this, tr("Téléchargement terminé"), tr("Le fichier a été enregistré avec succès."));
        return;
    }

    static const QStringList extensionsTexte = {"txt", "csv", "md", "log"};
    QString extension = QFileInfo(nomFichier).suffix().toLower();

    if (extensionsTexte.contains(extension)) {
        // Formats texte : affichage direct dans une fenêtre de lecture,
        // sans jamais quitter l'application.
        QDialog dialogue(this);
        dialogue.setWindowTitle(tr("Lecture : %1").arg(nomFichier));
        dialogue.resize(600, 500);

        QVBoxLayout *layout = new QVBoxLayout(&dialogue);
        QLabel *titreLabel = new QLabel(tr("<b>%1</b>").arg(nomFichier));
        layout->addWidget(titreLabel);

        QTextEdit *zoneTexte = new QTextEdit();
        zoneTexte->setPlainText(QString::fromUtf8(contenu));
        zoneTexte->setReadOnly(true);
        layout->addWidget(zoneTexte);

        QDialogButtonBox *boutons = new QDialogButtonBox(QDialogButtonBox::Ok);
        connect(boutons, &QDialogButtonBox::accepted, &dialogue, &QDialog::accept);
        layout->addWidget(boutons);

        dialogue.exec();
    } else {
        // Autres formats (PDF, Word...) : fichier temporaire ouvert avec
        // l'application associée du système.
        QString dossierTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString cheminTemp = QDir(dossierTemp).filePath(nomFichier);

        QFile fichierTemp(cheminTemp);
        if (!fichierTemp.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr("Erreur"), tr("Impossible de préparer le fichier pour la lecture."));
            return;
        }
        fichierTemp.write(contenu);
        fichierTemp.close();

        QDesktopServices::openUrl(QUrl::fromLocalFile(cheminTemp));
    }
}
