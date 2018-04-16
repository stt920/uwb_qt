// -------------------------------------------------------------------------------------------------------------------
//
//  File: GraphicsWidget.cpp
//
//  Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author:
//
// -------------------------------------------------------------------------------------------------------------------

#include "GraphicsWidget.h"
#include "ui_GraphicsWidget.h"

#include "RTLSDisplayApplication.h"
#include "ViewSettings.h"

#include <QDomDocument>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsRectItem>
#include <QDebug>
#include <QInputDialog>
#include <QFile>
#include <QPen>

#include <QSqlTableModel>

#include "storage.h"

#include<QDesktopWidget>
#include "storage_modify.h"
#include "storage_delete.h"


#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QAxObject>
#include <QSqlQuery>
#include <QSqlRecord>
#include "wms/connect_database.h"
#include <QDateTime>
#include <QTableWidgetItem>

#define PEN_WIDTH (0.04)
#define ANC_SIZE (0.15)
#define FONT_SIZE (10)

GraphicsWidget::GraphicsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GraphicsWidget)
{
    ui->setupUi(this);

    model=new QSqlTableModel(this);
    model->setTable("input");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select(); //选取整个表的所有行
    ui->tableView->setModel(model);

    model1=new QSqlTableModel(this);
    model1->setTable("reader");
    model1->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model1->select(); //选取整个表的所有行
    ui->tableView_2->setModel(model1);

    model2=new QSqlTableModel(this);
    model2->setTable("reader");
    model2->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model2->select(); //选取整个表的所有行
    ui->tableView_3->setModel(model2);

    model3=new QSqlTableModel(this);
    model3->setTable("outstorage");
    model3->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model3->select(); //选取整个表的所有行
    ui->tableView_5->setModel(model3);


    model5=new QSqlTableModel(this);
    model5->setTable("biaoshi");
    model5->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model5->select(); //选取整个表的所有行
    ui->tableView_7->setModel(model5);


    this->_scene = new QGraphicsScene(this);

    ui->graphicsView->setScene(this->_scene);
    ui->graphicsView->scale(1, -1);
    ui->graphicsView->setBaseSize(this->height()*0.75, this->width());

    //tagTable
    //Tag ID, x, y, z, r95, vbat? ...
    QStringList tableHeader;
    tableHeader << "Tag ID/Label" << "X\n(m)" << "Y\n(m)" << "Z\n(m)" << "R95 \n(m)"
                << "Anc 0\n range (m)" << "Anc 1\n range (m)" << "Anc 2\n range (m)" << "Anc 3\n range (m)"
                   ;
    ui->tagTable->setHorizontalHeaderLabels(tableHeader);

    ui->tagTable->setColumnWidth(ColumnID,100);    //ID
    ui->tagTable->setColumnWidth(ColumnX,55); //x
    ui->tagTable->setColumnWidth(ColumnY,55); //y
    ui->tagTable->setColumnWidth(ColumnZ,55); //z
    //ui->tagTable->setColumnWidth(ColumnBlinkRx,70); //% Blinks RX
    //ui->tagTable->setColumnWidth(ColumnLocations,70); //% Multilaterations Success
    ui->tagTable->setColumnWidth(ColumnR95,70); //R95

    ui->tagTable->setColumnWidth(ColumnRA0,70);
    ui->tagTable->setColumnWidth(ColumnRA1,70);
    ui->tagTable->setColumnWidth(ColumnRA2,70);
    ui->tagTable->setColumnWidth(ColumnRA3,70);

    ui->tagTable->setColumnHidden(ColumnIDr, true); //ID raw hex
    //ui->tagTable->setColumnWidth(ColumnIDr,70); //ID raw hex

    //anchorTable
    //Anchor ID, x, y, z,
    QStringList anchorHeader;
    anchorHeader << "Anchor ID" << "X\n(m)" << "Y\n(m)" << "Z\n(m)"
                    << "T0\n(cm)" << "T1\n(cm)" << "T2\n(cm)" << "T3\n(cm)"
                    << "T4\n(cm)" << "T5\n(cm)" << "T6\n(cm)" << "T7\n(cm)" ;
    ui->anchorTable->setHorizontalHeaderLabels(anchorHeader);

    ui->anchorTable->setColumnWidth(ColumnID,100);    //ID
    ui->anchorTable->setColumnWidth(ColumnX,55); //x
    ui->anchorTable->setColumnWidth(ColumnY,55); //y
    ui->anchorTable->setColumnWidth(ColumnZ,55); //z
    ui->anchorTable->setColumnWidth(4,55); //t0
    ui->anchorTable->setColumnWidth(5,55); //t1
    ui->anchorTable->setColumnWidth(6,55); //t2
    ui->anchorTable->setColumnWidth(7,55); //t3
    ui->anchorTable->setColumnWidth(8,55); //t4
    ui->anchorTable->setColumnWidth(9,55); //t5
    ui->anchorTable->setColumnWidth(10,55); //t6
    ui->anchorTable->setColumnWidth(11,55); //t7

    //hide Anchor/Tag range corection table
    hideTACorrectionTable(true);

    //set defaults
    _tagSize = 0.15;
    _historyLength = 20;
    _showHistoryP = _showHistory = true;

    _busy = true ;
    _ignore = true;

    _selectedTagIdx = -1;

    zone1 = NULL;
    zone2 = NULL;

    _maxRad = 1000;

    _minRad = 0;

    _zone1Rad = 0;
    _zone2Rad = 0;
    _zone1Red = false;
    _zone2Red = false;

    _geoFencingMode = false;
    _alarmOut = false;

    _line01 = NULL;
    _line02 = NULL;
    _line12 = NULL;
    RTLSDisplayApplication::connectReady(this, "onReady()");
    //stt
    ui->tagTable->hide();
    ui->anchorTable->hide();
    ui->dockWidget->hide();

    this->empytOrdermap();

}

void GraphicsWidget::onReady()
{
	QObject::connect(ui->tagTable, SIGNAL(cellChanged(int, int)), this, SLOT(tagTableChanged(int, int)));
    QObject::connect(ui->anchorTable, SIGNAL(cellChanged(int, int)), this, SLOT(anchorTableChanged(int, int)));
    QObject::connect(ui->tagTable, SIGNAL(cellClicked(int, int)), this, SLOT(tagTableClicked(int, int)));
    QObject::connect(ui->anchorTable, SIGNAL(cellClicked(int, int)), this, SLOT(anchorTableClicked(int, int)));
    QObject::connect(ui->tagTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    QObject::connect(ui->anchorTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChangedAnc()));

    QObject::connect(this, SIGNAL(centerAt(double,double)), graphicsView(), SLOT(centerAt(double, double)));
    QObject::connect(this, SIGNAL(centerRect(QRectF)), graphicsView(), SLOT(centerRect(QRectF)));

    _busy = false ;
}

GraphicsWidget::~GraphicsWidget()
{
    delete _scene;
    delete ui;
}

GraphicsView *GraphicsWidget::graphicsView()
{
    return ui->graphicsView;
}


void GraphicsWidget::loadConfigFile(QString filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg(filename).arg(file.errorString())));
        return;
    }

    QDomDocument doc;
    QString error;
    int errorLine;
    int errorColumn;

    if(doc.setContent(&file, false, &error, &errorLine, &errorColumn))
    {
        qDebug() << "file error !!!" << error << errorLine << errorColumn;

        emit setTagHistory(_historyLength);
    }

    QDomElement config = doc.documentElement();

    if( config.tagName() == "config" )
    {
        QDomNode n = config.firstChild();
        while( !n.isNull() )
        {
            QDomElement e = n.toElement();
            if( !e.isNull() )
            {
                if( e.tagName() == "tag_cfg" )
                {
                    _tagSize = (e.attribute( "size", "" )).toDouble();
                    _historyLength = (e.attribute( "history", "" )).toInt();
                }
                else
                if( e.tagName() == "tag" )
                {
                    bool ok;
                    quint64 id = (e.attribute( "ID", "" )).toULongLong(&ok, 16);
                    QString label = (e.attribute( "label", "" ));

                    _tagLabels.insert(id, label);
                }
            }

            n = n.nextSibling();
        }

    }

    file.close();

    emit setTagHistory(_historyLength);
}


void GraphicsWidget::hideTACorrectionTable(bool hidden)
{
    ui->anchorTable->setColumnHidden(4,hidden); //t0
    ui->anchorTable->setColumnHidden(5,hidden); //t1
    ui->anchorTable->setColumnHidden(6,hidden); //t2
    ui->anchorTable->setColumnHidden(7,hidden); //t3
    ui->anchorTable->setColumnHidden(8,hidden); //t4
    ui->anchorTable->setColumnHidden(9,hidden); //t5
    ui->anchorTable->setColumnHidden(10,hidden); //t6
    ui->anchorTable->setColumnHidden(11,hidden); //t7
}


QDomElement TagToNode( QDomDocument &d, quint64 id, QString label )
{
    QDomElement cn = d.createElement( "tag" );
    cn.setAttribute("ID", QString::number(id, 16));
    cn.setAttribute("label", label);
    return cn;
}

void GraphicsWidget::saveConfigFile(QString filename)
{
    QFile file( filename );

    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        qDebug(qPrintable(QString("Error: Cannot read file %1 %2").arg("./TREKtag_config.xml").arg(file.errorString())));
        return;
    }

    QDomDocument doc;

    // Adding tag config root
    QDomElement config = doc.createElement("config");
    doc.appendChild(config);

    QDomElement cn = doc.createElement( "tag_cfg" );
    cn.setAttribute("size", QString::number(_tagSize));
    cn.setAttribute("history", QString::number(_historyLength));
    config.appendChild(cn);

    //update the map
    QMap<quint64, QString>::iterator i = _tagLabels.begin();

    while (i != _tagLabels.end())
    {
        config.appendChild( TagToNode(doc, i.key(), i.value()) );

        i++;
    }

    QTextStream ts( &file );
    ts << doc.toString();

    file.close();

    qDebug() << doc.toString();
}

