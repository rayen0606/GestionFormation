#include "loginwindow.h"
#include "administrateur.h"
#include "portailetudiant.h"
#include "uiutils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QRegularExpression>
#include <QGraphicsDropShadowEffect>
#include <QFrame>
#include <QScreen>
#include <QGuiApplication>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Centre de Formation — Connexion"));
    resize(1080, 650);

    // Centre la fenêtre sur l'écran au démarrage.
    if (QScreen *ecran = QGuiApplication::primaryScreen()) {
        QRect zone = ecran->availableGeometry();
        move(zone.center() - QPoint(width() / 2, height() / 2));
    }

    setStyleSheet("LoginWindow { background:#F1F5F9; }");

    QHBoxLayout *layoutPrincipal = new QHBoxLayout(this);
    layoutPrincipal->setContentsMargins(0, 0, 0, 0);
    layoutPrincipal->setSpacing(0);

    layoutPrincipal->addWidget(creerPanneauBranding(), 5);

    // ---- Panneau droit : formulaire ----
    QWidget *panneauFormulaire = new QWidget();
    panneauFormulaire->setStyleSheet("background:#F1F5F9;");
    QVBoxLayout *layoutFormulaire = new QVBoxLayout(panneauFormulaire);
    layoutFormulaire->addStretch();

    carteAuth = new QFrame();
    carteAuth->setObjectName("carteAuth");
    carteAuth->setAttribute(Qt::WA_StyledBackground, true);
    carteAuth->setMaximumWidth(400);
    carteAuth->setStyleSheet(
        "#carteAuth { background:white; border-radius:16px; }"
        "#carteAuth QLineEdit { border:1px solid #DCE3EA; border-radius:8px; padding:9px 12px; "
        "  font-size:10pt; background:#F8FAFC; }"
        "#carteAuth QLineEdit:focus { border:1.5px solid #2F6FED; background:white; }"
        "#carteAuth QLabel#champLabel { color:#374151; font-weight:600; font-size:9pt; }");

    QVBoxLayout *layoutCarte = new QVBoxLayout(carteAuth);
    layoutCarte->setContentsMargins(32, 32, 32, 32);
    layoutCarte->setSpacing(14);

    QLabel *titreCarte = new QLabel(tr("Bienvenue"));
    titreCarte->setStyleSheet("font-size:18pt; font-weight:800; color:#1E3A5F;");
    layoutCarte->addWidget(titreCarte);

    QLabel *sousTitreCarte = new QLabel(tr("Connectez-vous ou créez un compte étudiant."));
    sousTitreCarte->setStyleSheet("color:#6B7A90; font-size:9.5pt;");
    sousTitreCarte->setWordWrap(true);
    layoutCarte->addWidget(sousTitreCarte);

    // ---- Onglets segmentés ----
    QFrame *segmentBox = new QFrame();
    segmentBox->setObjectName("segmentBox");
    segmentBox->setAttribute(Qt::WA_StyledBackground, true);
    segmentBox->setStyleSheet("#segmentBox { background:#EEF2F6; border-radius:9px; }");
    QHBoxLayout *segmentLayout = new QHBoxLayout(segmentBox);
    segmentLayout->setContentsMargins(4, 4, 4, 4);
    segmentLayout->setSpacing(4);

    btnOngletConnexion = new QPushButton(tr("Se connecter"));
    btnOngletInscription = new QPushButton(tr("Créer un compte"));
    for (QPushButton *b : {btnOngletConnexion, btnOngletInscription}) {
        b->setCheckable(true);
        b->setMinimumHeight(34);
        b->setCursor(Qt::PointingHandCursor);
    }
    btnOngletConnexion->setChecked(true);

    QString styleSegmentActif =
        "QPushButton { background:white; color:#1E3A5F; border-radius:7px; font-weight:700; font-size:9.5pt; }";
    QString styleSegmentInactif =
        "QPushButton { background:transparent; color:#6B7A90; border-radius:7px; font-weight:600; font-size:9.5pt; }"
        "QPushButton:hover { color:#1E3A5F; }";
    btnOngletConnexion->setStyleSheet(styleSegmentActif);
    btnOngletInscription->setStyleSheet(styleSegmentInactif);

    connect(btnOngletConnexion, &QPushButton::clicked, this, [this, styleSegmentActif, styleSegmentInactif]() {
        btnOngletConnexion->setChecked(true);
        btnOngletInscription->setChecked(false);
        btnOngletConnexion->setStyleSheet(styleSegmentActif);
        btnOngletInscription->setStyleSheet(styleSegmentInactif);
        onBasculerOnglet(0);
    });
    connect(btnOngletInscription, &QPushButton::clicked, this, [this, styleSegmentActif, styleSegmentInactif]() {
        btnOngletConnexion->setChecked(false);
        btnOngletInscription->setChecked(true);
        btnOngletInscription->setStyleSheet(styleSegmentActif);
        btnOngletConnexion->setStyleSheet(styleSegmentInactif);
        onBasculerOnglet(1);
    });

    segmentLayout->addWidget(btnOngletConnexion);
    segmentLayout->addWidget(btnOngletInscription);
    layoutCarte->addWidget(segmentBox);

    authStack = new QStackedWidget();
    authStack->addWidget(creerPageConnexion());   // index 0
    authStack->addWidget(creerPageInscription());  // index 1
    layoutCarte->addWidget(authStack);

    layoutFormulaire->addWidget(carteAuth, 0, Qt::AlignHCenter);
    layoutFormulaire->addStretch();
    layoutPrincipal->addWidget(panneauFormulaire, 4);

    UiUtils::animerEntree(carteAuth, 80, 420, [this]() {
        // L'ombre n'est appliquée qu'une fois le fondu terminé : la carte
        // ne doit jamais porter deux effets graphiques en même temps
        // (l'effet d'opacité du fondu, puis l'ombre portée).
        UiUtils::appliquerOmbre(carteAuth, 40, 14, 35);
    });
}

