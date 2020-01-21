#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileDialog>
#include <QMainWindow>
#include <math.h>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <unistd.h>
#include "math.h"


namespace Ui {
class MainWindow;
}

using namespace OpenMesh;
using namespace OpenMesh::Attributes;

struct MyTraits : public OpenMesh::DefaultTraits
{
    // use vertex normals and vertex colors
    VertexAttributes( OpenMesh::Attributes::Normal | OpenMesh::Attributes::Color | OpenMesh::Attributes::Status);
    // store the previous halfedge
    HalfedgeAttributes( OpenMesh::Attributes::PrevHalfedge );
    // use face normals face colors
    FaceAttributes( OpenMesh::Attributes::Normal | OpenMesh::Attributes::Color | OpenMesh::Attributes::Status);
    EdgeAttributes( OpenMesh::Attributes::Color | OpenMesh::Attributes::Status );
    // vertex thickness
    VertexTraits{float thickness; float value;};
    // edge thickness
    EdgeTraits{float thickness;};
};
typedef OpenMesh::TriMesh_ArrayKernelT<MyTraits> MyMesh;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void displayMesh(MyMesh *_mesh, bool isTemperatureMap = false, float mapRange = -1);
    void resetAllColorsAndThickness(MyMesh* _mesh);
    int max(int a, int b);
    MyMesh::Point getNormalFace(MyMesh* _mesh,VertexHandle v0, VertexHandle v1, VertexHandle v2);
    void defineFlatAreas(MyMesh * _mesh,
                                 std::vector<MyMesh::Point> faceNormals, int label);
    void defineMountainAreas(MyMesh * _mesh, float precision, float proportion);
    void defineValleyAreas(MyMesh * _mesh, float precision, float proportion);
    void removalSlopingPlains(float precisionAngle, std::vector<MyMesh::Point> faceNormals);
    FaceHandle getMaximalHeighterFace(MyMesh * _mesh, float *maximalHeight);
    FaceHandle getMinimaleHeighterFace(MyMesh * _mesh, float *minimalHeight);
    void smallAreaRemoval(MyMesh * _mesh, int minimalRegionSize);
    void squareDiamond(MyMesh * _mesh, int squareLength, int squareWidth,int squareHeight);
    void getNormalFaces(MyMesh * _mesh, std::vector<MyMesh::Point> *faceNormals);
    float getNormeAngle(MyMesh::Point vecteur1, MyMesh::Point vecteur2);
    void builtSquareFaces(MyMesh *_mesh, int squareLength, int squareWidth);
    float getStrechingGround(int maxValue, int iteration); //set
    void printListPoint(MyMesh *_mesh);
    void getElementsRepartition(MyMesh * _mesh);

    double getLCDNumberMountain();
    double getLCDNumberValley();
    double getLCDNumberPlain();

    double getValSpinBoxValley();
    double getValSpinBoxMountain();
    double getValSpinBoxPlain();
    void setValSpinBoxValley(double val);
    void setValSpinBoxMountain(double val);
    void setValSpinBoxPlain(double val);
    bool sumSpinBox();
    void printLabelsRegion();
    bool compareParamToGenerateMap(QVector<int> * p_vect);
    void app(/*QVector<int> * order_id_elm,*/MyMesh *_mesh, int * toInc, int * toDeac, QVector<int> * vect_valElmRegion, QVector<int> * vect_processToDo_elmRegion, int work_on_id_region);
    void modifPlain(MyMesh * _mesh, QVector<int> * vect_valElmRegion);

private slots:
    void on_pushButton_chargement_clicked();

    void on_pushButton_generer_clicked();

    void on_pushButton_clicked();

    void on_delateVertex_clicked();

    void on_setRegion_clicked();

    void on_QPushButon_Modif_clicked();

    void on_doubleSpinBox_mountain_valueChanged(double arg1);

    void on_doubleSpinBox_valley_valueChanged(double arg1);

    void on_doubleSpinBox_plain_valueChanged(double arg1);

private:

    MyMesh mesh;
    Ui::MainWindow *ui;
    int vertexNumber = 16;
    std::vector<int> labelRegion;

};

#endif // MAINWINDOW_H