void GraphicsWidget::clearTags(void)
{
    qDebug() << "table rows " << ui->tagTable->rowCount() << " list " << this->_tags.size();

    while (ui->tagTable->rowCount())
    {
        QTableWidgetItem* item = ui->tagTable->item(0, ColumnIDr);

        if(item)
        {
            qDebug() << "Item text: " << item->text();

            bool ok;
            quint64 tagID = item->text().toULongLong(&ok, 16);
            //clear scene from any tags
            Tag *tag = this->_tags.value(tagID, NULL);
            if(tag->r95p) //remove R95
            {
                //re-size the elipse... with a new rad value...
                tag->r95p->setOpacity(0); //hide it

                this->_scene->removeItem(tag->r95p);
                delete(tag->r95p);
                tag->r95p = NULL;
            }
            if(tag->avgp) //remove average
            {
                //re-size the elipse... with a new rad value...
                tag->avgp->setOpacity(0); //hide it

                this->_scene->removeItem(tag->avgp);
                delete(tag->avgp);
                tag->avgp = NULL;
            }
            if(tag->geop) //remove average
            {
                //re-size the elipse... with a new rad value...
                tag->geop->setOpacity(0); //hide it

                this->_scene->removeItem(tag->geop);
                delete(tag->geop);
                tag->geop = NULL;
            }
            if(tag->tagLabel) //remove label
            {
                //re-size the elipse... with a new rad value...
                tag->tagLabel->setOpacity(0); //hide it

                this->_scene->removeItem(tag->tagLabel);
                delete(tag->tagLabel);
                tag->tagLabel = NULL;
            }

            //remove history...
            for(int idx=0; idx<_historyLength; idx++ )
            {
                QAbstractGraphicsShapeItem *tag_p = tag->p[idx];
                if(tag_p)
                {
                    tag_p->setOpacity(0); //hide it

                    this->_scene->removeItem(tag_p);
                    delete(tag_p);
                    tag_p = NULL;
                    tag->p[idx] = 0;

                    qDebug() << "hist remove tag " << idx;
                }
            }
            {
                QMap<quint64, Tag*>::iterator i = _tags.find(tagID);

                if(i != _tags.end()) _tags.erase(i);
            }
        }
        ui->tagTable->removeRow(0);
    }

    //clear tag table
    ui->tagTable->clearContents();

    qDebug() << "clear tags/tag table";

}

void GraphicsWidget::itemSelectionChanged(void)
{
    QList <QTableWidgetItem *>  l = ui->tagTable->selectedItems();
}

void GraphicsWidget::itemSelectionChangedAnc(void)
{
    QList <QTableWidgetItem *>  l = ui->anchorTable->selectedItems();
}

void GraphicsWidget::tagTableChanged(int r, int c)
{
    if(!_ignore)
    {
        Tag *tag = NULL;
        bool ok;
        quint64 tagId = (ui->tagTable->item(r,ColumnIDr)->text()).toULongLong(&ok, 16);
        tag = _tags.value(tagId, NULL);

        if(!tag) return;

        if(c == ColumnID) //label has changed
        {
            QString newLabel = ui->tagTable->item(r,ColumnID)->text();

            tag->tagLabelStr = newLabel;
            tag->tagLabel->setText(newLabel);

            //update the map
            QMap<quint64, QString>::iterator i = _tagLabels.find(tagId);

            if(i == _tagLabels.end()) //did not find the label
            {
                //insert the new value
                _tagLabels.insert(tagId, newLabel);
            }
            else //if (i != taglabels.end()) // && i.key() == tagId)
            {
                _tagLabels.erase(i); //erase the key
                _tagLabels.insert(tagId, newLabel);
            }
        }
    }
}

void GraphicsWidget::anchorTableChanged(int r, int c)
{
    if(!_ignore)
    {
        _ignore = true;

        Anchor *anc = NULL;
        bool ok;

        anc = _anchors.value(r, NULL);

        if(!anc) return;

        switch(c)
        {
            case ColumnX:
            case ColumnY:
            case ColumnZ:
            {
                double xyz = (ui->anchorTable->item(r,c)->text()).toDouble(&ok);
                if(ok)
                {
                    if((ColumnZ == c) && (r > 2))
                    {
                        emit updateAnchorXYZ(r, c, xyz);
                    }
                    else
                    {
                        emit updateAnchorXYZ(r, c, xyz);
                    }

                    double xn = (ui->anchorTable->item(r,ColumnX)->text()).toDouble(&ok);
                    double yn = (ui->anchorTable->item(r,ColumnY)->text()).toDouble(&ok);
                    double zn = (ui->anchorTable->item(r,ColumnZ)->text()).toDouble(&ok);

                    ui->anchorTable->item(r,ColumnX)->setText(QString::number(xn, 'f', 2));
                    ui->anchorTable->item(r,ColumnY)->setText(QString::number(yn, 'f', 2));

                    anchPos(r, xn, yn, zn, true, false);

                    if((ColumnZ == c) && (r <= 2))
                    {
                        //update z of 1, 2, and 3 to be the same
                        emit updateAnchorXYZ(0, c, xyz);
                        emit updateAnchorXYZ(1, c, xyz);
                        emit updateAnchorXYZ(2, c, xyz);

                        //make all 3 anchors have the same z coordinate
                        ui->anchorTable->item(0,ColumnZ)->setText(QString::number(zn, 'f', 2));
                        ui->anchorTable->item(1,ColumnZ)->setText(QString::number(zn, 'f', 2));
                        ui->anchorTable->item(2,ColumnZ)->setText(QString::number(zn, 'f', 2));
                    }
                }
            }
            break;
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            {
                int value = (ui->anchorTable->item(r,c)->text()).toInt(&ok);
                if(ok)
                    emit updateTagCorrection(r, c-4, value);
            }
            break;
        }

        _ignore = false;
    }
}

void GraphicsWidget::anchorTableClicked(int r, int c)
{
    Anchor *anc = NULL;

    anc = _anchors.value(r, NULL);

    if(!anc) return;

    if(c == ColumnID) //toggle label
    {
        QTableWidgetItem *pItem = ui->anchorTable->item(r, c);
        anc->show = (pItem->checkState() == Qt::Checked) ? true : false;

        anc->a->setOpacity(anc->show ? 1.0 : 0.0);
        anc->ancLabel->setOpacity(anc->show ? 1.0 : 0.0);
    }
}

void GraphicsWidget::tagTableClicked(int r, int c)
{
    Tag *tag = NULL;
    bool ok;
    quint64 tagId = (ui->tagTable->item(r,ColumnIDr)->text()).toULongLong(&ok, 16);
    tag = _tags.value(tagId, NULL);

    _selectedTagIdx = r;

    if(!tag) return;

    if(c == ColumnR95) //toggle R95 display
    {
        QTableWidgetItem *pItem = ui->tagTable->item(r, c);
        tag->r95Show = (pItem->checkState() == Qt::Checked) ? true : false;
    }

    if(c == ColumnID) //toggle label
    {
        QTableWidgetItem *pItem = ui->tagTable->item(r, c);
        tag->showLabel = (pItem->checkState() == Qt::Checked) ? true : false;

        tag->tagLabel->setOpacity(tag->showLabel ? 1.0 : 0.0);
    }

}

/**
 * @fn    tagIDtoString
 * @brief  convert hex Tag ID to string (preappend 0x)
 *
 * */
void GraphicsWidget::tagIDToString(quint64 tagId, QString *t)
{
    //NOTE: this needs to be hex string as it is written into ColumnIDr
    //and is used later to remove the correct Tag
    *t = "0x"+QString::number(tagId, 16);
}

int GraphicsWidget::findTagRowIndex(QString &t)
{
    for (int ridx = 0 ; ridx < ui->tagTable->rowCount() ; ridx++ )
    {
        QTableWidgetItem* item = ui->tagTable->item(ridx, ColumnIDr);

        if(item)
        {
            if(item->text() == t)
            {
                return ridx;
            }
        }
    }

    return -1;
}

/**
 * @fn    insertTag
 * @brief  insert Tag into the tagTable at row ridx
 *
 * */
void GraphicsWidget::insertTag(int ridx, QString &t, bool showR95, bool showLabel, QString &l)
{
    _ignore = true;

    ui->tagTable->insertRow(ridx);

    qDebug() << "Insert Tag" << ridx << t << l;

    for( int col = ColumnID ; col < ui->tagTable->columnCount(); col++)
    {
        QTableWidgetItem* item = new QTableWidgetItem();
        QTableWidgetItem *pItem = new QTableWidgetItem();
        if(col == ColumnID )
        {
            if(showLabel)
            {
                pItem->setCheckState(Qt::Checked);
                pItem->setText(l);
            }
            else
            {
                pItem->setCheckState(Qt::Unchecked);
                pItem->setText(l);
            }
            item->setFlags((item->flags() ^ Qt::ItemIsEditable) | Qt::ItemIsSelectable);
            //ui->tagTable->setItem(ridx, col, item);
            ui->tagTable->setItem(ridx, col, pItem);
        }
        else
        {
            if(col == ColumnIDr)
            {
                item->setText(t);
            }

            item->setFlags((item->flags() ^ Qt::ItemIsEditable) | Qt::ItemIsSelectable);
            ui->tagTable->setItem(ridx, col, item);
        }

        if(col == ColumnR95)
        {
            if(showR95)
            {
                pItem->setCheckState(Qt::Checked);
            }
            else
            {
                pItem->setCheckState(Qt::Unchecked);
            }
            pItem->setText(" ");
            ui->tagTable->setItem(ridx,col,pItem);
        }
   }

   _ignore = false; //we've added a row
}

/**
 * @fn    insertAnchor
 * @brief  insert Anchor/Tag correction values into the anchorTable at row ridx
 *
 * */