QWidget* LoginWindow::creerPanneauBranding()
{
    QFrame *panneau = new QFrame();
    panneau->setObjectName("panneauBranding");
    panneau->setAttribute(Qt::WA_StyledBackground, true);
    panneau->setStyleSheet(
        "#panneauBranding { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #1E3A5F, stop:0.55 #2A4E7C, stop:1 #2F6FED); }");

    QVBoxLayout *layout = new QVBoxLayout(panneau);
    layout->setContentsMargins(56, 56, 56, 56);
    layout->addStretch(2);

    QLabel *icone = new QLabel("🎓");
    icone->setStyleSheet("font-size:46pt;");
    layout->addWidget(icone);

    QLabel *titre = new QLabel(tr("Centre de\nFormation"));
    titre->setStyleSheet("color:white; font-size:26pt; font-weight:800;");
    layout->addWidget(titre);

    QLabel *tagline = new QLabel(tr("Apprenez à votre rythme, avec des formateurs\nexperts et des cours certifiants."));
    tagline->setStyleSheet("color:#CBDCF5; font-size:11pt;");
    tagline->setWordWrap(true);
    layout->addWidget(tagline);

    layout->addSpacing(28);

    const QStringList atouts = {
        tr("✓  Cours accessibles à vie après paiement"),
        tr("✓  Lecture intégrée et téléchargement des supports"),
        tr("✓  Suivi de vos inscriptions en un clin d'œil"),
    };
    for (const QString &texte : atouts) {
        QLabel *l = new QLabel(texte);
        l->setStyleSheet("color:#E7EFFC; font-size:10pt; padding:4px 0;");
        layout->addWidget(l);
    }

    layout->addStretch(3);
    return panneau;
}

QWidget* LoginWindow::creerPageConnexion()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(10);

    QLabel *labelEmail = new QLabel(tr("Email"));
    labelEmail->setObjectName("champLabel");
    loginEmailEdit = new QLineEdit();
    loginEmailEdit->setPlaceholderText(tr("exemple@mail.tn"));
    layout->addWidget(labelEmail);
    layout->addWidget(loginEmailEdit);

    QLabel *labelMdp = new QLabel(tr("Mot de passe"));
    labelMdp->setObjectName("champLabel");
    loginMotDePasseEdit = new QLineEdit();
    loginMotDePasseEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(labelMdp);
    layout->addWidget(loginMotDePasseEdit);

    loginErreurLabel = new QLabel();
    loginErreurLabel->setStyleSheet("color:#B91C1C; font-size:9pt;");
    loginErreurLabel->setWordWrap(true);
    layout->addWidget(loginErreurLabel);

    QPushButton *btnConnexion = new QPushButton(tr("Se connecter"));
    btnConnexion->setCursor(Qt::PointingHandCursor);
    btnConnexion->setMinimumHeight(42);
    btnConnexion->setStyleSheet(
        "QPushButton { background:#1E3A5F; color:white; font-weight:700; font-size:10pt; border-radius:9px; }"
        "QPushButton:hover { background:#2A4E7C; }"
        "QPushButton:pressed { background:#16304F; }");
    connect(btnConnexion, &QPushButton::clicked, this, &LoginWindow::onSeConnecter);
    connect(loginMotDePasseEdit, &QLineEdit::returnPressed, this, &LoginWindow::onSeConnecter);
    layout->addSpacing(4);
    layout->addWidget(btnConnexion);

    QLabel *astuce = new QLabel(tr("Compte administrateur par défaut : admin@centre.tn"));
    astuce->setStyleSheet("color:#9AA6B5; font-size:8pt;");
    astuce->setAlignment(Qt::AlignCenter);
    layout->addWidget(astuce);

    return page;
}

