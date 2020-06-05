// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Sierra Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "overviewpage.h"
#include "ui_overviewpage.h"
#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "init.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "utilitydialog.h"
#include "walletmodel.h"
#include "modaloverlay.h"
#include "validation.h"
#include "instantx.h"
#include "masternode-sync.h"
#include "privatesend-client.h"
#include "consensus/consensus.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QTimer>

#define ICON_OFFSET 15
#define DECORATION_SIZE 50
#define NUM_ITEMS 7

extern int64_t nLastCoinStakeSearchInterval;

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
            QAbstractItemDelegate(), unit(BitcoinUnits::SIERRA),
            platformStyle(_platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QRect mainRect = option.rect;
        mainRect.moveLeft(ICON_OFFSET);
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon = platformStyle->SingleColorIcon(icon);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::floorWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
    const PlatformStyle *platformStyle;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
        QWidget(parent),
        timer(nullptr),
        ui(new Ui::OverviewPage),
        clientModel(0),
        walletModel(0),
        currentBalance(-1),
        currentUnconfirmedBalance(-1),
        currentStakeInputs(-1),
        currentStakeBalance(-1),
        currentImmatureBalance(-1),
        currentWatchOnlyBalance(-1),
        currentWatchUnconfBalance(-1),
        currentWatchImmatureBalance(-1),
        txdelegate(new TxViewDelegate(platformStyle, this))
{
    ui->setupUi(this);
    QString theme = GUIUtil::getThemeName();

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    // Note: minimum height of listTransactions will be set later in updateAdvancedPSUI() to reflect actual settings
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));


    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& anonymizedBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance, const CAmount& stakeBalance, const int stakeInputs)
{
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    currentStakeBalance = stakeBalance;
    currentStakeInputs = stakeInputs;
    ui->labelBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, balance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelStakeBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, stakeBalance, false, BitcoinUnits::separatorAlways));

    uint64_t coins = nMinimumStakeValue / COIN;
    uint64_t conf = COINBASE_MATURITY + 1;
    uint64_t age = CurrentMinStakeAge(GetAdjustedTime());
    QString stakeInputsTooltip =
        tr("Number of inputs eligible for staking (having at least %1 coins, at least %2 confirmations and age of %3 or more)")
            .arg(QString::number(coins))
            .arg(QString::number(conf))
            .arg(GUIUtil::formatDurationStr(age));
    ui->labelStakeInputs->setText(QString::number(stakeInputs));
    ui->labelStakeInputs->setToolTip(stakeInputsTooltip);

    ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, balance + unconfirmedBalance + immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchAvailable->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchPending->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, BitcoinUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance
}

QString OverviewPage::getStakeStatus()
{
    if (!GetBoolArg("-staking", DEFAULT_STAKING))
        return tr("<font color='darkred'>Not configured</font>");
    if (chainActive.Height() < Params().GetConsensus().nLastPoWBlock)
        return tr("<font color='darkred'>PoW phase</font>");
    if (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) == 0)
        return tr("<font color='darkred'>Not connected to peers</font>");
    if (!masternodeSync.IsSynced())
        return tr("<font color='darkred'>Masternode list not synced</font>");
    if (pwalletMain->IsLocked(true))
        return tr("<font color='darkred'>Wallet is locked</font>");
    if (pwalletMain->GetMintableCoins() == 0)
        return tr("<font color='darkred'>No mintable inputs</font>");
    return tr("<font color='darkgreen'>Staking</font>");
}

QString OverviewPage::getAvgBlockTime()
{
    auto n = Params().GetConsensus().nPowTargetSpacing;
    auto h = (60 * 60) / n;
    auto d = (60 * 60 * 24) / n;
    QString avgH = GUIUtil::formatDurationStr(chainActive.GetAvgBlockTime(h));
    QString avgD = GUIUtil::formatDurationStr(chainActive.GetAvgBlockTime(d));
    return avgH + " /  " + avgD;
}

void OverviewPage::setBlockChainInfo(int count, const QDateTime& blockDate, double nVerificationProgress, bool headers)
{
    // Update staking status each block
    if (!headers) {
        ui->labelBlocks->setText(QString::number(count));
        if (masternodeSync.IsSynced()) {
            ui->labelAvgBlockTime->setText(getAvgBlockTime());
            ui->labelStakeStatus->setText(getStakeStatus());
        }
    }
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly){
        ui->labelWatchImmature->hide();
    }
    else{
        ui->labelBalance->setIndent(20);
        ui->labelUnconfirmed->setIndent(20);
        ui->labelImmature->setIndent(20);
        ui->labelTotal->setIndent(20);
    }
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
        setBlockChainInfo(model->getNumBlocks(), model->getLastBlockDate(), model->getVerificationProgress(NULL), false);
        connect(model, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(setBlockChainInfo(int,QDateTime,double,bool)));

    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // update the display unit, to not use the default ("SIERRA")
        updateDisplayUnit();
        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getAnonymizedBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(), model->getStakeBalance(), model->getStakeInputs());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount, CAmount, int)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount, CAmount, int)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
        SetupTransactionList();
    }
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentAnonymizedBalance,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance, currentStakeBalance, currentStakeInputs);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = nDisplayUnit;

        ui->listTransactions->update();

        ui->labelAvgBlockTime->setText(tr("<font color='darkred'>Syncing...</font>") );
        ui->labelStatus->setText(tr("<font color='darkred'>Syncing...</font>") );
        ui->labelMnList->setText(tr("<font color='darkred'>Syncing...</font>") );
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelStatus->setVisible(fShow);
    ui->labelMnList->setVisible(fShow);
    if (!fShow) {
        ui->labelStatus->setVisible(true);
        ui->labelMnList->setVisible(true);
        ui->labelStatus->setText(tr("<font color='darkgreen'>Ready</font>") );
        ui->labelAvgBlockTime->setText(getAvgBlockTime());
        if (masternodeSync.IsSynced()) {
            ui->labelMnList->setText(tr("<font color='darkgreen'>Ready</font>") );
        } else if (masternodeSync.IsFailed()) {
            ui->labelMnList->setText(tr("<font color='darkred'>Failed</font>") );
        }
    }
    ui->labelStakeStatus->setText(getStakeStatus());
}

void OverviewPage::SetupTransactionList() {
    int nNumItems = NUM_ITEMS;
    ui->listTransactions->setMinimumHeight(nNumItems * (DECORATION_SIZE + 2));
    if(walletModel && walletModel->getOptionsModel()) {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(walletModel->getTransactionTableModel());
        filter->setLimit(nNumItems);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);
        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);
    }
}