void GraphicsWidget::insertAnchor(int ridx, double x, double y, double z,int *array, bool show)
{
    int i = 0;
    _ignore = true;

    //add tag offsets
    for( int col = 4 ; col < ui->anchorTable->columnCount(); col++)
    {
        QTableWidgetItem* item = new QTableWidgetItem();
        item->setText(QString::number(array[i]));
        item->setTextAlignment(Qt::AlignHCenter);
        //item->setFlags((item->flags() ^ Qt::ItemIsEditable) | Qt::ItemIsSelectable);
        ui->anchorTable->setItem(ridx, col, item);
    }

    //add x, y, z
    QTableWidgetItem* itemx = new QTableWidgetItem();
    QTableWidgetItem* itemy = new QTableWidgetItem();
    QTableWidgetItem* itemz = new QTableWidgetItem();

    itemx->setText(QString::number(x, 'f', 2));
    itemy->setText(QString::number(y, 'f', 2));
    itemz->setText(QString::number(z, 'f', 2));

    ui->anchorTable->setItem(ridx, ColumnX, itemx);
    ui->anchorTable->setItem(ridx, ColumnY, itemy);
    ui->anchorTable->setItem(ridx, ColumnZ, itemz);

    {
        QTableWidgetItem *pItem = new QTableWidgetItem();

        if(show)
        {
            pItem->setCheckState(Qt::Checked);
        }
        else
        {
            pItem->setCheckState(Qt::Unchecked);
        }
        pItem->setText(QString::number(ridx));
        pItem->setTextAlignment(Qt::AlignHCenter);

        pItem->setFlags((pItem->flags() ^ Qt::ItemIsEditable) | Qt::ItemIsSelectable);

        ui->anchorTable->setItem(ridx, ColumnID, pItem);
    }

    ui->anchorTable->showRow(ridx);
    _ignore = false;
}

/**
 * @fn    addNewTag
 * @brief  add new Tag with tagId into the tags QMap
 *
 * */
void GraphicsWidget::addNewTag(quint64 tagId)
{
    static double c_h = 0.1;
    Tag *tag;
    int tid = tagId & 0xf;
    QString taglabel = QString("Tag %1").arg(tid); //taglabels.value(tagId, NULL);

    qDebug() << "Add new Tag: 0x" + QString::number(tagId, 16) << tagId;

    //insert into QMap list, and create an array to hold history of its positions
    _tags.insert(tagId,new(Tag));

    tag = this->_tags.value(tagId, NULL);
    tag->p.resize(_historyLength);

    c_h += 0.568034;
    if (c_h >= 1)
        c_h -= 1;
    tag->colourH = c_h;
    tag->colourS = 0.55;
    tag->colourV = 0.98;

    tag->tsPrev = 0;
    tag->r95Show = false;
    tag->showLabel = (taglabel != NULL) ? true : false;
    tag->tagLabelStr = taglabel;
    //add ellipse for the R95 - set to transparent until we get proper r95 data
    tag->r95p = this->_scene->addEllipse(-0.1, -0.1, 0, 0);
    tag->r95p->setOpacity(0);
    tag->r95p->setPen(Qt::NoPen);
    tag->r95p->setBrush(Qt::NoBrush);
    //add ellipse for average point - set to transparent until we get proper average data
    tag->avgp = this->_scene->addEllipse(0, 0, 0, 0);
    tag->avgp->setBrush(Qt::NoBrush);
    tag->avgp->setPen(Qt::NoPen);
    tag->avgp->setOpacity(0);
    //add ellipse for geo fencing point - set to transparent until we get proper average data
    tag->geop = this->_scene->addEllipse(0, 0, 0, 0);
    tag->geop->setBrush(Qt::NoBrush);
    tag->geop->setPen(Qt::NoPen);
    tag->geop->setOpacity(0);
    //add text label and hide until we see if user has enabled showLabel
    {
        QPen pen = QPen(Qt::blue);
        tag->tagLabel = new QGraphicsSimpleTextItem(NULL);
        tag->tagLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        tag->tagLabel->setZValue(1);
        tag->tagLabel->setText(taglabel);
        tag->tagLabel->setOpacity(0);
        QFont font = tag->tagLabel->font();
        font.setPointSize(FONT_SIZE);
        font.setWeight(QFont::Normal);
        tag->tagLabel->setFont(font);
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(PEN_WIDTH);
        tag->tagLabel->setPen(pen);
        this->_scene->addItem(tag->tagLabel);
    }
}

/**
 * @fn    tagPos
 * @brief  update tag position on the screen (add to scene if it does not exist)
 *
 * */
void GraphicsWidget::tagPos(quint64 tagId, double x, double y, double z)
{
    //qDebug() << "tagPos Tag: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z;

    if(_busy || _geoFencingMode) //don't display position if geofencing is on
    {
        qDebug() << "(Widget - busy IGNORE) Tag: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z;
    }
    else
    {
        Tag *tag = NULL;
        bool newTag = false;
        QString t ;

        _busy = true ;
        tagIDToString(tagId, &t); //convert uint64 to string

        tag = _tags.value(tagId, NULL);

        if(!tag) //add new tag to the tags array
        {
            //tag does not exist, so create a new one
            newTag = true;
            addNewTag(tagId);
            tag = this->_tags.value(tagId, NULL);
        }

        if(!tag->p[tag->idx]) //we have not added this object yet to the history array
        {
            QAbstractGraphicsShapeItem *tag_pt = this->_scene->addEllipse(-1*_tagSize/2, -1*_tagSize/2, _tagSize, _tagSize);
            tag->p[tag->idx] = tag_pt;
            tag_pt->setPen(Qt::NoPen);

            if(tag->idx > 0) //use same brush settings for existing tag ID
            {
                tag_pt->setBrush(tag->p[0]->brush());
            }
            else //new brush... new tag ID as idx = 0
            {
                QBrush b = QBrush(QColor::fromHsvF(tag->colourH, tag->colourS, tag->colourV));
                QPen pen = QPen(b.color().darker());
                pen.setStyle(Qt::SolidLine);
                pen.setWidthF(PEN_WIDTH);
                //set the brush colour (average point is darker of the same colour
                tag_pt->setBrush(b.color().dark());
                tag->avgp->setBrush(b.color().darker());
                tag->tagLabel->setBrush(b.color().dark());
                tag->tagLabel->setPen(pen);
            }

            tag_pt->setToolTip(t);

            //update colour for next tag
            if(newTag) //keep same colour for same tag ID
            {
                tag->ridx = findTagRowIndex(t);

                if(tag->ridx == -1)
                {
                    tag->ridx = ui->tagTable->rowCount();

                    insertTag(tag->ridx, t, tag->r95Show, tag->showLabel, tag->tagLabelStr);
                }
            }
        }

        _ignore = true;
        ui->tagTable->item(tag->ridx,ColumnX)->setText(QString::number(x, 'f', 3));
        ui->tagTable->item(tag->ridx,ColumnY)->setText(QString::number(y, 'f', 3));
        ui->tagTable->item(tag->ridx,ColumnZ)->setText(QString::number(z, 'f', 3));

        tag->p[tag->idx]->setPos(x, y);

        if(_showHistory)
        {
            tagHistory(tagId);
            tag->idx = (tag->idx+1)%_historyLength;
        }
        else
        {
            //index will stay at 0
            tag->p[tag->idx]->setOpacity(1);
        }

        tag->tagLabel->setPos(x + 0.15, y + 0.15);
        _ignore = false;
        _busy = false ;

        //qDebug() << "Tag: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z;
	}
}


void GraphicsWidget::tagStats(quint64 tagId, double x, double y, double z, double r95)
{
    if(_busy)
    {
        qDebug() << "(busy IGNORE) R95: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z << " " << r95;
    }
    else
    {
        Tag *tag = NULL;

        _busy = true ;

        tag = _tags.value(tagId, NULL);

        if(!tag) //add new tag to the tags array
        {
            addNewTag(tagId);
            tag = this->_tags.value(tagId, NULL);
        }

        if (tag)
        {
            //static float h = 0.1;
            //float s = 0.5, v = 0.98;
            double rad = r95*2;

            if(tag->r95p)
            {
                //re-size the elipse... with a new rad value...
                tag->r95p->setOpacity(0); //hide it

                this->_scene->removeItem(tag->r95p);
                delete(tag->r95p);
                tag->r95p = NULL;
            }

            //else //add r95 circle

            {
                //add R95 circle
                tag->r95p = this->_scene->addEllipse(-1*r95, -1*r95, rad, rad);
                tag->r95p->setPen(Qt::NoPen);
                tag->r95p->setBrush(Qt::NoBrush);

                if( tag->r95Show && (rad <= 1))
                {
                    QPen pen = QPen(Qt::darkGreen);
                    pen.setStyle(Qt::DashDotDotLine);
                    pen.setWidthF(PEN_WIDTH);

                    tag->r95p->setOpacity(0.5);
                    tag->r95p->setPen(pen);
                    //tag->r95p->setBrush(QBrush(Qt::green, Qt::Dense7Pattern));
                    tag->r95p->setBrush(Qt::NoBrush);
                }
                else if (tag->r95Show && ((rad > 1)/*&&(rad <= 2)*/))
                {
                    QPen pen = QPen(Qt::darkRed);
                    pen.setStyle(Qt::DashDotDotLine);
                    pen.setWidthF(PEN_WIDTH);

                    tag->r95p->setOpacity(0.5);
                    tag->r95p->setPen(pen);
                    //tag->r95p->setBrush(QBrush(Qt::darkRed, Qt::Dense7Pattern));
                    tag->r95p->setBrush(Qt::NoBrush);
                }

            }

            //update Tag R95 value in the table
            {
                QString t ;
                int ridx = 0;

                tagIDToString(tagId, &t);

                ridx = findTagRowIndex(t);

                if(ridx != -1)
                {
                    ui->tagTable->item(ridx,ColumnR95)->setText(QString::number(r95, 'f', 3));
                }
                else
                {

                }
            }


            if(!tag->avgp) //add the average point
            {
                QBrush b = tag->p[0]->brush().color().darker();

                tag->avgp = this->_scene->addEllipse(-0.025, -0.025, 0.05, 0.05);
                tag->avgp->setBrush(b);
                tag->avgp->setPen(Qt::NoPen);
            }

            //if  (rad > 2)
            if(!tag->r95Show)
            {
                 tag->avgp->setOpacity(0);
            }
            else
            {
                tag->avgp->setOpacity(1);
                tag->avgp->setPos(x, y); //move it to the avg x and y values
            }

            tag->r95p->setPos(x, y); //move r95 center to the avg x and y values
        }
        else
        {
            //ERROR - there has to be a tag already...
            //ignore this statistics report
        }

        _busy = false ;

        qDebug() << "R95: 0x" + QString::number(tagId, 16) << " " << x << " " << y << " " << z << " " << r95;

    }
}


