/*************************************************************************
    mainwindow.h
    :Main Window

    Copyright (C) 2015 wwylele

    This file is part of MApRX.

    MApRX is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MApRX is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MApRX.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../KssuFile.h"
#include <QMainWindow>
#include <QTreeWidget>
#include <QTimer>
#include <QImage>
#include <QPixmap>
#include <stack>
#include <memory>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QVector>
#include <QPainter>


namespace Ui {
    class MainWindow;
}

class MainWindow;
class ItemTableModal:public QAbstractTableModel{
    Q_OBJECT
    MainWindow* pMainWindow;
    KfMap* pMap;
public:
    ItemTableModal(MainWindow* _pMainWindow,QObject *parent=0);
    void itemChanged(u8 id);
    QModelIndex getIndex(int row,int column);
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex & index) const Q_DECL_OVERRIDE ;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;

};

class ItemTableDelegate:public QStyledItemDelegate{
    Q_OBJECT
private:
    MainWindow* pMainWindow;
public:
    ItemTableDelegate(MainWindow *pMainWindow, QWidget *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const Q_DECL_OVERRIDE;
};
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    class MapOperation{
    public:
        static KfMap* pMap;
        static MainWindow* pMainWindow;
        virtual void doOperation()=0;
        virtual MapOperation* generateReversal()=0;//Call before doOperation
        virtual ~MapOperation();
        QString toolTip;
    };
    class MoEditCell:public MapOperation{
    private:
        u16 x,y;
        u16 blockIdToBe;
    public:
        MoEditCell(u16 _x,u16 _y,u16 toBe);
        void doOperation();
        MapOperation* generateReversal();
    };
    class MoEditItemBasic:public MapOperation{
    private:
        u8 itemId;
        KfMap::Item itemBasicToBe;
    public:
        MoEditItemBasic(u8 _itemId, const KfMap::Item& toBe);
        void doOperation();
        MapOperation* generateReversal();
    };
    class MoSwapItem:public MapOperation{
    private:
        u8 firstItemId;
    public:
        MoSwapItem(u8 itemId);
        void doOperation();
        MapOperation* generateReversal();
    };
    class MoDeleteItem:public MapOperation{
    private:
        u8 itemId;
    public:
        MoDeleteItem(u8 _itemId);
        void doOperation();
        MapOperation* generateReversal();
    };
    class MoNewItem:public MapOperation{
    private:
        u8 itemId;
        KfMap::RipeItem itemToInsert;
    public:
        MoNewItem(u8 _itemId,const KfMap::RipeItem& item);
        void doOperation();
        MapOperation* generateReversal();
    };
    class MoEditMetaData:public MapOperation{
    private:
        KfMap::MetaData_Struct metaDataToBe;
    public:
        MoEditMetaData(const KfMap::MetaData_Struct& metaData);
        void doOperation();
        MapOperation* generateReversal();
    };
    class MoResizeMap:public MapOperation{
    private:
        u8 widthToBe,heightToBe;
        KfMap::Align hAlign,vAlign;
    public:
        MoResizeMap(u8 width, u8 height, KfMap::Align hA, KfMap::Align vA);
        void doOperation();
        MapOperation* generateReversal();
    };
    class MoPasteMap:public MapOperation{
    private:
        KfMap mapToBe;
    public:
        MoPasteMap(const KfMap& map);
        void doOperation();
        MapOperation* generateReversal();
    };
    class MoEditCellScript:public MapOperation{
    private:
        std::vector<KfMap::Script> scriptsToBe;
        u16 x,y;
    public:
        MoEditCellScript(const std::vector<KfMap::Script>& scripts,u16 x,u16 y);
        void doOperation();
        MapOperation* generateReversal();
    };
    class MoEditItemScript:public MapOperation{
    private:
        std::vector<KfMap::Script> scriptsToBe;
        u8 itemId;
    public:
        MoEditItemScript(const std::vector<KfMap::Script>& scripts,u8 itemId);
        void doOperation();
        MapOperation* generateReversal();
    };

    std::stack<std::unique_ptr<MapOperation>> undoStack;//store the reversal of history operation
    std::stack<std::unique_ptr<MapOperation>> redoStack;//store the reversal of operation pop from undoStack

    void clearOperationStack();
    void doOperation(MapOperation *op);
public slots:
    void undo();
    void redo();
    void editItemScripts(u8 itemId);

protected:

    ItemTableModal itemTableModal;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    class PltTransit:public QVector<QRgb>{
    public:
        PltTransit():QVector<QRgb>(256){data()[0]=0x00000000;}
        void doTransit(const KfPlt& plt){
            for(int i=1;i<256;i++){
                data()[i]=plt.getColors(i).toARGB32();
            }
        }
    };
    class BlockSetTransit{
        std::unique_ptr<std::unique_ptr<QPixmap>[]> pImages;
        u32 size;
        void adjustSize(u32 blockCount){
            if(size!=blockCount){
                size=blockCount;
                pImages.reset(new std::unique_ptr<QPixmap>[size]);
                for(u32 i=0;i<size;i++){
                    pImages[i].reset(new QPixmap(24,24));
                }
            }
        }
        void doTransitOne(KfBlockSet& blocks,KfTileSet& tiles,QVector<QRgb> colors,int id){
            pImages[id].get()->fill(Qt::transparent);
            QPainter painter(pImages[id].get());
            for(int j=0;j<9;j++){
                QImage tile(tiles[
                            blocks[id].data[j]&TILE_ID_MASK
                        ].data,8,8,QImage::Format_Indexed8);
                tile.setColorTable(colors);
                painter.drawImage((j%3)*8,j/3*8,
                                  tile.mirrored
                                  (blocks[id].data[j]&FLIP_X?true:false,
                                   blocks[id].data[j]&FLIP_Y?true:false));


            }
        }

    public:
        BlockSetTransit():size(0){}
        QPixmap& operator[](int i){
            return *pImages[i].get();
        }

        void doTransit(KfBlockSet& blocks,KfTileSet& tiles,QVector<QRgb> colors){
            adjustSize(blocks.blockCount());
            for(u32 i=0;i<size;i++){
                for(int j=0;j<9;j++){
                    if(tiles.getTickCounter()[blocks[i].data[j]&TILE_ID_MASK]){
                        doTransitOne(blocks,tiles,colors,i);
                        break;
                    }
                }
            }
        }
        void doAllTransit(KfBlockSet& blocks,KfTileSet& tiles,QVector<QRgb> colors){
            adjustSize(blocks.blockCount());
            for(u32 i=0;i<size;i++){
                doTransitOne(blocks,tiles,colors,i);
            }
        }
    };
    class ScrTransit{
    public:
        QPixmap pixmap;
    private:
        u16 w,h;
        void adjustSize(u16 nw,u16 nh){
            if(nw!=w || nh!=h){
                w=nw;
                h=nh;
                pixmap=QPixmap(w*8,h*8);
            }
        }
        void doTransitOne(KfBckScr scr,KfTileSet& tiles,QVector<QRgb> colors,u16 x,u16 y){
            QPainter painter(&pixmap);
            CharData chard=scr.at(x,y);
            QImage tile(tiles[
                        chard&TILE_ID_MASK
                    ].data,8,8,QImage::Format_Indexed8);
            tile.setColorTable(colors);
            painter.drawImage(x*8,y*8,
                              tile.mirrored
                              (chard&FLIP_X?true:false,
                               chard&FLIP_Y?true:false));
        }

    public:
        ScrTransit():w(0),h(0){}
        void doTransit(KfBckScr scr,KfTileSet& tiles,QVector<QRgb> colors){
            adjustSize(scr.getWidth(),scr.getHeight());
            for(u16 x=0;x<w;x++)for(u16 y=0;y<h;y++){
                if(tiles.getTickCounter()[scr.at(x,y)&TILE_ID_MASK])
                    doTransitOne(scr,tiles,colors,x,y);
            }
        }
        void doAllTransit(KfBckScr scr,KfTileSet& tiles,QVector<QRgb> colors){
            adjustSize(scr.getWidth(),scr.getHeight());
            for(u16 x=0;x<w;x++)for(u16 y=0;y<h;y++){
                doTransitOne(scr,tiles,colors,x,y);
            }
        }
    };

    KfPlt plt,bckPlt;
    PltTransit pltTransit,bckPltTransit;
    KfTileSet tiles,bckTiles;
    KfBlockSet blocks;
    BlockSetTransit blocksTransit;
    KfBckScr bckScr;
    ScrTransit bckScrTransit;
    KfMap map;
    int curRoomId;

    bool showEssence=false;
    bool showScript=false;
    bool showAnimation=true;
    bool showItems=false;
    bool showBackground=false;

    int selBlock;
private slots:
    void on_listRoom_itemDoubleClicked(QTreeWidgetItem * item);
    void on_updateMap();

    void on_actionAboutMe_triggered();

    void on_actionOpen_triggered();

    void on_actionSave_triggered();

    void on_actionShowEssence_triggered(bool checked);
    void on_actionShowScript_triggered(bool checked);

    void on_actionShowAnimation_triggered(bool checked);

    void on_actionShowItem_triggered(bool checked);

    void on_actionShowBackground_triggered(bool checked);

    void on_actionSaveAs_triggered();

    void on_actionMakeRom_triggered();

    void on_actionMapProperties_triggered();

    void on_actionEnglish_triggered();

    void on_actionChinese_triggered();

    void on_actionExtract_triggered();

    void openMapdata(QString fileName);

    void on_buttonItemUp_clicked();

    void on_buttonItemDown_clicked();

    void on_buttonItemDelete_clicked();

    void on_buttonItemNew_clicked();

    void on_itemTable_clicked(const QModelIndex &index);

    void on_actionDiscardChanges_triggered();

    void on_actionResizeMap_triggered();

    void on_actionSaveToImage_triggered();

    void resetMap();

    void onSelectItem(int itemId);


    void on_actionExportMap_triggered();

    void on_actionImportMap_triggered();

private:
    Ui::MainWindow *ui;
    QTimer mapUpdateTimer;

    QString currentFileName;
    Kf_mapdata mapdata;
    void saveCurrentRoom();
    void loadRoom(int roomId);
    void loadRoomList();

};

#endif // MAINWINDOW_H