QWidget* LoginWindow::creerPageInscription()
{
    QWidget *page = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(8);

    QLabel *info = new QLabel(tr("La création de compte est réservée aux étudiants."));
    info->setStyleSheet("color:#9AA6B5; font-size:8.5pt;");
    info->setWordWrap(true);
    layout->addWidget(info);

    auto ajouterChamp = [&](const QString &libelle, QLineEdit *&champ, bool motDePasse = false) {
        QLabel *l = new QLabel(libelle);
        l->setObjectName("champLabel");
        champ = new QLineEdit();
        if (motDePasse) champ->setEchoMode(QLineEdit::Password);
        layout->addWidget(l);
        layout->addWidget(champ);
    };

    QHBoxLayout *ligneNomPrenom = new QHBoxLayout();
    QVBoxLayout *colNom = new QVBoxLayout();
    QLabel *lNom = new QLabel(tr("Nom")); lNom->setObjectName("champLabel");
    regNomEdit = new QLineEdit();
    colNom->addWidget(lNom); colNom->addWidget(regNomEdit);
    QVBoxLayout *colPrenom = new QVBoxLayout();
    QLabel *lPrenom = new QLabel(tr("Prénom")); lPrenom->setObjectName("champLabel");
    regPrenomEdit = new QLineEdit();
    colPrenom->addWidget(lPrenom); colPrenom->addWidget(regPrenomEdit);
    ligneNomPrenom->addLayout(colNom);
    ligneNomPrenom->addLayout(colPrenom);
    layout->addLayout(ligneNomPrenom);

    ajouterChamp(tr("Email"), regEmailEdit);
    regEmailEdit->setPlaceholderText(tr("exemple@mail.tn"));
    ajouterChamp(tr("Téléphone"), regTelEdit);
    regTelEdit->setPlaceholderText(tr("8 chiffres"));
    ajouterChamp(tr("Mot de passe"), regMotDePasseEdit, true);
    ajouterChamp(tr("Confirmer le mot de passe"), regMotDePasseConfirmEdit, true);

    regErreurLabel = new QLabel();
    regErreurLabel->setStyleSheet("color:#B91C1C; font-size:9pt;");
    regErreurLabel->setWordWrap(true);
    layout->addWidget(regErreurLabel);

    QPushButton *btnCreer = new QPushButton(tr("Créer mon compte"));
    btnCreer->setCursor(Qt::PointingHandCursor);
    btnCreer->setMinimumHeight(42);
    btnCreer->setStyleSheet(
        "QPushButton { background:#1E3A5F; color:white; font-weight:700; font-size:10pt; border-radius:9px; }"
        "QPushButton:hover { background:#2A4E7C; }"
        "QPushButton:pressed { background:#16304F; }");
    connect(btnCreer, &QPushButton::clicked, this, &LoginWindow::onCreerCompte);
    layout->addSpacing(4);
    layout->addWidget(btnCreer);

    return page;
}

void LoginWindow::onBasculerOnglet(int index)
{
    // Pas d'effet d'opacité ici : "authStack" est un descendant de
    // "carteAuth", qui porte déjà une ombre portée. Superposer un second
    // QGraphicsEffect sur un widget enfant pendant que l'ancêtre en a un
    // (ou vient d'en avoir un) casse le rendu sur certaines configurations
    // (formulaire qui apparaît totalement blanc). Le changement de page
    // reste instantané, ce qui est de toute façon plus réactif au clic.
    authStack->setCurrentIndex(index);
}

void LoginWindow::onSeConnecter()
{
    QString email = loginEmailEdit->text().trimmed();
    QString motDePasse = loginMotDePasseEdit->text();

    if (email.isEmpty() || motDePasse.isEmpty()) {
        loginErreurLabel->setText(tr("Veuillez saisir votre email et votre mot de passe."));
        return;
    }

    // 1. Compte administrateur ?
    QString nomAdmin;
    if (Administrateur::authentifier(email, motDePasse, nomAdmin)) {
        loginErreurLabel->clear();
        emit connexionAdminReussie(nomAdmin);
        return;
    }

    // 2. Compte étudiant (participant) ?
    int idParticipant = -1;
    QString nomComplet;
    if (PortailEtudiant::seConnecter(email, motDePasse, idParticipant, nomComplet)) {
        loginErreurLabel->clear();
        emit connexionEtudiantReussie(idParticipant, nomComplet);
        return;
    }

    loginErreurLabel->setText(tr("Email ou mot de passe incorrect."));
}

void LoginWindow::onCreerCompte()
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
    QMessageBox::information(this, tr("Compte créé"), tr("Votre compte a été créé avec succès !"));
    emit connexionEtudiantReussie(idParticipant, tr("%1 %2").arg(prenom, nom));
}