void GraphicsWidget::tagRange(quint64 tagId, quint64 aId, double range)
{
    if(_busy)
    {
        qDebug() << "(busy IGNORE) Range 0x" + QString::number(tagId, 16) << " " << range << " " << QString::number(aId, 16) ;
    }
    else
    {
        Tag *tag = NULL;

        _busy = true ;

        tag = _tags.value(tagId, NULL);

        if(!tag) //add new tag to the tags array
        {
            addNewTag(tagId);
            tag = this->_tags.value(tagId, NULL);
        }

        if (tag)
        {
            //Geo-fencing mode
            if(_geoFencingMode && (aId == 0))
            {
                QPen pen = QPen(Qt::darkGreen);

                if(tag->geop)
                {
                    //re-size the elipse... with a new rad value...
                    tag->geop->setOpacity(0); //hide it

                    this->_scene->removeItem(tag->geop);
                    delete(tag->geop);
                    tag->geop = NULL;
                }

                //if(!tag->geop) //add the circle around the anchor 0 which represents the tag
                {
                    pen.setColor(Qt::darkGreen);
                    pen.setStyle(Qt::DashDotDotLine);
                    pen.setWidthF(PEN_WIDTH);

                    if(_alarmOut) //then if > than outside zone
                    {
                        if(range >= _maxRad) //if > zone out then colour of zone out
                        {
                            pen.setColor(Qt::red);
                        }
                        else
                        {
                            if(range > _minRad) //if < zone out and > zone in
                            {
                                //amber
                                //pen.setColor(QColor::fromRgbF(1.0, 0.5, 0.0));
                                //blue
                                pen.setColor(QColor::fromRgbF(0.0, 0.0, 1.0));
                            }
                        }
                    }
                    else //then if < inside zone
                    {
                        if(range <= _minRad)
                        {
                            pen.setColor(Qt::red);
                        }
                        else
                        {
                            if(range < _maxRad) //if < zone out and > zone in
                            {
                                //amber
                                //pen.setColor(QColor::fromRgbF(1.0, 0.5, 0.0));
                                //blue
                                pen.setColor(QColor::fromRgbF(0.0, 0.0, 1.0));
                            }
                        }
                    }

                    tag->geop = this->_scene->addEllipse(-range, -range, range*2, range*2);
                    tag->geop->setPen(pen);
                    tag->geop->setBrush(Qt::NoBrush);
                }

                tag->geop->setOpacity(1);
                tag->geop->setPos(0, 0);

                tag->tagLabel->setPos(range + ((tagId & 0xF)*0.05 + 0.05), ((tagId & 0xF)*0.1 + 0.05));
            }

            //update Tag/Anchor range value in the table
            {
                QString t ;
                int ridx = 0;
                int anc = aId&0x3;

                tagIDToString(tagId, &t);

                ridx = findTagRowIndex(t);

                if(ridx != -1)
                {
                    ui->tagTable->item(ridx,ColumnRA0+anc)->setText(QString::number(range, 'f', 3));
                }
                else //add tag into tag table
                {
                    tag->ridx = ui->tagTable->rowCount();

                    insertTag(tag->ridx, t, false, false, tag->tagLabelStr);
                }
            }
        }
        else
        {
            //if in tracking/navigation mode - ignore the range report
        }

    }
    _busy = false ;

}


void GraphicsWidget::tagHistory(quint64 tagId)
{
    int i = 0;
    int j = 0;

    Tag *tag = this->_tags.value(tagId, NULL);
    for(i = 0; i < _historyLength; i++)
    {
        QAbstractGraphicsShapeItem *tag_p = tag->p[i];

        if(!tag_p)
        {
            break;
        }
        else
        {
            j = (tag->idx - i); //element at index is opaque
            if(j<0) j+= _historyLength;
            tag_p->setOpacity(1-(float)j/_historyLength);
        }
    }
}

/**
 * @fn    tagHistoryNumber
 * @brief  set tag history length
 *
 * */
void GraphicsWidget::tagHistoryNumber(int value)
{
    bool tag_showHistory = _showHistory;

    while(_busy);

    _busy = true;

    //remove old history
    setShowTagHistory(false);

    //need to resize all the tag point arrays
    for (int i = 0; i < ui->tagTable->rowCount(); i++)
    {
        QTableWidgetItem* item = ui->tagTable->item(i, ColumnIDr);

        if(item)
        {
            bool ok;
            quint64 tagID = item->text().toULongLong(&ok, 16);
            //clear scene from any tags
            Tag *tag = _tags.value(tagID, NULL);
            tag->p.resize(value);
        }
    }

    //set new history length
    _historyLength = value;

    //set the history to show/hide
    _showHistory = tag_showHistory;

    _busy = false;
}


/**
 * @fn    zone1Value
 * @brief  set Zone 1 radius
 *
 * */
void GraphicsWidget::zone1Value(double value)
{
    if(zone1)
    {
        this->_scene->removeItem(zone1);
        delete(zone1);
        zone1 = NULL;
    }

    zone(0, value, _zone1Red);
}

/**
 * @fn    zone2Value
 * @brief  set Zone 2 radius
 *
 * */
void GraphicsWidget::zone2Value(double value)
{
    if(zone2)
    {
        this->_scene->removeItem(zone2);
        delete(zone2);
        zone2 = NULL;
    }

    zone(1, value, _zone2Red);
}


/**
 * @fn    setAlarm
 * @brief  set alarms
 *
 * */
void GraphicsWidget::setAlarm(bool in, bool out)
{
    Q_UNUSED(in);
    if(zone1)
    {
        zone1->setOpacity(0);
        this->_scene->removeItem(zone1);
        delete(zone1);
        zone1 = NULL;
    }

    if(zone2)
    {
        zone2->setOpacity(0);
        this->_scene->removeItem(zone2);
        delete(zone2);
        zone2 = NULL;
    }

    if(out)
    {
        _alarmOut = true;

        //set outside zone to red
        if(_zone1Rad > _zone2Rad)
        {
            zone(0, _zone1Rad, true);

            zone(1, _zone2Rad, false);
        }
        else
        {
            zone(0, _zone1Rad, false);

            zone(1, _zone2Rad, true);
        }
    }
    else
    {
        _alarmOut = false;

        //set inside zone to red
        if(_zone1Rad < _zone2Rad)
        {
            zone(0, _zone1Rad, true);

            zone(1, _zone2Rad, false);
        }
        else
        {
            zone(0, _zone1Rad, false);

            zone(1, _zone2Rad, true);
        }
    }
}

/**
 * @fn    zones
 * @brief  set Zone radius
 *
 * */
void GraphicsWidget::zone(int zone, double radius, bool red)
{
    //add Zone 1
    if(zone == 0)
    {
        QPen pen = QPen(QBrush(Qt::green), 0.005) ;

        _zone1Red = false;

        if(red)
        {
            pen.setColor(Qt::red);
            _zone1Red = true;
        }
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(0.03);

        if(!zone1)
        {
            zone1 = this->_scene->addEllipse(-_zone1Rad, -_zone1Rad, 2*_zone1Rad, 2*_zone1Rad);
        }
        zone1->setPen(pen);
        zone1->setBrush(Qt::NoBrush);
        zone1->setToolTip("Zone1");
        zone1->setPos(0, 0);
        _zone1Rad = radius;

        if(_geoFencingMode)
        {
            zone1->setOpacity(1);
        }
        else
        {
            zone1->setOpacity(0);
        }
    }
    else
    //add Zone 2
    {
        QPen pen = QPen(QBrush(Qt::green), 0.005) ;

        _zone2Red = false;

        if(red)
        {
            pen.setColor(Qt::red);
            _zone2Red = true;
        }
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(0.03);

        if(!zone2)
        {
            zone2 = this->_scene->addEllipse(-_zone2Rad, -_zone2Rad, 2*_zone2Rad, 2*_zone2Rad);
        }
        zone2->setPen(pen);
        zone2->setBrush(Qt::NoBrush);
        zone2->setToolTip("Zone2");
        zone2->setPos(0, 0);
        _zone2Rad = radius;

        if(_geoFencingMode)
        {
            zone2->setOpacity(1);
        }
        else
        {
            zone2->setOpacity(0);
        }
    }

    _maxRad = (_zone1Rad > _zone2Rad) ? _zone1Rad : _zone2Rad;
    _minRad = (_zone1Rad > _zone2Rad) ? _zone2Rad : _zone1Rad;
}

void GraphicsWidget::centerOnAnchors(void)
{

    Anchor *a1 = this->_anchors.value(0, NULL);
    Anchor *a2 = this->_anchors.value(1, NULL);
    Anchor *a3 = this->_anchors.value(2, NULL);
    //Anchor *a4 = anc = this->anchors.value(0, NULL);

    QPolygonF p1 = QPolygonF() << QPointF(a1->a->pos()) << QPointF(a2->a->pos()) << QPointF(a3->a->pos()) ;


    emit centerRect(p1.boundingRect());

}

/**
 * @fn    showGeoFencingMode
 * @brief  if Geo-fencing mode selected - then place anchor 0 in the middle
 *         and draw two concentric circles for the two zones
 *         hide other anchors
 *
 * */
