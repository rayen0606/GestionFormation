#include "logindialog.h"
#include "utilisateur.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QStyle>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Connexion — Centre de Formation"));
    setMinimumWidth(380);

    auto *layout = new QVBoxLayout(this);

    auto *titre = new QLabel(tr("Centre de Formation"));
    titre->setStyleSheet("font-size:18pt; font-weight:700; color:#1E3A5F;");
    titre->setAlignment(Qt::AlignCenter);
    layout->addWidget(titre);

    auto *sousTitre = new QLabel(tr("Veuillez vous connecter pour continuer"));
    sousTitre->setStyleSheet("color:#64748B;");
    sousTitre->setAlignment(Qt::AlignCenter);
    layout->addWidget(sousTitre);
    layout->addSpacing(16);

    auto *form = new QFormLayout();
    loginEdit = new QLineEdit();
    loginEdit->setPlaceholderText(tr("admin"));
    motDePasseEdit = new QLineEdit();
    motDePasseEdit->setEchoMode(QLineEdit::Password);
    motDePasseEdit->setPlaceholderText(tr("••••••••"));

    form->addRow(tr("Identifiant :"), loginEdit);
    form->addRow(tr("Mot de passe :"), motDePasseEdit);
    layout->addLayout(form);

    erreurLabel = new QLabel();
    erreurLabel->setStyleSheet("color:#DC2626; font-weight:600;");
    erreurLabel->setWordWrap(true);
    erreurLabel->hide();
    layout->addWidget(erreurLabel);

    auto *boutons = new QDialogButtonBox();
    auto *btnQuitter = boutons->addButton(tr("Quitter"), QDialogButtonBox::RejectRole);
    auto *btnConnecter = boutons->addButton(tr("  Se connecter"), QDialogButtonBox::AcceptRole);
    btnConnecter->setProperty("classe", "primaire");
    btnConnecter->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));
    btnConnecter->setDefault(true);

    connect(btnQuitter, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnConnecter, &QPushButton::clicked, this, &LoginDialog::onSeConnecter);
    connect(motDePasseEdit, &QLineEdit::returnPressed, this, &LoginDialog::onSeConnecter);

    layout->addWidget(boutons);
}

void LoginDialog::onSeConnecter()
{
    QString login = loginEdit->text().trimmed();
    QString motDePasse = motDePasseEdit->text();

    if (login.isEmpty() || motDePasse.isEmpty()) {
        erreurLabel->setText(tr("Veuillez renseigner l'identifiant et le mot de passe."));
        erreurLabel->show();
        return;
    }

    QString role;
    if (Utilisateur::authentifier(login, motDePasse, role)) {
        m_login = login;
        m_role = role;
        accept();
    } else {
        erreurLabel->setText(tr("Identifiant ou mot de passe incorrect."));
        erreurLabel->show();
        motDePasseEdit->clear();
        motDePasseEdit->setFocus();
    }
}
