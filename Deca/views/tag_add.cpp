#include "tag_add.h"
#include "ui_tag_add.h"
#include <QSqlQuery>
#include <QMessageBox>


tag_add::tag_add(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::tag_add)
{
    ui->setupUi(this);
    ui->lineEdit->setFocusPolicy(Qt::NoFocus);
    ui->lineEdit_2->setFocusPolicy(Qt::NoFocus);//不可编辑
}

tag_add::~tag_add()
{
    delete ui;
}

void tag_add:: showid(QString data,QString data2)
{
    QString id1=data,id2=data2;
    ui->lineEdit->setText(id1);
    ui->lineEdit_2->setText(id2);
}

void tag_add::on_pushButton_clicked()
{
    QString str1,str2,str3,str4,str5,str6,str7;
    QSqlQuery query;
    str1=ui->lineEdit->text().trimmed();
    str2=ui->lineEdit_2->text().trimmed();
    str3=ui->lineEdit_3->text().trimmed();
    str4=ui->lineEdit_4->text().trimmed();
    str5=ui->lineEdit_5->text().trimmed();
    str6=ui->lineEdit_6->text().trimmed();
    str7=ui->lineEdit_7->text().trimmed();
    query.exec("select 标签编号,标签标识 from biaoshi where 标签编号='"+str1+"'||标签标识='"+str2+"'");
    if(!query.next())
    {
        if(str1!=""&&str2!="")
        {
            query.prepare(tr("insert into biaoshi values(:one,:two,:three,:four,:five,:six,:seven)"));
            query.bindValue(":one",str1);
            query.bindValue(":two",str2);
            query.bindValue(":three",str3);
            query.bindValue(":four",str4);
            query.bindValue(":five",str5);
            query.bindValue(":six",str6);
            query.bindValue(":seven",str7);
            query.exec();
            if(query.isActive())
            {
                query.numRowsAffected();
                QMessageBox::information(this, tr("信息"), tr("标签增加成功"));
                this->close();
            }
            else
            {
                QMessageBox::warning(this, tr("警告"), tr("标签增加失败！try again"));
                this->close();
            }

        }
        else
        {
            QMessageBox::warning(this, tr("警告"), tr("标签编号与标识不能为空"));
            this->close();
           }

    }
    else
    {
        QMessageBox::warning(this, tr("警告"), tr("标签编号与标识是唯一的，不能重复！"));
        this->close();
        return ;
    }

}

void tag_add::on_pushButton_3_clicked()
{
    this->close();
}

void tag_add::on_pushButton_2_clicked()
{
    ui->lineEdit_3->clear();
    ui->lineEdit_4->clear();
    ui->lineEdit_5->clear();
    ui->lineEdit_6->clear();
    ui->lineEdit_7->clear();
}