void GraphicsWidget::showGeoFencingMode(bool set)
{
    Anchor *anc;
    int i;

    if(set)
    {
        double a1x = 0.0;
        double a1y = 0.0;
        double a1z = 0.0;

        bool showHistoryTemp = _showHistory;

        //disable tag history and clear any
        setShowTagHistory(false);

        _showHistoryP = showHistoryTemp ; //so that the show/hide tag history will be restored when going back to tracking mode

        //clear all Tag entries
        clearTags();

        //place anchor ID 0 at 0,0
        anchPos(0, a1x, a1y, a1z, true, false);

        zone(0, _zone1Rad, _zone1Red);

        zone(1, _zone2Rad, _zone2Red);

        zone1->setOpacity(1);
        zone2->setOpacity(1);

        for(i=1; i<4; i++)
        {
            anc = this->_anchors.value(i, NULL);

            if(anc)
            {
                anc->a->setOpacity(0);
                anc->ancLabel->setOpacity(0);
            }
        }

        if(0)
        {
            //center the scene on anchor 0
            emit centerAt(a1x, a1y);
        }
        else //zoom out so the zones can be seen
        {

            double rad = _zone2Rad > _zone1Rad ? _zone2Rad : _zone1Rad ;
            QPolygonF p1 = QPolygonF() << QPointF( a1x - rad, a1y) << QPointF(a1x + rad, a1y) << QPointF(a1x, a1y + rad) << QPointF(a1x, a1y - rad) ;

            emit centerRect(p1.boundingRect());
        }


        _geoFencingMode = true;
    }
    else
    {

        //clear all Tag entries
        clearTags();

        for(i=0; i<4; i++)
        {
            anc = this->_anchors.value(i, NULL);

            if(anc)
            {
                if(anc->show)
                {
                    anc->a->setOpacity(1);
                    anc->ancLabel->setOpacity(1);
                }
            }
        }

        zone1->setOpacity(0);
        zone2->setOpacity(0);

        this->_scene->removeItem(zone1);
        delete(zone1);
        zone1 = NULL;

        this->_scene->removeItem(zone2);
        delete(zone2);
        zone2 = NULL;

        setShowTagHistory(_showHistoryP);

        //center the centriod of the triangle given by anchors (0,1,2) points
        //

        centerOnAnchors();

        _geoFencingMode = false;
    }

    ui->tagTable->setColumnHidden(ColumnX,_geoFencingMode); //x
    ui->tagTable->setColumnHidden(ColumnY,_geoFencingMode); //y
    ui->tagTable->setColumnHidden(ColumnZ,_geoFencingMode); //z
    ui->tagTable->setColumnHidden(ColumnR95,_geoFencingMode); //r95
    ui->tagTable->setColumnHidden(ColumnRA1,_geoFencingMode); //range to A1
    ui->tagTable->setColumnHidden(ColumnRA2,_geoFencingMode); //range to A2
    ui->tagTable->setColumnHidden(ColumnRA3,_geoFencingMode); //range to A3
}

/**
 * @fn    setShowTagAncTable
 * @brief  hide/show Tag and Anchor tables
 *
 * */
void GraphicsWidget::setShowTagAncTable(bool anchorTable, bool tagTable, bool ancTagCorr)
{

    if(!tagTable)
    {
        ui->tagTable->hide();
    }
    else
    {
        ui->tagTable->show();
    }

    //hide or show Anchor-Tag correction table
    hideTACorrectionTable(!ancTagCorr);

    if(!anchorTable)
    {
        ui->anchorTable->hide();
    }
    else
    {
        ui->anchorTable->show();
    }

}

/**
 * @fn    setShowTagHistory
 * @brief  if show Tag history is checked then display last N Tag locations
 *         else only show the current/last one
 *
 * */
void GraphicsWidget::setShowTagHistory(bool set)
{
    _busy = true ;

    if(set != _showHistory) //the value has changed
    {
        //for each tag
        if(set == false) //we want to hide history - clear the array
        {
            QMap<quint64, Tag*>::iterator i = _tags.begin();

            while(i != _tags.end())
            {
                Tag *tag = i.value();
                for(int idx=0; idx<_historyLength; idx++ )
                {
                    QAbstractGraphicsShapeItem *tag_p = tag->p[idx];
                    if(tag_p)
                    {
                        tag_p->setOpacity(0); //hide it

                        this->_scene->removeItem(tag_p);
                        delete(tag_p);
                        tag_p = NULL;
                        tag->p[idx] = 0;
                    }
                }
                tag->idx = 0; //reset history
                i++;
            }
        }
        else
        {

        }

        _showHistoryP = _showHistory = set; //update the value
    }

    _busy = false;
}

/**
 * @fn    addNewAnchor
 * @brief  add new Anchor with anchId into the tags QMap
 *
 * */
void GraphicsWidget::addNewAnchor(quint64 anchId, bool show)
{
    Anchor *anc;

    qDebug() << "Add new Anchor: 0x" + QString::number(anchId, 16);

    //insert into table, and create an array to hold history of its positions
    _anchors.insert(anchId,new(Anchor));
    anc = this->_anchors.value(anchId, NULL);

    anc->a = NULL;
    //add text label and show by default
    {
        anc->ancLabel = new QGraphicsSimpleTextItem(NULL);
        anc->ancLabel->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        anc->ancLabel->setZValue(1);
        anc->ancLabel->setText(QString::number(anchId));
        QFont font = anc->ancLabel->font();
        font.setPointSize(FONT_SIZE);
        font.setWeight(QFont::Normal);
        anc->ancLabel->setFont(font);
        this->_scene->addItem(anc->ancLabel);
    }
    anc->show = show;

}

/**
 * @fn    ancRanges
 * @brief  the inputs are anchor to anchor ranges
 *         e.g. 0,1 is anchor 0 to anchor 1
 *         e.g. 0,2 is anchor 0 to anchor 2
 * */
void GraphicsWidget::ancRanges(int a01, int a02, int a12)
{
    if(_busy)
    {
        qDebug() << "(Widget - busy IGNORE) anchor ranges";
    }
    else
    {
        Anchor *anc0 = _anchors.value(0x0, NULL);
        Anchor *anc1 = _anchors.value(0x1, NULL);
        Anchor *anc2 = _anchors.value(0x2, NULL);

        qreal rA01 = ((double)a01)/1000;
        qreal rA02 = ((double)a02)/1000;
        qreal rA12 = ((double)a12)/1000;

        _busy = true ;

        qDebug() << "Anchor Ranges: " << rA01 << rA02 << rA12 ;

        if((anc0 != NULL) && (anc1 != NULL) && (anc2 != NULL))
        {
            QPen pen = QPen(QColor::fromRgb(85, 60, 150, 255));
            qreal x_coord = anc0->p.x() + rA01;
            qreal pwidth = 0.025;
            //qreal penw = pen.widthF() ;
            pen.setWidthF(pwidth);
            //penw = pen.widthF() ;
            QLineF line01 = QLineF(anc0->p, QPointF(x_coord, anc0->p.y()));


            if(!_line01)
            {
                _line01 = this->_scene->addLine(line01, pen);
            }
            else
            {
                _line01->setLine(line01);
            }
        }
        _busy = false ;
    }
}

/**
 * @fn    anchPos
 * @brief  add an anchor to the database and show on screen
 *
 * */
void GraphicsWidget::anchPos(quint64 anchId, double x, double y, double z, bool show, bool update)
{
    if(_busy)
    {
        qDebug() << "(Widget - busy IGNORE) Anch: 0x" + QString::number(anchId, 16) << " " << x << " " << y << " " << z;
    }
    else
    {
        Anchor *anc = NULL;

        _busy = true ;

        anc = _anchors.value(anchId, NULL);

        if(!anc) //add new anchor to the anchors array
        {
            addNewAnchor(anchId, show);
            insertAnchor(anchId, x, y, z, RTLSDisplayApplication::instance()->client()->getTagCorrections(anchId), show);
            anc = this->_anchors.value(anchId, NULL);
        }

        //add the graphic shape (anchor image)
        if (anc->a == NULL)
        {
            QAbstractGraphicsShapeItem *anch = this->_scene->addEllipse(-ANC_SIZE/2, -ANC_SIZE/2, ANC_SIZE, ANC_SIZE);
            anch->setPen(Qt::NoPen);
            anch->setBrush(QBrush(QColor::fromRgb(85, 60, 150, 255)));
            anch->setToolTip("0x"+QString::number(anchId, 16));
            anc->a = anch;
        }

        anc->a->setOpacity(anc->show ? 1.0 : 0.0);
        anc->ancLabel->setOpacity(anc->show ? 1.0 : 0.0);

        anc->ancLabel->setPos(x + 0.15, y + 0.15);
        anc->a->setPos(x, y);

        if(update) //update Table entry
        {
            int r = anchId & 0x3;
            _ignore = true;
            ui->anchorTable->item(r,ColumnX)->setText(QString::number(x, 'f', 2));
            ui->anchorTable->item(r,ColumnY)->setText(QString::number(y, 'f', 2));
            _ignore = false;
        }
        _busy = false ;
        //qDebug() << "(Widget) Anch: 0x" + QString::number(anchId, 16) << " " << x << " " << y << " " << z;
    }
}

/**
 * @fn    anchTableEditing
 * @brief  enable or disable editing of anchor table
 *
 * */
void GraphicsWidget::anchTableEditing(bool enable)
{
    //ui->anchorTable->setEnabled(enable);

    if(!enable)
    {
        for(int i = 0; i<3; i++)
        {
            Qt::ItemFlags flags = ui->anchorTable->item(i,ColumnX)->flags();
            flags = flags &  ~(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled) ;
            ui->anchorTable->item(i,ColumnX)->setFlags(flags);
            ui->anchorTable->item(i,ColumnY)->setFlags(flags);
        }
    }
    else
    {
        for(int i = 0; i<3; i++)
        {
            Qt::ItemFlags flags = ui->anchorTable->item(i,ColumnX)->flags();
            ui->anchorTable->item(i,ColumnX)->setFlags(flags | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
            ui->anchorTable->item(i,ColumnY)->setFlags(flags | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
        }
    }

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GraphicsWidget::on_pushButton_clicked()
{

    bool flag=ui->dockWidget->isHidden();
    if(flag)
    {
        ui->dockWidget->show();
    }
    if(!flag)
    {
        ui->dockWidget->hide();
    }
}

void GraphicsWidget::on_pushButton_2_clicked()
{
    bool flag=ui->anchorTable->isHidden();
    if(flag)
    {
        ui->anchorTable->show();
    }
    if(!flag)
    {
        ui->anchorTable->hide();
    }
}

void GraphicsWidget::on_pushButton_3_clicked()
{
    bool flag=ui->tagTable->isHidden();
    if(flag)
    {
        ui->tagTable->show();
    }
    if(!flag)
    {
        ui->tagTable->hide();
    }
}

void GraphicsWidget::on_pushButton_4_clicked()
{
    model->setTable("book");
    model->select();
}

void GraphicsWidget::on_pushButton_5_clicked()
{
    ui->tagTable->hide();
    ui->anchorTable->hide();
    ui->dockWidget->hide();
}



void GraphicsWidget::on_pushButton_6_clicked()
{
     QSqlQueryModel *model = new QSqlQueryModel(ui->tableView);
     model->setQuery("select 订单号,产品编号 from storage_copy");

     ui->tableView->setModel(model);

}

void GraphicsWidget::on_pushButton_13_clicked()
{
    model1->setTable("storage_copy");
    model1->select();
}

void GraphicsWidget::on_pushButton_8_clicked()
{
    storage *add_storage=new storage();
    add_storage->show();
    add_storage->move((QApplication::desktop()->width() - add_storage->width()) / 2,
                      (QApplication::desktop()->height() - add_storage->height()) / 2);

}

void GraphicsWidget::on_pushButton_7_clicked()
{
    QString query_text;
    QString slect_text;
    query_text=ui->lineEdit->text();
    slect_text="订单号 like'"+query_text+"%'";
    model1->setFilter(slect_text);
    model1->select();
    ui->lineEdit->clear();


}

void GraphicsWidget::on_pushButton_11_clicked()
{

    storage_modify *modify = new storage_modify();
    modify->show();
    modify->move((QApplication::desktop()->width() - modify->width()) / 2,
                 (QApplication::desktop()->height() - modify->height()) / 2);
}

void GraphicsWidget::on_pushButton_9_clicked()
{
    model1->setTable("storage_copy");
    model1->select();
}

void GraphicsWidget::on_pushButton_10_clicked()
{
    storage_delete *deletes = new storage_delete();
    deletes->show();
    deletes->move((QApplication::desktop()->width() - deletes->width()) / 2,
                  (QApplication::desktop()->height() - deletes->height()) / 2);
}

void GraphicsWidget::outputQtableviewtoexcel(QTableView *table,QString title)
{
         QString fileName = QFileDialog::getSaveFileName(table, "保存",QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),"Excel 文件(*.xls *.xlsx)");
         if (fileName!="")
         {
             QAxObject *excel = new QAxObject;
             if (excel->setControl("Excel.Application")) //连接Excel控件
             {
                 excel->dynamicCall("SetVisible (bool Visible)","false");//不显示窗体
                 excel->setProperty("DisplayAlerts", false);//不显示任何警告信息。如果为true那么在关闭是会出现类似“文件已修改，是否保存”的提示
                 QAxObject *workbooks = excel->querySubObject("WorkBooks");//获取工作簿集合
                 workbooks->dynamicCall("Add");//新建一个工作簿
                 QAxObject *workbook = excel->querySubObject("ActiveWorkBook");//获取当前工作簿
                 QAxObject *worksheet = workbook->querySubObject("Worksheets(int)", 1);

                 int i,j;
                 //QTablewidget 获取数据的列数

               //  int colcount=table->columnCount();
                  //QTablewidget 获取数据的行数
               //  int rowscount=table->rowCount()

                 //QTableView 获取列数
            int colcount=table->model()->columnCount();
                 //QTableView 获取行数
            int rowcount=table->model()->rowCount();

                 QAxObject *cell,*col;
                 //标题行
                 cell=worksheet->querySubObject("Cells(int,int)", 1,4);
                 cell->dynamicCall("SetValue(const QString&)", title);
                 cell->querySubObject("Font")->setProperty("Size", 18);
                 //调整行高
                 worksheet->querySubObject("Range(const QString&)", "1:1")->setProperty("RowHeight", 30);
                 //合并标题行
                 QString cellTitle;
                 cellTitle.append("A1:");
                 cellTitle.append(QChar(colcount - 1 + 'A'));
                 cellTitle.append(QString::number(1));
                 QAxObject *range = worksheet->querySubObject("Range(const QString&)", cellTitle);
//                 range->setProperty("WrapText", true);
//                 range->setProperty("MergeCells", true);
//                 range->setProperty("HorizontalAlignment", -4108);//xlCenter
//                 range->setProperty("VerticalAlignment", -4108);//xlCenter
                 //列标题
                 for(i=0;i<colcount;i++)
                 {
                     QString columnName;
                     columnName.append(QChar(i  + 'A'));
                     columnName.append(":");
                     columnName.append(QChar(i + 'A'));
                     col = worksheet->querySubObject("Columns(const QString&)", columnName);
                     col->setProperty("ColumnWidth", table->columnWidth(i)/6);
                     cell=worksheet->querySubObject("Cells(int,int)", 2, i+1);
                     //QTableWidget 获取表格头部文字信息
                     //columnName=table->horizontalHeaderItem(i)->text();
                     //QTableView 获取表格头部文字信息
                     columnName=table->model()->headerData(i,Qt::Horizontal,Qt::DisplayRole).toString();
                     cell->dynamicCall("SetValue(const QString&)", columnName);
                     cell->querySubObject("Font")->setProperty("Bold", true);
                     cell->querySubObject("Interior")->setProperty("Color",QColor(191, 191, 191));
                     cell->setProperty("HorizontalAlignment", -4108);//xlCenter
                     cell->setProperty("VerticalAlignment", -4108);//xlCenter
                 }

                //数据区

                //QTableWidget 获取表格数据部分
                 /*
                 for(i=0;i<rowcount;i++){
                     for (j=0;j<colcount;j++)
                     {
                         worksheet->querySubObject("Cells(int,int)", i+3, j+1)->dynamicCall("SetValue(const QString&)", table->item(i,j)?table->item(i,j)->text():"");
                     }
                 }
*/

                //QTableView 获取表格数据部分
                  for(i=0;i<rowcount;i++) //行数
                     {
                         for (j=0;j<colcount;j++)   //列数
                         {
                             QModelIndex index = table->model()->index(i, j);
                             QString strdata=table->model()->data(index).toString();
                             worksheet->querySubObject("Cells(int,int)", i+3, j+1)->dynamicCall("SetValue(const QString&)", strdata);
                         }
                     }


                 //画框线
                 QString lrange;
                 lrange.append("A2:");
                 lrange.append(colcount - 1 + 'A');
                 //lrange.append(QString::number(table->rowSpan + 2));
                 range = worksheet->querySubObject("Range(const QString&)", lrange);
//                 range->querySubObject("Borders")->setProperty("LineStyle", QString::number(1));
//                 range->querySubObject("Borders")->setProperty("Color", QColor(0, 0, 0));
                 //调整数据区行高
                 QString rowsName;
                 rowsName.append("2:");
                // rowsName.append(QString::number(table->rowCount() + 2));
                 range = worksheet->querySubObject("Range(const QString&)", rowsName);
                 //range->setProperty("RowHeight", 20);
                 workbook->dynamicCall("SaveAs(const QString&)",QDir::toNativeSeparators(fileName));//保存至fileName
                 workbook->dynamicCall("Close()");//关闭工作簿
                 excel->dynamicCall("Quit()");//关闭excel
                 delete excel;
                 excel=NULL;
                 if (QMessageBox::question(NULL,"完成","文件已经导出，是否现在打开？",QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes)
                 {
                     QDesktopServices::openUrl(QUrl("file:///" + QDir::toNativeSeparators(fileName)));
                 }
             }
             else
             {
                 QMessageBox::warning(NULL,"错误","未能创建 Excel 对象，请安装 Microsoft Excel。",QMessageBox::Apply);
             }
         }


}

void GraphicsWidget::outputQtablewidgettoexcel(QTableWidget *table,QString title)
{
         QString fileName = QFileDialog::getSaveFileName(table, "保存",QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),"Excel 文件(*.xls *.xlsx)");
         if (fileName!="")
         {
             QAxObject *excel = new QAxObject;
             if (excel->setControl("Excel.Application")) //连接Excel控件
             {
                 excel->dynamicCall("SetVisible (bool Visible)","false");//不显示窗体
                 excel->setProperty("DisplayAlerts", false);//不显示任何警告信息。如果为true那么在关闭是会出现类似“文件已修改，是否保存”的提示
                 QAxObject *workbooks = excel->querySubObject("WorkBooks");//获取工作簿集合
                 workbooks->dynamicCall("Add");//新建一个工作簿
                 QAxObject *workbook = excel->querySubObject("ActiveWorkBook");//获取当前工作簿
                 QAxObject *worksheet = workbook->querySubObject("Worksheets(int)", 1);

                 int i,j;
                 //QTablewidget 获取数据的列数

                int colcount=table->columnCount();
                //QTablewidget 获取数据的行数
                int rowcount=table->rowCount();

                 //QTableView 获取列数
           // int colcount=table->model()->columnCount();
                 //QTableView 获取行数
          //  int rowcount=table->model()->rowCount();

                 QAxObject *cell,*col;
                 //标题行
                 cell=worksheet->querySubObject("Cells(int,int)", 1,4);
                 cell->dynamicCall("SetValue(const QString&)", title);
                 cell->querySubObject("Font")->setProperty("Size", 18);
                 //调整行高
                 worksheet->querySubObject("Range(const QString&)", "1:1")->setProperty("RowHeight", 30);
                 //合并标题行
                 QString cellTitle;
                 cellTitle.append("A1:");
                 cellTitle.append(QChar(colcount - 1 + 'A'));
                 cellTitle.append(QString::number(1));
                 QAxObject *range = worksheet->querySubObject("Range(const QString&)", cellTitle);
//                 range->setProperty("WrapText", true);
//                 range->setProperty("MergeCells", true);
//                 range->setProperty("HorizontalAlignment", -4108);//xlCenter
//                 range->setProperty("VerticalAlignment", -4108);//xlCenter
                 //列标题
                 for(i=0;i<colcount;i++)
                 {
                     QString columnName;
                     columnName.append(QChar(i  + 'A'));
                     columnName.append(":");
                     columnName.append(QChar(i + 'A'));
                     col = worksheet->querySubObject("Columns(const QString&)", columnName);
                     col->setProperty("ColumnWidth", table->columnWidth(i)/6);
                     cell=worksheet->querySubObject("Cells(int,int)", 2, i+1);
                     //QTableWidget 获取表格头部文字信息
                     //columnName=table->horizontalHeaderItem(i)->text();
                     //QTableView 获取表格头部文字信息
                     columnName=table->model()->headerData(i,Qt::Horizontal,Qt::DisplayRole).toString();
                     cell->dynamicCall("SetValue(const QString&)", columnName);
                     cell->querySubObject("Font")->setProperty("Bold", true);
                     cell->querySubObject("Interior")->setProperty("Color",QColor(191, 191, 191));
                     cell->setProperty("HorizontalAlignment", -4108);//xlCenter
                     cell->setProperty("VerticalAlignment", -4108);//xlCenter
                 }

                //数据区

                //QTableWidget 获取表格数据部分

                 for(i=0;i<rowcount;i++){
                     for (j=0;j<colcount;j++)
                     {
                         worksheet->querySubObject("Cells(int,int)", i+3, j+1)->dynamicCall("SetValue(const QString&)", table->item(i,j)?table->item(i,j)->text():"");
                     }
                 }


                //QTableView 获取表格数据部分
//                  for(i=0;i<rowcount;i++) //行数
//                     {
//                         for (j=0;j<colcount;j++)   //列数
//                         {
//                             QModelIndex index = table->model()->index(i, j);
//                             QString strdata=table->model()->data(index).toString();
//                             worksheet->querySubObject("Cells(int,int)", i+3, j+1)->dynamicCall("SetValue(const QString&)", strdata);
//                         }
//                     }


                 //画框线
                 QString lrange;
                 lrange.append("A2:");
                 lrange.append(colcount - 1 + 'A');
                 //lrange.append(QString::number(table->rowSpan + 2));
                 range = worksheet->querySubObject("Range(const QString&)", lrange);
//                 range->querySubObject("Borders")->setProperty("LineStyle", QString::number(1));
//                 range->querySubObject("Borders")->setProperty("Color", QColor(0, 0, 0));
                 //调整数据区行高
                 QString rowsName;
                 rowsName.append("2:");
                // rowsName.append(QString::number(table->rowCount() + 2));
                 range = worksheet->querySubObject("Range(const QString&)", rowsName);
                 //range->setProperty("RowHeight", 20);
                 workbook->dynamicCall("SaveAs(const QString&)",QDir::toNativeSeparators(fileName));//保存至fileName
                 workbook->dynamicCall("Close()");//关闭工作簿
                 excel->dynamicCall("Quit()");//关闭excel
                 delete excel;
                 excel=NULL;
                 if (QMessageBox::question(NULL,"完成","文件已经导出，是否现在打开？",QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes)
                 {
                     QDesktopServices::openUrl(QUrl("file:///" + QDir::toNativeSeparators(fileName)));
                 }
             }
             else
             {
                 QMessageBox::warning(NULL,"错误","未能创建 Excel 对象，请安装 Microsoft Excel。",QMessageBox::Apply);
             }
         }


}

void GraphicsWidget::on_pushButton_12_clicked()
{
    //第一个参数是页面上的表格，第二个是导出文件的表头数据
     //void FaJianDialog::Table2ExcelByHtml(QTableWidget *table,QString title)
    QString title=" 入库报表";
    QTableView *table;
    table=ui->tableView_2;
    outputQtableviewtoexcel(table,title);

}

void GraphicsWidget::on_pushButton_22_clicked()
{
    model2->setTable("storage_copy");
    model2->select();
}

void GraphicsWidget::empytOrdermap()
{
    ConnectDatabase::openDatabase();  //为什么要重复打开databas？？？

    QSqlQuery query;
    QSqlRecord record;
    QString order_id;
    query.exec("select * from storage_copy");
    while(query.next())
    {
        record = query.record();
        order_id= query.value(record.indexOf("订单号")).toString();
        order_num_map.insert(order_id ,0);
    }
}

void GraphicsWidget::on_pushButton_15_clicked()               //从库存添加到当前出库
{
    QDateTime current_data=QDateTime::currentDateTime();
    QString current_time=current_data.toString("yyyy-MM-dd hh:mm:ss");


    //订单号、产品名称、产品编号、标签编号、出库数量、出库时间、仓库、管理员
    QString str1;QString str2;QString str3;QString str4;QString str5; QString str6;QString str7;QString str8;

    QString order_id;  //库存订单编号
    QString order_out_id; //需出库订单编号

    int order_num;   //库存订单总数量
    int order_num_all;//需出库订单数量
    int repead_flag=0;

    if(ui->tableView_3->currentIndex().row()<0){
        QMessageBox::warning(this, tr("提示"), tr("请添加出库订单！"));
        return ;
    }

    QModelIndex index=ui->tableView_3->currentIndex();

    order_id=model2->record(index.row()).value("订单号").toString();
    str2=model2->record(index.row()).value("产品名称").toString();
    str3=model2->record(index.row()).value("产品编号").toString();
    str4=model2->record(index.row()).value("标签编号").toString();
    order_num=model2->record(index.row()).value("数量").toInt();
    //str6=model2->record(index.row()).value("入库时间").toString();
    str7=model2->record(index.row()).value("仓库").toString();
    str8=model2->record(index.row()).value("管理员").toString();

    order_itera=order_num_map.find(order_id);//find 订单号
    order_num_all=1+order_itera.value();
    qDebug() << "order_num--------" << order_num ;

    if(order_num_all<=order_num)
    {
       order_id.append(str1);
       for(int i=0;i<ui->tableWidget_4->rowCount();i++)
       {
           order_out_id=ui->tableWidget_4->item(i,0)->text();
           if(order_out_id==order_id)
           {

               QTableWidgetItem *item=new QTableWidgetItem(QString::number(order_num_all));
               ui->tableWidget_4->setItem(i,4,item);
               repead_flag=1;
               break;
           }
       }
       if(repead_flag==0)
       {
           QTableWidgetItem *item0 = new QTableWidgetItem(order_id);
           QTableWidgetItem *item1 = new QTableWidgetItem(str2);
           QTableWidgetItem *item2 = new QTableWidgetItem(str3);
           QTableWidgetItem *item3 = new QTableWidgetItem(str4);
           QTableWidgetItem *item4 = new QTableWidgetItem(QString::number(order_num_all));
           QTableWidgetItem *item5 = new QTableWidgetItem(current_time);
           QTableWidgetItem *item6 = new QTableWidgetItem(str7);
           QTableWidgetItem *item7 = new QTableWidgetItem(str8);
           int row=ui->tableWidget_4->rowCount();
           ui->tableWidget_4->setRowCount(row+1);
           ui->tableWidget_4->setItem(row, 0, item0);
           ui->tableWidget_4->setItem(row, 1, item1);
           ui->tableWidget_4->setItem(row, 2, item2);
           ui->tableWidget_4->setItem(row, 3, item3);
           ui->tableWidget_4->setItem(row, 4, item4);
           ui->tableWidget_4->setItem(row, 5, item5);
           ui->tableWidget_4->setItem(row, 6, item6);
           ui->tableWidget_4->setItem(row, 7, item7);

       }
       order_num_map[order_id]=order_num_all;
       ui->lineEdit_3->setText(order_id);
       ui->lineEdit_4->setText(QString::number(order_num_all));


    }
    else{
        QMessageBox::warning(this, tr("警告"), tr("出库订单数量超过库存！"));
        order_num_all=order_num_all-1;

    }

}


void GraphicsWidget::on_pushButton_23_clicked()
{
   QString order_id;
   int row=ui->tableWidget_4->rowCount();
   for(int i=0;i<row;i++)              //清零待出库订单数量
   {
      order_id=ui->tableWidget_4->item(i,0)->text();
      order_num_map[order_id] = 0;
   }
   ui->tableWidget_4->clearContents(); //只清除表中数据，不清除表头数据
   ui->tableWidget_4->setRowCount(0);  //清除行
}

void GraphicsWidget::on_pushButton_20_clicked()
{
    QString title=" 当前库存";
    QTableView *table;
    table=ui->tableView_3;
    outputQtableviewtoexcel(table,title);
}

void GraphicsWidget::on_pushButton_21_clicked()
{
    QString title=" 当前出库";
    QTableWidget *table;
    table=ui->tableWidget_4;
    GraphicsWidget::outputQtablewidgettoexcel(table, title);

}

void GraphicsWidget::on_pushButton_17_clicked()
{
    ui->lineEdit_3->clear();
    ui->lineEdit_4->clear();
}

void GraphicsWidget::on_pushButton_16_clicked()
{
    QString order_id;

    bool focus=ui->tableWidget_4->isItemSelected(ui->tableWidget_4->currentItem());
    if(focus)  //判断是否选中任意一行，否则无法获取行
    {
    int allrow=ui->tableWidget_4->rowCount();

    if(allrow>0) //表中必须有数据，否则无法移出，发生异常
    {
        int row=ui->tableWidget_4->currentItem()->row();
        order_id=ui->tableWidget_4->item(row,0)->text();
        ui->tableWidget_4->removeRow(row);
        order_num_map[order_id] = 0;
    }
    else
    {
        QMessageBox::warning(this, tr("警告"), tr("当前出库为空，无法移除"));

    }
    }
}


void GraphicsWidget::on_pushButton_18_clicked()
{
    ConnectDatabase::openDatabase();
    QDateTime current_data=QDateTime::currentDateTime();
    QString current_time=current_data.toString("yyyy-MM-dd hh:mm:ss");
    QSqlQuery query;
    QString str1;QString str2;QString str3;QString str4;QString str5; QString str6;QString str7;QString str8;
    int order_num_all;
    QString out_order_id;
    int repeat_flag = 0;

    QString order_id=ui->lineEdit_3->text().trimmed();       //读取输入
    int order_num=ui->lineEdit_4->text().trimmed().toInt();


    if(order_id!=""&&order_num!=0)
    {
        query.exec("select * from storage_copy where 订单号 = '"+order_id+"'");
        if(query.next())
        {
            QSqlRecord record=query.record();
            order_itera=order_num_map.find(order_id);
            order_num_all=order_num+order_itera.value();  //现要出库适量+以在map中数量=总出库数量
            if(order_num_all<=query.value(record.indexOf("数量")).toInt())
            {
                //str1  order_id
                //获取表中数据
                str2=query.value(record.indexOf("产品名称")).toString();  //产品名称
                str3=query.value(record.indexOf("产品编号")).toString(); //产品编号
                str4=query.value(record.indexOf("标签编号")).toString();//标签编号
                //str5 order_num
                //str6 入库时间  current_time
                str7=query.value(record.indexOf("仓库")).toString();//仓库
                str8=query.value(record.indexOf("管理员")).toString();//管理员

                for(int i=0;i<ui->tableWidget_4->rowCount();i++)
                {
                    out_order_id=ui->tableWidget_4->item(i,0)->text();
                    if(out_order_id==order_id)   //如果存在，则出库数量加当前数量
                    {
                        QTableWidgetItem *item= new QTableWidgetItem(QString::number(order_num_all));
                        ui->tableWidget_4->setItem(i,4,item);
                        repeat_flag=1;
                        break;
                    }
                }
                if(repeat_flag==0)         //如果不存在，则重新创建新行
                {
                    QTableWidgetItem *item1= new QTableWidgetItem(order_id);
                    QTableWidgetItem *item2= new QTableWidgetItem(str2);
                    QTableWidgetItem *item3= new QTableWidgetItem(str3);
                    QTableWidgetItem *item4= new QTableWidgetItem(str4);
                    QTableWidgetItem *item5= new QTableWidgetItem(QString::number(order_num_all));
                    QTableWidgetItem *item6= new QTableWidgetItem(current_time);
                    QTableWidgetItem *item7= new QTableWidgetItem(str7);
                    QTableWidgetItem *item8= new QTableWidgetItem(str8);
                    int row=ui->tableWidget_4->rowCount();
                    ui->tableWidget_4->setRowCount(row+1);
                    ui->tableWidget_4->setItem(row,0,item1);
                    ui->tableWidget_4->setItem(row,1,item2);
                    ui->tableWidget_4->setItem(row,2,item3);
                    ui->tableWidget_4->setItem(row,3,item4);
                    ui->tableWidget_4->setItem(row,4,item5);
                    ui->tableWidget_4->setItem(row,5,item6);
                    ui->tableWidget_4->setItem(row,6,item7);
                    ui->tableWidget_4->setItem(row,7,item8);
                }
                order_num_map[order_id]=order_num_all;
            }
            else{
                QMessageBox::warning(this, tr("警告"), tr("货物数量超过库存！"));
                order_num_all-=order_num;
            }


        }
        else
              QMessageBox::warning(this, tr("警告"), tr("不存在该订单！请先入库！"));
    }
    else
        QMessageBox::warning(this, tr("警告"), tr("输入不正确"));
    ui->lineEdit_4->setText("");

}

void GraphicsWidget::on_pushButton_19_clicked()
{
    ConnectDatabase::openDatabase();
    QString order_id;
    QSqlQuery query;
    QSqlQuery query_insert;
    QSqlQuery query_updata;

    //订单号、产品名称、产品编号、标签编号、出库数量、出库时间、仓库、管理员
    QString str1;QString str2;QString str3;QString str4;QString str5; QString str6;QString str7;QString str8;
    int row=ui->tableWidget_4->rowCount();
    if(row==0)
        QMessageBox::warning(this,tr("警告"),tr("请选择当前出库订单！"));
    //将当前出库表中数据插入outstorage,并且减少storage_copy中相应订单货物的数量
    for(int i=0;i<row;i++)
    {
        order_id=ui->tableWidget_4->item(i,0)->text();
        str2=ui->tableWidget_4->item(i,1)->text();
        str3=ui->tableWidget_4->item(i,2)->text();
        str4=ui->tableWidget_4->item(i,3)->text();
        str5=ui->tableWidget_4->item(i,4)->text();
        str6=ui->tableWidget_4->item(i,5)->text();
        str7=ui->tableWidget_4->item(i,6)->text();
        str8=ui->tableWidget_4->item(i,7)->text();


        QString properties="订单号,产品名称,产品编号,标签编号,出库数量,出库时间,仓库,管理员";
        QString values=":no1, :no2, :no3, :no4, :no5, :no6, :no7, :no8";

        query_insert.prepare("insert into outstorage ("+ properties + ") values ("+ values + ")");
        query_insert.bindValue(":no1",order_id);
        query_insert.bindValue(":no2",str2);
        query_insert.bindValue(":no3",str3);
        query_insert.bindValue(":no4",str4);
        query_insert.bindValue(":no5",str5);
        query_insert.bindValue(":no6",str6);
        query_insert.bindValue(":no7",str7);
        query_insert.bindValue(":no8",str8);
        query_insert.exec();



        //将当前库存中已出库减去已出库货物数量
        int order_num;  //定义当前出库货物数量
        query.exec("select 数量 from storage_copy where 订单号 ='"+order_id+"'");
        if(query.next())
        {
            QSqlRecord record=query.record();
            order_num=query.value(record.indexOf("数量")).toInt();  //获取当前库存订单数量
        }
        int order_num_out=str5.toInt();
        int order_num_new=order_num-order_num_out;
        query.prepare("update storage_copy set 数量 = :one where 订单号 ='"+order_id+"'");
        query.bindValue(":one",order_num_new);
        query.exec();
        model2->setTable("storage_copy");  //刷新当前库存
        model2->select();
    }




    //完成出库后清空当前出库订单
    for(int i=0;i<row;i++)              //清零待出库订单数量
    {
       order_id=ui->tableWidget_4->item(i,0)->text();
       order_num_map[order_id] = 0;
    }
    ui->tableWidget_4->clearContents(); //只清除表中数据，不清除表头数据
    ui->tableWidget_4->setRowCount(0);  //清除行
    //更新已出库订单
    model3->setTable("outstorage");
    model3->select();




}

void GraphicsWidget::on_pushButton_24_clicked()
{
    model3->setTable("outstorage");
    model3->select();
}

void GraphicsWidget::on_pushButton_25_clicked()
{
    QString title="         出库报表";
    QTableView *table;
    table=ui->tableView_5;
    GraphicsWidget::outputQtableviewtoexcel(table, title);
}

void GraphicsWidget::on_pushButton_27_clicked()
{
    if(ui->groupBox_3->isHidden())
    {
        ui->groupBox_3->show();
    }
    else
   {
       ui->groupBox_3->hide();
   }
}

void GraphicsWidget::on_pushButton_28_clicked()
{
    if(ui->groupBox_2->isHidden())
    {
        ui->groupBox_2->show();
        ui->groupBox->show();
        ui->pushButton_15->show();
        ui->pushButton_16->show();
        ui->pushButton_19->show();
        ui->label_3->show();
        ui->label_4->show();
        ui->lineEdit_3->show();
        ui->lineEdit_4->show();
        ui->pushButton_17->show();
        ui->pushButton_18->show();

    }
    else
   {
       ui->groupBox_2->hide();
       ui->groupBox->hide();
       ui->pushButton_15->hide();
       ui->pushButton_16->hide();
       ui->pushButton_19->hide();
       ui->label_3->hide();
       ui->label_4->hide();
       ui->lineEdit_3->hide();
       ui->lineEdit_4->hide();
       ui->pushButton_17->hide();
       ui->pushButton_18->hide();
   }
}

void GraphicsWidget::on_pushButton_31_clicked()
{
    //获取选中的行
       int curRow = ui->tableView_5->currentIndex().row();

       if(curRow==-1)
       {
           QMessageBox::warning(this,tr("提示"),tr("没有选中行") );
       }
       else
       {
        //删除该行
         model3->removeRow(curRow);

         int ok = QMessageBox::warning(this,tr("删除当前行!"),
                                     tr("你确定删除当前行吗？"),
                                     QMessageBox::Yes,QMessageBox::No);
         if(ok == QMessageBox::No)
          {
            model3->revertAll(); //如果不删除，则撤销
           }
          else model3->submitAll(); //否则提交，在数据库中删除该行
       }
}

void GraphicsWidget::on_pushButton_26_clicked()
{
    if(ui->groupBox_4->isHidden())
    {
        ui->groupBox_4->show();
        ui->groupBox_5->hide();
    }
    else
    ui->groupBox_5->hide();

}

void GraphicsWidget::on_pushButton_29_clicked()
{
    if(ui->groupBox_5->isHidden())
    {
        ui->groupBox_5->show();
        ui->groupBox_4->hide();
    }
    else
        ui->groupBox_4->hide();
}

void GraphicsWidget::on_pushButton_39_clicked()
{
    ui->groupBox_4->show();
    ui->groupBox_5->show();
}

void GraphicsWidget::on_pushButton_36_clicked()
{
    QSqlQueryModel *model = new QSqlQueryModel(ui->tableView_6);
    model->setQuery("select * from storage_copy");
    ui->tableView_6->setModel(model);


}

void GraphicsWidget::on_pushButton_38_clicked()
{
    QSqlQueryModel *model = new QSqlQueryModel(ui->tableView_4);
    model->setQuery("select * from outstorage");
    ui->tableView_4->setModel(model);
}



void GraphicsWidget::on_pushButton_30_clicked()
{
    QString query_text;
    QString select_text;
    QString method;
    method=ui->comboBox->currentText().trimmed();
    query_text=ui->lineEdit_5->text();

    select_text=""+method+" like '"+query_text+"%'";

    QSqlTableModel *model;
    model=new QSqlTableModel(this);
    model->setTable("storage_copy");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    model->setFilter(select_text);

    model->select(); //选取整个表的所有行
    ui->tableView_6->setModel(model);
    ui->lineEdit_5->clear();

}

void GraphicsWidget::on_pushButton_37_clicked()
{
    QString query_text;
    QString select_text;
    QString method;
    method=ui->comboBox_2->currentText().trimmed();
    query_text=ui->lineEdit_6->text();

    select_text=""+method+" like '"+query_text+"%'";

    QSqlTableModel *model;
    model=new QSqlTableModel(this);
    model->setTable("outstorage");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    model->setFilter(select_text);

    model->select(); //选取整个表的所有行
    ui->tableView_4->setModel(model);
    ui->lineEdit_6->clear();
}

void GraphicsWidget::on_pushButton_48_clicked()
{
    QSqlQueryModel *model = new QSqlQueryModel(ui->tableView_7);
    model->setQuery("select * from biaoshi");
    ui->tableView_7->setModel(model);
}

void GraphicsWidget::on_pushButton_49_clicked()
{
    QString qurey_text;
    qurey_text=ui->lineEdit_9->text();
    QSqlQueryModel *model = new QSqlQueryModel(ui->tableView_7);
    model->setQuery("select * from biaoshi where 标签编号='"+qurey_text+"'");
    ui->tableView_7->setModel(model);
    ui->lineEdit_9->clear();


}

void GraphicsWidget::on_pushButton_43_clicked()
{
    QString qurey_text;
    qurey_text=ui->lineEdit_10->text();
    QSqlQueryModel *model = new QSqlQueryModel(ui->tableView_7);
    model->setQuery("select * from biaoshi where 标签标识='"+qurey_text+"'");
    ui->tableView_7->setModel(model);
    ui->lineEdit_10->clear();

}
