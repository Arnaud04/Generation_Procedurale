#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>

/* **** début de la partie boutons et IHM **** */

/* **** autres fonction **** */
enum Element { VALEY = 1, MOUNTAIN = 2, PLAIN = 3, RIVER = 4 };

// exemple pour charger un fichier .obj
void MainWindow::on_pushButton_chargement_clicked()
{
    // fenêtre de sélection des fichiers
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Mesh"), "", tr("Mesh Files (*.obj)"));

    // chargement du fichier .obj dans la variable globale "mesh"
    OpenMesh::IO::read_mesh(mesh, fileName.toUtf8().constData());

    mesh.update_normals();

    // initialisation des couleurs et épaisseurs (sommets et arêtes) du mesh
    resetAllColorsAndThickness(&mesh);

    // on affiche le maillage
    displayMesh(&mesh);
}

int MainWindow::max(int a, int b)
{
    if(a>=b)
    {
        return a;
    }
    return b;
}

//void regionClassification(int *label, int labelsNumber,int squareLength) // labels [0] -> montagne : 50% ...


/*
** return angle (radians) between norme1 and norme2
*/
float MainWindow::getNormeAngle(MyMesh::Point vecteur1, MyMesh::Point vecteur2)
{
    float normeV1, normeV2,produitScal,angle;
    produitScal = (vecteur1[0]*vecteur2[0]+vecteur1[1]*vecteur2[1]+vecteur1[2]*vecteur2[2]);
    normeV1 = sqrt(vecteur1[0]*vecteur1[0]+vecteur1[1]*vecteur1[1]+vecteur1[2]*vecteur1[2]);
    normeV2 = sqrt(vecteur2[0]*vecteur2[0]+vecteur2[1]*vecteur2[1]+vecteur2[2]*vecteur2[2]);
    float cosAngle = produitScal/(normeV1*normeV2);
    angle = acos(cosAngle); //angle en radians
    return angle;

}

MyMesh::Point MainWindow::getNormalFace(MyMesh* _mesh,VertexHandle v0, VertexHandle v1, VertexHandle v2)
{
     MyMesh::Point resultat;
     QVector<float> vecteur_U, vecteur_V, produitVect, normal;

     //on créer les vecteurs vecteur_U :v0v1 et vecteur_V : v0v2
     for(int i=0; i<3; i++)
     {
         vecteur_U.push_back(_mesh->point(v1)[i] -_mesh->point(v0)[i]);
         vecteur_V.push_back(_mesh->point(v2)[i] -_mesh->point(v0)[i]);
     }

     //On fait le produit vectoriel de U et V
     produitVect.push_back(vecteur_U[1]*vecteur_V[2] - vecteur_U[2]*vecteur_V[1]);
     produitVect.push_back(vecteur_U[0]*vecteur_V[2] - vecteur_U[2]*vecteur_V[0]);
     produitVect.push_back(vecteur_U[0]*vecteur_V[1] - vecteur_U[1]*vecteur_V[0]);

     //On calcul sa norme:
     float normeVect = sqrt(produitVect[0]*produitVect[0]+produitVect[1]*produitVect[1]+produitVect[2]*produitVect[2]);

     //On determine la normal
     for(int i=0; i<3; i++)
     {
         normal.push_back(produitVect[i]/normeVect);
     }

     resultat = MyMesh::Point(normal[0],normal[1],normal[2]);

     return resultat;

}

/* Normals saved in faceNormals
** Where faceNormals[0] correspond to the normal of face 0
** faceNormales[1] to the normal of face 1 ...
*/
void MainWindow::getNormalFaces(MyMesh * _mesh, std::vector<MyMesh::Point> *faceNormals)
{
    MyMesh::Point normaleCurFace;
    std::vector<VertexHandle> v;

    for(MyMesh::FaceIter curFace = _mesh->faces_begin(); curFace != _mesh->faces_end(); curFace++)
    {
        for (MyMesh::FaceVertexIter curVert = _mesh->fv_iter(curFace); curVert.is_valid(); curVert ++)
        {
            v.push_back(curVert);
        }
        normaleCurFace = getNormalFace(_mesh, v[0], v[1], v[2]);
        v.clear();
        faceNormals->push_back(normaleCurFace);
    }
}

void MainWindow::smallAreaRemoval(MyMesh * _mesh, int minimalRegionSize)
{
    int nbLabel;
    int labelCur=1;
    int labelCur2=1;
    std::vector<int> newLabelRegion;

    for(unsigned int i=0; i<_mesh->n_faces(); i++){
        newLabelRegion.push_back(0);
    }

    //On élimne les petite region de taille < minimalRegionSize
    for(unsigned int i=0; i<labelRegion.size(); i++){
        nbLabel =0;
        for(unsigned int j=0; j<labelRegion.size(); j++){
            if(labelRegion.at(j) == labelCur)
                nbLabel ++;
        }
        if(nbLabel < minimalRegionSize)
        {
            for(unsigned int j=0; j<labelRegion.size(); j++){
                if(labelRegion.at(j) == labelCur){
                    labelRegion.at(j) = 0;
                    newLabelRegion.at(j) = 0;
                }
            }
        }
        else
        {
            labelCur2 ++;
            for(unsigned int j=0; j<labelRegion.size(); j++){
                if(labelRegion.at(j) == labelCur){
                    newLabelRegion.at(j) = labelCur2;
                }
            }
        }
        labelCur ++;
    }

    //elimination des petites zones dans notre tableau de labels

    /*for(unsigned int i=0; i<newLabelRegion.size(); i++)
    {
        r = random()%255;
        g = random()%255;
        b = random()%255;

        for(unsigned int j=0; j<newLabelRegion.size(); j++)
        {
            if(newLabelRegion.at(j) == labelTmp)
            {
                _mesh->set_color(_mesh->face_handle(j), MyMesh::Color(r, g, b));
                displayMesh(_mesh);
            }
        }
        labelTmp ++;
    }*/

    //swap array
    for(unsigned int i=0; i<newLabelRegion.size(); i++)
    {
        labelRegion.at(i) = newLabelRegion.at(i);
    }
}

FaceHandle MainWindow::getMaximalHeighterFace(MyMesh * _mesh, float *maximalHeight)
{
    FaceHandle maxHeightFace;
    float maxHeight= -10000000.0;
    float curHeight;

    for(MyMesh::FaceIter curFace = _mesh->faces_begin(); curFace != _mesh->faces_end(); curFace++)
    {
        for (MyMesh::FaceVertexIter curVert = _mesh->fv_iter(*curFace); curVert.is_valid(); curVert ++)
        {
            curHeight = _mesh->point(curVert)[2];
            if(curHeight > maxHeight)
            {
                maxHeight = curHeight;
                maxHeightFace = *curFace;
            }
            //qDebug()<<"cur point"<<_mesh->point(curVert)[0]<<_mesh->point(curVert)[1]<<_mesh->point(curVert)[2];
        }
    }
    //qDebug()<<"maxHeight"<<maxHeight;
    *maximalHeight = maxHeight;
    return maxHeightFace;

}

FaceHandle MainWindow::getMinimaleHeighterFace(MyMesh * _mesh, float *minimalHeight)
{
    FaceHandle minHeightFace;
    float minHeight= 10000000.0;
    float curHeight;

    for(MyMesh::FaceIter curFace = _mesh->faces_begin(); curFace != _mesh->faces_end(); curFace++)
    {
        for (MyMesh::FaceVertexIter curVert = _mesh->fv_iter(*curFace); curVert.is_valid(); curVert ++)
        {

            curHeight = _mesh->point(curVert)[2];
            if(curHeight < minHeight)
            {
                minHeight = curHeight;
                minHeightFace = *curFace;
            }
            //qDebug()<<"cur point"<<_mesh->point(curVert)[0]<<_mesh->point(curVert)[1]<<_mesh->point(curVert)[2];
        }
    }
    *minimalHeight = minHeight;
    return minHeightFace;

}

void MainWindow::defineMountainAreas(MyMesh * _mesh, float precision, float proportion)
{
    float slowerHeight;
    FaceHandle slowerFace = getMinimaleHeighterFace(_mesh, &slowerHeight);

    float maximalHeight;
    FaceHandle heighterFace = getMaximalHeighterFace(_mesh, &maximalHeight);
    float minLevel = (proportion*(maximalHeight-slowerHeight))+slowerHeight;
    float currentLevel = maximalHeight;
    float curVertHeight;
    std::vector<std::vector<int> > faceslabels;
    std::vector<int>facesVisited;
    std::vector<int> v;

    qDebug()<<"minLevel :"<<minLevel;
    v.push_back(heighterFace.idx());//faceslabels.push_back(heighterFace.idx());
    faceslabels.push_back(v);

    qDebug()<<"maxheight"<<maximalHeight;
    while(currentLevel > minLevel)
    {
        currentLevel -= precision;
        for(MyMesh::FaceIter curFace = _mesh->faces_begin(); curFace != _mesh->faces_end(); curFace++)
        {
            bool isHeighter = false;
            for (MyMesh::FaceVertexIter curVert = _mesh->fv_iter(*curFace); curVert.is_valid(); curVert ++)
            {
                curVertHeight = _mesh->point(curVert)[2];
                if(curVertHeight > currentLevel
                   && !(std::find(facesVisited.begin(), facesVisited.end(), curFace->idx()) != facesVisited.end())
                  )
                {
                    isHeighter = true;
                }
             }
            if(isHeighter)
            {
                bool inside = false;
                facesVisited.push_back(curFace->idx());
                for(unsigned int i=0; i<faceslabels.size(); i++)
                {
                    for(unsigned int j=0; j<faceslabels[i].size(); j++)
                    {
                        for (MyMesh::FaceFaceIter curFace2 = _mesh->ff_iter(_mesh->face_handle(faceslabels[i][j])); curFace2.is_valid(); curFace2 ++)
                        {
                            if(curFace2->idx() == curFace->idx() && !inside)
                            {
                                faceslabels[i].push_back(curFace->idx());
                                //qDebug()<<curFace2->idx();
                                inside = true;
                                break;
                            }
                        }
                    }
                }
                if(!inside)
                {
                    std::vector<int> v;
                    v.push_back(curFace->idx());//faceslabels.push_back(heighterFace.idx());
                    faceslabels.push_back(v);
                }
            }
        }
    }

    for(unsigned int i=0; i<faceslabels.size(); i++)
    {
        for(unsigned int j=0; j<faceslabels[i].size(); j++)
        {
            //_mesh->set_color(_mesh->face_handle(faceslabels[i][j]), MyMesh::Color(100, 100, 100));
            //displayMesh(_mesh);
            if(labelRegion.at(faceslabels[i][j]) != PLAIN
               && labelRegion.at(faceslabels[i][j]) != VALEY
               && labelRegion.at(faceslabels[i][j]) != RIVER
             )
                labelRegion.at(faceslabels[i][j])=MOUNTAIN;
        }
    }

}

/*
 * Utilisation de l'algorithme de montée des eaux jusq'a 1/3 au dessus du niveau min
*/
void MainWindow::defineValleyAreas(MyMesh * _mesh, float precision, float proportion)
{
    float minimalHeight;
    float maximalHeight;
    FaceHandle heighterFace = getMaximalHeighterFace(_mesh, &maximalHeight);

    FaceHandle slowerFace = getMinimaleHeighterFace(_mesh, &minimalHeight);
    float maxLevel = (proportion*(maximalHeight-minimalHeight))+minimalHeight;//0.0;
    qDebug()<<"minLevel"<<minimalHeight;
    float currentLevel = minimalHeight;
    float curVertHeight;
    std::vector<std::vector<int> > faceslabels;
    std::vector<int>facesVisited;

    std::vector<int> v;
    v.push_back(slowerFace.idx());
    faceslabels.push_back(v);

    qDebug()<<"maxLevel"<<maxLevel;

    qDebug()<<"hauteur max et min :"<<maximalHeight<<minimalHeight;
    while(currentLevel < maxLevel)
    {
        currentLevel += precision;
        for(MyMesh::FaceIter curFace = _mesh->faces_begin(); curFace != _mesh->faces_end(); curFace++)
        {
            bool isHeighter = false;
            int countIsHeighter = 0;
            for (MyMesh::FaceVertexIter curVert = _mesh->fv_iter(*curFace); curVert.is_valid(); curVert ++)
            {
                curVertHeight = _mesh->point(curVert)[2];
                if(curVertHeight < currentLevel
                   && !(std::find(facesVisited.begin(), facesVisited.end(), curFace->idx()) != facesVisited.end())
                  )
                {
                    isHeighter = true;
                    countIsHeighter++;
                }
             }
            if(countIsHeighter == 3)
            {
                bool inside = false;
                facesVisited.push_back(curFace->idx());
                for(unsigned int i=0; i<faceslabels.size(); i++)
                {
                    for(unsigned int j=0; j<faceslabels[i].size(); j++)
                    {
                        for (MyMesh::FaceFaceIter curFace2 = _mesh->ff_iter(_mesh->face_handle(faceslabels[i][j])); curFace2.is_valid(); curFace2 ++)
                        {
                            if(curFace2->idx() == curFace->idx() && !inside)
                            {
                                faceslabels[i].push_back(curFace->idx());
                                inside = true;
                                break;
                            }
                        }
                    }
                }
                if(!inside)
                {
                    std::vector<int> v;
                    v.push_back(curFace->idx());//faceslabels.push_back(slowerFace.idx());
                    faceslabels.push_back(v);
                }
            }
        }
    }

    for(unsigned int i=0; i<faceslabels.size(); i++)
    {
        for(unsigned int j=0; j<faceslabels[i].size(); j++)
        {
            //_mesh->set_color(_mesh->face_handle(faceslabels[i][j]), MyMesh::Color(10, 80, 20));
            //displayMesh(_mesh);
            if(labelRegion.at(faceslabels[i][j]) != PLAIN
               && labelRegion.at(faceslabels[i][j]) != MOUNTAIN
               && labelRegion.at(faceslabels[i][j]) != RIVER
             )
                labelRegion.at(faceslabels[i][j]) = VALEY;
        }
    }
}

/*
** Cette fonction permet d'éliminer les plaines dont la pente est > au parametre entré
** angle : flaot : slope angle
*/
void MainWindow::removalSlopingPlains(float precisionAngle, std::vector<MyMesh::Point> faceNormals)
{
    MyMesh::Point averageNormale;
    MyMesh::Point ground;
    int label = 1;
    int nbgroupfaces = 0;
    float angle ;

    for(unsigned int i=0; i<labelRegion.size(); i++)
    {
        //qDebug()<<"labelRegion:"<<labelRegion.at(i);
        //qDebug()<<"label"<<label;

        for(unsigned int j=0; j<labelRegion.size(); j++)
        {
            //qDebug()<<"labelregion:"<<labelRegion.at(j)<<"label"<<label;
            if(labelRegion.at(j) == label)
            {
                averageNormale += faceNormals[j];
                nbgroupfaces ++;
                //labelRegion.at(i) = 0;
            }
        }

        if(nbgroupfaces > 0)
        {
            averageNormale[0] = averageNormale[0]/3.0;
            averageNormale[1] = averageNormale[1]/3.0;
            averageNormale[2] = averageNormale[2]/3.0;

            ground[0]=1.0,ground[1]=0.0,ground[2]=0.0;
            angle = getNormeAngle(averageNormale,ground);
            qDebug()<<"angle a 90° avec + ou - de precision "<<angle;
            if((angle < ((M_PI/2)+precisionAngle) && angle >((M_PI/2)-precisionAngle))
               ||(angle < (-(M_PI/2)+precisionAngle) && angle >(-(M_PI/2)-precisionAngle))
            )
            {
                qDebug()<<"angle : ok";
                for(unsigned int j=0; j<labelRegion.size(); j++)
                {
                    if(labelRegion.at(j) == label)
                    {
                        labelRegion.at(j) = PLAIN;
                    }
                }
            }
            else
            {
                qDebug()<<"refusé";
                for(unsigned int j=0; j<labelRegion.size(); j++)
                {
                    if(labelRegion.at(j) == label)
                    {
                        qDebug()<<"j"<<j<<"label"<<label;
                        labelRegion.at(j) = 0;
                    }
                }
            }

        }
        label ++;
        nbgroupfaces = 0;
        averageNormale[0] = 0,averageNormale[1] = 0,averageNormale[2] = 0;
    }
}

void MainWindow::defineFlatAreas(MyMesh * _mesh, std::vector<MyMesh::Point> faceNormals, int label)
{

    std::vector<int> addedNeighbors, neighborsToVisit;
    int nbNeigbors = 0;
    bool hasElement;
    nbNeigbors = 0;

    for(MyMesh::FaceIter curFace = _mesh->faces_begin(); curFace != _mesh->faces_end(); curFace++)
    {

        hasElement = false;

        if(std::find(addedNeighbors.begin(), addedNeighbors.end(), curFace->idx()) != addedNeighbors.end())
            hasElement = true;

        if(!hasElement)
            neighborsToVisit.push_back(curFace->idx());

        nbNeigbors ++;

        while(neighborsToVisit.size() > 0 && hasElement == false)
        {
            FaceHandle curFace1 = _mesh->face_handle(neighborsToVisit.front());


            for (MyMesh::FaceFaceIter curFace2 = _mesh->ff_iter(curFace1); curFace2.is_valid(); curFace2 ++)
            {

                MyMesh::Point normaleCurFace = faceNormals[curFace1.idx()];
                MyMesh::Point normaleCurFace2 = faceNormals[curFace2->idx()];

                //test ecart angulaire
                //qDebug()<<getNormeAngle(normaleCurFace, normaleCurFace2);

                if(getNormeAngle(normaleCurFace, normaleCurFace2) < 0.090 //0.087 = 5deg
                   && curFace1.idx() != curFace2->idx()
                   && !(std::find(addedNeighbors.begin(), addedNeighbors.end(), curFace2->idx()) != addedNeighbors.end())
                   && !(std::find(neighborsToVisit.begin(), neighborsToVisit.end(), curFace2->idx()) != neighborsToVisit.end())
                   && ( curFace1.idx()%(vertexNumber*2) != (vertexNumber-1)|| curFace2->idx()%(vertexNumber*2) != vertexNumber)
                )
                {
                    neighborsToVisit.push_back(curFace2->idx());
                    //qDebug()<<"face added : "<<curFace2->idx();
                }
            }

            addedNeighbors.push_back(neighborsToVisit.at(0));
            labelRegion.at(neighborsToVisit.at(0)) = nbNeigbors;

            //qDebug()<<"labelregion"<<labelRegion.at(addedNeighbors.back());

            if(neighborsToVisit.size()>0)
                neighborsToVisit.erase(neighborsToVisit.begin());
                //neighborsToVisit.pop_front();

       }
       //qDebug()<<"===============================";
       neighborsToVisit.clear();
    }
}

float MainWindow::getStrechingGround(int maxValue, int iteration)
{
    float value = maxValue/((float)iteration+25.0);
    float strechingValue = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    strechingValue *= value;

    float signValue = rand()%2; //entre 0 et 1
    if(signValue >0)
        strechingValue *= -1.0;

    return (float)strechingValue;
}

void MainWindow::squareDiamond(MyMesh * _mesh, int squareLength, int squareWidth, int squareHeight)
{

    int cut_iteration = 0;

    int cut_number_x = 0;
    int cut_number_y = 0;

    int numberPart_x = 2;
    int numberPart_y = 2;

    int partLength_x = 0;
    int partLength_y = 0;

    int cut_pos_x = 0;
    int cut_pos_y = 0;
    int vertexId = 0;

    int greaterSquareSide = max(squareLength, squareWidth);

    while(pow(2,cut_iteration) < (greaterSquareSide-1))
    {

        partLength_x = (squareLength)/numberPart_x;
        cut_pos_x = partLength_x;

        while( cut_pos_x < (squareLength-1))
        {
            //qDebug() << cut_pos_x;

            partLength_y = (squareWidth)/numberPart_y;
            cut_pos_y = partLength_y;

            while( cut_pos_y < (squareWidth-1))
            {

                vertexId = (squareLength)*(cut_pos_y)+(cut_pos_x+1)-1;

                //mise a jour de la hauteur de z du point selectionné
                float heightPoint = getStrechingGround(squareHeight,cut_iteration);

                bool addMatter;
                if(heightPoint >= _mesh->point(_mesh->vertex_handle(vertexId))[2])
                    addMatter = true;
                else //delate Matter (dig)
                    addMatter = false;

                //===modification des points alentour de la surface courante ===
                float halfDistCourantSquare = (float)max(abs(partLength_x),abs(partLength_y/2));

                //qDebug()<<partLength_x<<cut_pos_x-((partLength_x))<<"et"<<cut_pos_x+((partLength_x));
                for(int i=cut_pos_x-(partLength_x); i<cut_pos_x+(partLength_x)+1; i++)
                {
                    for(int j=cut_pos_y-(partLength_y); j<cut_pos_y+(partLength_y)+1; j++)
                    {
                        //qDebug()<<i<<":"<<j;
                        float dist_from_CourantCenterSquare = max(abs(cut_pos_x-i),abs(cut_pos_y-j))+1;
                        vertexId = (squareLength)*(i)+(j);
                        float oldHeightPoint = _mesh->point(_mesh->vertex_handle(vertexId))[2];
                        float courrant_arroundPointHeight = oldHeightPoint+((float)heightPoint/halfDistCourantSquare)*(halfDistCourantSquare-dist_from_CourantCenterSquare);

                        //On ne modifi le point que si ça valeur est plus grande que la précedente
                        if(courrant_arroundPointHeight >= oldHeightPoint && addMatter) //suppression de matière
                        {
                            VertexHandle currentVertex = _mesh->vertex_handle(vertexId);
                            MyMesh::Point NewModifiedCoordonatesCurrentVertex(i, j,courrant_arroundPointHeight);
                            _mesh->set_point(currentVertex, NewModifiedCoordonatesCurrentVertex);
                        }
                        else if(courrant_arroundPointHeight >= oldHeightPoint && !addMatter) //elimination de matière
                        {
                            VertexHandle currentVertex = _mesh->vertex_handle(vertexId);
                            MyMesh::Point NewModifiedCoordonatesCurrentVertex(i, j,
                                          oldHeightPoint-(courrant_arroundPointHeight-oldHeightPoint));
                            _mesh->set_point(currentVertex, NewModifiedCoordonatesCurrentVertex);
                        }

                    }
                }

                cut_pos_y += (/*2**/partLength_y);
                //qDebug() <<"num vertex" <<vertexId;

            }
            cut_pos_x += (/*2**/partLength_x);
        }

        cut_pos_y = 0;
        numberPart_y *= 2;
        cut_number_y *= 4;
        cut_pos_x = 0;
        numberPart_x *= 2;
        cut_number_x *= 4;

        cut_iteration ++;

        //qDebug() <<"nb_iteration :" <<cut_iteration;
    }

}

void MainWindow::getElementsRepartition(MyMesh * _mesh)
{
    float nbValeyFace,nbMountainFace,nbPlainFace,nbRiverFace;
    int totalFaceNumber = _mesh->n_faces();
    float result;

    for(int i=0; i<labelRegion.size(); i++)
    {
        if(labelRegion.at(i) == VALEY)
            nbValeyFace ++;
        else if(labelRegion.at(i) == MOUNTAIN)
            nbMountainFace ++;
        else if(labelRegion.at(i) == PLAIN)
            nbPlainFace ++;
        else if(labelRegion.at(i) == RIVER)
            nbRiverFace ++;
    }
    //qDebug()<<"nbmountain"<<nbMountainFace<<"totalFace"<<totalFaceNumber;
    if(nbMountainFace != 0)
    {
        result = (nbMountainFace*100.0)/(float)totalFaceNumber;
        ui->lcdNumber_Mountain->display(result);
    }
    else
    {
        ui->lcdNumber_Mountain->display(0.0);
    }

    if(nbValeyFace != 0)
    {
        result = (nbValeyFace*100.0)/(float)totalFaceNumber;
        ui->lcdNumber_Valley->display(result);
    }
    else
    {
          ui->lcdNumber_Valley->display(0.0);
    }

    if(nbPlainFace != 0)
    {
        result = (nbPlainFace*100.0)/(float)totalFaceNumber;
        ui->lcdNumber_PLain->display(result);
    }
    else
    {
        ui->lcdNumber_PLain->display(0.0);
    }

}

double MainWindow::getLCDNumberMountain()
{
    return ui->lcdNumber_Mountain->value();
}

double MainWindow::getLCDNumberValley()
{
    return ui->lcdNumber_Valley->value();
}

double MainWindow::getLCDNumberPlain()
{
    return ui->lcdNumber_PLain->value();
}

double MainWindow::getValSpinBoxValley()
{
    return ui->doubleSpinBox_valley->value();
}

double MainWindow::getValSpinBoxMountain()
{
    return ui->doubleSpinBox_mountain->value();
}

double MainWindow::getValSpinBoxPlain()
{
    return ui->doubleSpinBox_plain->value();
}

void MainWindow::setValSpinBoxValley(double val)
{
    ui->doubleSpinBox_valley->setValue(val);
}

void MainWindow::setValSpinBoxMountain(double val)
{
    ui->doubleSpinBox_mountain->setValue(val);
}

void MainWindow::setValSpinBoxPlain(double val)
{
    ui->doubleSpinBox_plain->setValue(val);
}

bool MainWindow::sumSpinBox()
{
    if (getValSpinBoxPlain() + getValSpinBoxValley() + getValSpinBoxMountain() > 100.0)
        return false;
    return true;
}

void MainWindow::printLabelsRegion()
{
    qDebug() << __FUNCTION__;
    qDebug() << labelRegion.size(); // == nb faces
    QVector<int> test;
    int v;
    bool ajout;
    for (int i = 0; i < static_cast<int>(labelRegion.size()); i++) {
        v = labelRegion.at(i);
        ajout = true;
        for (int j = 0; j < static_cast<int>(test.size()); j++) {
            if (test.at(j) == v)
                ajout = false;
        }
        if (ajout) {
            test.push_back(v);
            qDebug() << v;
        }
    }
}

bool MainWindow::compareParamToGenerateMap(QVector<int> * p_vect)
{
    p_vect->push_back( static_cast<int>( getLCDNumberValley() - getValSpinBoxValley() ) );
    p_vect->push_back( static_cast<int>( getLCDNumberMountain() - getValSpinBoxMountain() ) );
    p_vect->push_back( static_cast<int>( getLCDNumberPlain() - getValSpinBoxPlain() ) );
    for (int i = 0; i < p_vect->size(); i++) {
        if(p_vect->at(i) != 0.0)
            return false;
    }
    return true;
}

void MainWindow::app(/*QVector<int> * order_id_elm,*/MyMesh *_mesh, int * toInc, int * toDeac, QVector<int> * vect_valElmRegion, QVector<int> * vect_processToDo_elmRegion, int work_on_id_region = 0)
{
    // ------------ old ----------- KO
    // order_id_elm => FIFO VALEY, MOUNTAIN ...
    //int u = MOUNTAIN;
    /*int process = 0;
    for (int i = 0; i < vect_processToDo_elmRegion->size(); i++) {
        qDebug() << "*********";
        process = vect_processToDo_elmRegion->at(i);
        switch (process) {
        case -1 :
            work_on_id_region = i+1;
            break;
        case 1 :
            work_on_id_region = i+1;
            break;
        default:
            break;
        }

        if (work_on_id_region != 0) {
            qDebug() << "----";
            int i_label = 0;
            for (MyMesh::FaceIter curFace = _mesh->faces_begin(); curFace != _mesh->faces_end(); curFace++)
            {
                //_mesh->set_color(*curFace, MyMesh::Color(150, 150, 150));
                if ( labelRegion.at(i_label) == work_on_id_region ) {
                    _mesh->set_color(*curFace, MyMesh::Color(150, 0, 0));
                    modifPlain(_mesh, process, vect_valElmRegion);

                }
                i_label++;
            }
        }

        work_on_id_region = 0;
        return;
    }*/
}

void MainWindow::modifPlain(MyMesh *_mesh, QVector<int> * vect_valElmRegion)
{
    unsigned nb_area = 1;
    bool OK = false;

    qDebug() << __FUNCTION__ << " " << vect_valElmRegion->at( 0 );
    return; // temporaire

    //int i = 0;
    while (OK) {
        // tire rand
        unsigned id_face_current = static_cast <unsigned> ( rand() ) / static_cast <unsigned> ( _mesh->n_faces() );

        if ( labelRegion.at(id_face_current) != PLAIN) {
            // nb de face a modif
            unsigned nb_faces_to_modif = (_mesh->n_faces() * vect_valElmRegion->at(PLAIN - 1)) / 100;

            // --- parcours voisinage face
            QVector<int> vect_neight_visited;
            QVector<int> vect_neight_to_visited;
            /*while () {
                MyMesh::FaceHandle fh_current = _mesh->face_handle(id_face_current);
                for (MyMesh::FaceFaceIter ff_iter = _mesh->ff_iter(fh_current); ff_iter.is_valid(); ff_iter++) {
                    if ( labelRegion.at(id_face_current) != PLAIN) {

                    }
                }
            }*/
            // ---
        }
    }
}

void MainWindow::printListPoint(MyMesh *_mesh)
{

    for (MyMesh::VertexIter curVert = _mesh->vertices_begin(); curVert != _mesh->vertices_end(); curVert++)
    {
        _mesh->data(*curVert).thickness = 3;
        _mesh->set_color(*curVert, MyMesh::Color(0, 0, 0));
        //qDebug()<<curVert->idx();
    }
}

void MainWindow::builtSquareFaces(MyMesh *_mesh, int squareLength, int squareWidth)
{
     std::vector<MyMesh::VertexHandle> uneNouvelleFace;

     VertexHandle topNeigboor;
     VertexHandle rightNeigboor;
     bool isTopNeigbor =false;
     bool isRightNeigbor = false;
     bool isCornerNeigbor = false;
     int rightNeigborPos,topNeigborPos, cornerNeigborPos;

     for(MyMesh::VertexIter curVert1 = _mesh->vertices_begin(); curVert1 != _mesh->vertices_end(); curVert1++)
     {
         VertexHandle v1 = *curVert1;

         int curVert1_x = _mesh->point(v1)[0];
         int curVert1_y = _mesh->point(v1)[1];

         if((curVert1_x < squareLength)&&(curVert1_y < squareWidth))
         {
             uneNouvelleFace.clear();
             for(MyMesh::VertexIter curVert2 = _mesh->vertices_begin(); curVert2 != _mesh->vertices_end(); curVert2++)
             {

                 VertexHandle v2 = *curVert2;
                 int curVert2_x = _mesh->point(v2)[0];
                 int curVert2_y = _mesh->point(v2)[1];


                 if((curVert2_x < squareLength)&&(curVert2_y < squareWidth))
                 {

                     if((curVert2_y == curVert1_y)&&(curVert2_x == curVert1_x+1))
                     {
                         rightNeigborPos = v2.idx();
                         isRightNeigbor = true;
                     }
                     if((curVert2_x == curVert1_x)&&(curVert2_y == curVert1_y+1))
                     {
                         topNeigborPos = v2.idx();
                         isTopNeigbor = true;
                     }
                     if((curVert2_y == curVert1_y+1)&&(curVert2_x == curVert1_x+1))
                     {
                         cornerNeigborPos = v2.idx();
                         isCornerNeigbor = true;
                     }

                 }
             }
             if(isRightNeigbor && isTopNeigbor && isCornerNeigbor)
             {
                 uneNouvelleFace.push_back(_mesh->vertex_handle(cornerNeigborPos));
                 uneNouvelleFace.push_back(_mesh->vertex_handle(rightNeigborPos));
                 uneNouvelleFace.push_back(_mesh->vertex_handle(v1.idx()));
                 uneNouvelleFace.push_back(_mesh->vertex_handle(topNeigborPos));

                _mesh->add_face(uneNouvelleFace);

             }

             isTopNeigbor = false;
             isRightNeigbor = false;
             isCornerNeigbor = false;
         }
         //test =false;

     }
     _mesh->update_normals();
     //this->mesh = _mesh;


}

// exemple pour construire un mesh face par face
void MainWindow::on_pushButton_generer_clicked()
{
    int maxSizeX = vertexNumber+1; //2n+1
    int maxSizeY = vertexNumber+1; //
    int maxSizeZ = vertexNumber/2;

    mesh.clear();

    // on construit une liste de sommets
    MyMesh::VertexHandle sommets[maxSizeX*maxSizeY];

    int precision = 1;
    int numberPoint =0;

    for(int x=0; x<maxSizeX; x++)
    {
        for(int y=0; y<maxSizeY;y++)
        {
            numberPoint++;
            sommets[numberPoint] = mesh.add_vertex(MyMesh::Point(x, y, 0));
        }

    }

    // initialisation des couleurs et épaisseurs (sommets et arêtes) du mesh
    resetAllColorsAndThickness(&mesh);

    printListPoint(&mesh);
    squareDiamond(&mesh, maxSizeX, maxSizeY,maxSizeZ);
    builtSquareFaces(&mesh, maxSizeX,maxSizeY);

    // on affiche le maillage
    resetAllColorsAndThickness(&mesh);
    displayMesh(&mesh);

    labelRegion.clear();
    for(unsigned int i=0; i<mesh.n_faces(); i++)
    {
        labelRegion.push_back(0);
    }
}

/* **** fin de la partie boutons et IHM **** */



/* **** fonctions supplémentaires **** */
// permet d'initialiser les couleurs et les épaisseurs des élements du maillage
void MainWindow::resetAllColorsAndThickness(MyMesh* _mesh)
{
    for (MyMesh::VertexIter curVert = _mesh->vertices_begin(); curVert != _mesh->vertices_end(); curVert++)
    {
        _mesh->data(*curVert).thickness = 1;
        _mesh->set_color(*curVert, MyMesh::Color(0, 0, 0));
    }

    for (MyMesh::FaceIter curFace = _mesh->faces_begin(); curFace != _mesh->faces_end(); curFace++)
    {
        _mesh->set_color(*curFace, MyMesh::Color(150, 150, 150));
    }

    for (MyMesh::EdgeIter curEdge = _mesh->edges_begin(); curEdge != _mesh->edges_end(); curEdge++)
    {
        _mesh->data(*curEdge).thickness = 1;
        _mesh->set_color(*curEdge, MyMesh::Color(0, 0, 0));
    }
}

// charge un objet MyMesh dans l'environnement OpenGL
void MainWindow::displayMesh(MyMesh* _mesh, bool isTemperatureMap, float mapRange)
{
    GLuint* triIndiceArray = new GLuint[_mesh->n_faces() * 3];
    GLfloat* triCols = new GLfloat[_mesh->n_faces() * 3 * 3];
    GLfloat* triVerts = new GLfloat[_mesh->n_faces() * 3 * 3];

    int i = 0;

    if(isTemperatureMap)
    {
        QVector<float> values;

        if(mapRange == -1)
        {
            for (MyMesh::VertexIter curVert = _mesh->vertices_begin(); curVert != _mesh->vertices_end(); curVert++)
                values.append(fabs(_mesh->data(*curVert).value));
            qSort(values);
            mapRange = values.at(values.size()*0.8);
            qDebug() << "mapRange" << mapRange;
        }

        float range = mapRange;
        MyMesh::ConstFaceIter fIt(_mesh->faces_begin()), fEnd(_mesh->faces_end());
        MyMesh::ConstFaceVertexIter fvIt;

        for (; fIt!=fEnd; ++fIt)
        {
            fvIt = _mesh->cfv_iter(*fIt);
            if(_mesh->data(*fvIt).value > 0){triCols[3*i+0] = 255; triCols[3*i+1] = 255 - std::min((_mesh->data(*fvIt).value/range) * 255.0, 255.0); triCols[3*i+2] = 255 - std::min((_mesh->data(*fvIt).value/range) * 255.0, 255.0);}
            else{triCols[3*i+2] = 255; triCols[3*i+1] = 255 - std::min((-_mesh->data(*fvIt).value/range) * 255.0, 255.0); triCols[3*i+0] = 255 - std::min((-_mesh->data(*fvIt).value/range) * 255.0, 255.0);}
            triVerts[3*i+0] = _mesh->point(*fvIt)[0]; triVerts[3*i+1] = _mesh->point(*fvIt)[1]; triVerts[3*i+2] = _mesh->point(*fvIt)[2];
            triIndiceArray[i] = i;

            i++; ++fvIt;
            if(_mesh->data(*fvIt).value > 0){triCols[3*i+0] = 255; triCols[3*i+1] = 255 - std::min((_mesh->data(*fvIt).value/range) * 255.0, 255.0); triCols[3*i+2] = 255 - std::min((_mesh->data(*fvIt).value/range) * 255.0, 255.0);}
            else{triCols[3*i+2] = 255; triCols[3*i+1] = 255 - std::min((-_mesh->data(*fvIt).value/range) * 255.0, 255.0); triCols[3*i+0] = 255 - std::min((-_mesh->data(*fvIt).value/range) * 255.0, 255.0);}
            triVerts[3*i+0] = _mesh->point(*fvIt)[0]; triVerts[3*i+1] = _mesh->point(*fvIt)[1]; triVerts[3*i+2] = _mesh->point(*fvIt)[2];
            triIndiceArray[i] = i;

            i++; ++fvIt;
            if(_mesh->data(*fvIt).value > 0){triCols[3*i+0] = 255; triCols[3*i+1] = 255 - std::min((_mesh->data(*fvIt).value/range) * 255.0, 255.0); triCols[3*i+2] = 255 - std::min((_mesh->data(*fvIt).value/range) * 255.0, 255.0);}
            else{triCols[3*i+2] = 255; triCols[3*i+1] = 255 - std::min((-_mesh->data(*fvIt).value/range) * 255.0, 255.0); triCols[3*i+0] = 255 - std::min((-_mesh->data(*fvIt).value/range) * 255.0, 255.0);}
            triVerts[3*i+0] = _mesh->point(*fvIt)[0]; triVerts[3*i+1] = _mesh->point(*fvIt)[1]; triVerts[3*i+2] = _mesh->point(*fvIt)[2];
            triIndiceArray[i] = i;

            i++;
        }
    }
    else
    {
        MyMesh::ConstFaceIter fIt(_mesh->faces_begin()), fEnd(_mesh->faces_end());
        MyMesh::ConstFaceVertexIter fvIt;
        for (; fIt!=fEnd; ++fIt)
        {
            fvIt = _mesh->cfv_iter(*fIt);
            triCols[3*i+0] = _mesh->color(*fIt)[0]; triCols[3*i+1] = _mesh->color(*fIt)[1]; triCols[3*i+2] = _mesh->color(*fIt)[2];
            triVerts[3*i+0] = _mesh->point(*fvIt)[0]; triVerts[3*i+1] = _mesh->point(*fvIt)[1]; triVerts[3*i+2] = _mesh->point(*fvIt)[2];
            triIndiceArray[i] = i;

            i++; ++fvIt;
            triCols[3*i+0] = _mesh->color(*fIt)[0]; triCols[3*i+1] = _mesh->color(*fIt)[1]; triCols[3*i+2] = _mesh->color(*fIt)[2];
            triVerts[3*i+0] = _mesh->point(*fvIt)[0]; triVerts[3*i+1] = _mesh->point(*fvIt)[1]; triVerts[3*i+2] = _mesh->point(*fvIt)[2];
            triIndiceArray[i] = i;

            i++; ++fvIt;
            triCols[3*i+0] = _mesh->color(*fIt)[0]; triCols[3*i+1] = _mesh->color(*fIt)[1]; triCols[3*i+2] = _mesh->color(*fIt)[2];
            triVerts[3*i+0] = _mesh->point(*fvIt)[0]; triVerts[3*i+1] = _mesh->point(*fvIt)[1]; triVerts[3*i+2] = _mesh->point(*fvIt)[2];
            triIndiceArray[i] = i;

            i++;
        }
    }


    ui->displayWidget->loadMesh(triVerts, triCols, _mesh->n_faces() * 3 * 3, triIndiceArray, _mesh->n_faces() * 3);

    delete[] triIndiceArray;
    delete[] triCols;
    delete[] triVerts;

    GLuint* linesIndiceArray = new GLuint[_mesh->n_edges() * 2];
    GLfloat* linesCols = new GLfloat[_mesh->n_edges() * 2 * 3];
    GLfloat* linesVerts = new GLfloat[_mesh->n_edges() * 2 * 3];

    i = 0;
    QHash<float, QList<int> > edgesIDbyThickness;
    for (MyMesh::EdgeIter eit = _mesh->edges_begin(); eit != _mesh->edges_end(); ++eit)
    {
        float t = _mesh->data(*eit).thickness;
        if(t > 0)
        {
            if(!edgesIDbyThickness.contains(t))
                edgesIDbyThickness[t] = QList<int>();
            edgesIDbyThickness[t].append((*eit).idx());
        }
    }
    QHashIterator<float, QList<int> > it(edgesIDbyThickness);
    QList<QPair<float, int> > edgeSizes;
    while (it.hasNext())
    {
        it.next();

        for(int e = 0; e < it.value().size(); e++)
        {
            int eidx = it.value().at(e);

            MyMesh::VertexHandle vh1 = _mesh->to_vertex_handle(_mesh->halfedge_handle(_mesh->edge_handle(eidx), 0));
            linesVerts[3*i+0] = _mesh->point(vh1)[0];
            linesVerts[3*i+1] = _mesh->point(vh1)[1];
            linesVerts[3*i+2] = _mesh->point(vh1)[2];
            linesCols[3*i+0] = _mesh->color(_mesh->edge_handle(eidx))[0];
            linesCols[3*i+1] = _mesh->color(_mesh->edge_handle(eidx))[1];
            linesCols[3*i+2] = _mesh->color(_mesh->edge_handle(eidx))[2];
            linesIndiceArray[i] = i;
            i++;

            MyMesh::VertexHandle vh2 = _mesh->from_vertex_handle(_mesh->halfedge_handle(_mesh->edge_handle(eidx), 0));
            linesVerts[3*i+0] = _mesh->point(vh2)[0];
            linesVerts[3*i+1] = _mesh->point(vh2)[1];
            linesVerts[3*i+2] = _mesh->point(vh2)[2];
            linesCols[3*i+0] = _mesh->color(_mesh->edge_handle(eidx))[0];
            linesCols[3*i+1] = _mesh->color(_mesh->edge_handle(eidx))[1];
            linesCols[3*i+2] = _mesh->color(_mesh->edge_handle(eidx))[2];
            linesIndiceArray[i] = i;
            i++;
        }
        edgeSizes.append(qMakePair(it.key(), it.value().size()));
    }

    ui->displayWidget->loadLines(linesVerts, linesCols, i * 3, linesIndiceArray, i, edgeSizes);

    delete[] linesIndiceArray;
    delete[] linesCols;
    delete[] linesVerts;

    GLuint* pointsIndiceArray = new GLuint[_mesh->n_vertices()];
    GLfloat* pointsCols = new GLfloat[_mesh->n_vertices() * 3];
    GLfloat* pointsVerts = new GLfloat[_mesh->n_vertices() * 3];

    i = 0;
    QHash<float, QList<int> > vertsIDbyThickness;
    for (MyMesh::VertexIter vit = _mesh->vertices_begin(); vit != _mesh->vertices_end(); ++vit)
    {
        float t = _mesh->data(*vit).thickness;
        if(t > 0)
        {
            if(!vertsIDbyThickness.contains(t))
                vertsIDbyThickness[t] = QList<int>();
            vertsIDbyThickness[t].append((*vit).idx());
        }
    }
    QHashIterator<float, QList<int> > vitt(vertsIDbyThickness);
    QList<QPair<float, int> > vertsSizes;

    while (vitt.hasNext())
    {
        vitt.next();

        for(int v = 0; v < vitt.value().size(); v++)
        {
            int vidx = vitt.value().at(v);

            pointsVerts[3*i+0] = _mesh->point(_mesh->vertex_handle(vidx))[0];
            pointsVerts[3*i+1] = _mesh->point(_mesh->vertex_handle(vidx))[1];
            pointsVerts[3*i+2] = _mesh->point(_mesh->vertex_handle(vidx))[2];
            pointsCols[3*i+0] = _mesh->color(_mesh->vertex_handle(vidx))[0];
            pointsCols[3*i+1] = _mesh->color(_mesh->vertex_handle(vidx))[1];
            pointsCols[3*i+2] = _mesh->color(_mesh->vertex_handle(vidx))[2];
            pointsIndiceArray[i] = i;
            i++;
        }
        vertsSizes.append(qMakePair(vitt.key(), vitt.value().size()));
    }

    ui->displayWidget->loadPoints(pointsVerts, pointsCols, i * 3, pointsIndiceArray, i, vertsSizes);

    delete[] pointsIndiceArray;
    delete[] pointsCols;
    delete[] pointsVerts;
}


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    mesh.clear();
    vertexNumber *= 2;
    on_pushButton_generer_clicked();
}

void MainWindow::on_delateVertex_clicked()
{
    mesh.clear();
    vertexNumber /=2;
    on_pushButton_generer_clicked();
}

void MainWindow::on_setRegion_clicked()
{
    std::vector<MyMesh::Point> faceNormals;
    int label = 1;
    int minimalRegionSize = 3;
    getNormalFaces(&mesh, &faceNormals);

    for(unsigned int i=0; i<faceNormals.size(); i++)
    {
        MyMesh::Point p;
        p = faceNormals[i];
        //qDebug()<<p[0]<<p[1]<<p[2];
    }

    defineFlatAreas(&mesh,faceNormals, label);
    smallAreaRemoval(&mesh,minimalRegionSize);
    removalSlopingPlains(0.04/*5°*/,faceNormals);

    float proportionElement = 0.5;
    defineMountainAreas(&mesh,0.01,proportionElement); //proportionElement under average height
    defineValleyAreas(&mesh,0.01,proportionElement); //proportionElement under average height

    //test affichage
    for(unsigned int i=0; i<labelRegion.size(); i++)
    {
        qDebug()<<labelRegion.at(i)<<" ";
    }

    for(unsigned int i=0; i<labelRegion.size(); i++ )
    {
          if(labelRegion.at(i) == PLAIN)
            mesh.set_color(mesh.face_handle(i), MyMesh::Color(58, 29, 00));
          else if(labelRegion.at(i) == MOUNTAIN)
              mesh.set_color(mesh.face_handle(i), MyMesh::Color(100, 100, 98));
          else if(labelRegion.at(i) == VALEY)
              mesh.set_color(mesh.face_handle(i), MyMesh::Color(10, 80, 20));
    }
    displayMesh(&mesh);

    getElementsRepartition(&mesh);

    // --- maj ---
    setValSpinBoxPlain( getLCDNumberPlain() );
    setValSpinBoxValley( getLCDNumberValley() );
    setValSpinBoxMountain( getLCDNumberMountain() );
    // -----------
}

void MainWindow::on_QPushButon_Modif_clicked()
{
    // Valley 1, Mountain 2, Plain 3
    qDebug() << __FUNCTION__;
    /*mesh.set_color(mesh.face_handle(1), MyMesh::Color(150, 0, 0));
    displayMesh(&mesh);*/
    //printLabelsRegion();

    // verifier si les param correspond au a la map deja generer
    QVector<int> vect_valElmRegion;
    vect_valElmRegion.reserve(3);

    if ( !compareParamToGenerateMap(&vect_valElmRegion) ) {
        QVector<int> vect_processToDo_elmRegion; // -1 a diminuer, 1 a augmenter, 0 ne rien faire
        int elmRegionToIncrease = 0;
        int elmRegionToDecrease = 0;
        for (int i = 0; i < vect_valElmRegion.size(); i++) {
            if (vect_valElmRegion.at(i) < 0.0) {
                 vect_processToDo_elmRegion.push_back(1);
                 elmRegionToIncrease++;
            }
            else if (vect_valElmRegion.at(i) > 0.0) {
                vect_processToDo_elmRegion.push_back(-1);
                elmRegionToDecrease++;
            }
            else
                vect_processToDo_elmRegion.push_back(0);
        }

        // ----------------------
        // appliquer les changement de hauteur
        qDebug() << "toInc : : " << elmRegionToIncrease << " || toDeac : : " << elmRegionToDecrease;

        modifPlain(&mesh, &vect_valElmRegion);


        //app(&mesh, &elmRegionToIncrease, &elmRegionToDecrease, &vect_valElmRegion, &vect_processToDo_elmRegion);
        //displayMesh(&mesh);

        return;
        for (int i = 0; i < vect_processToDo_elmRegion.size(); i++) {
            if (vect_processToDo_elmRegion.at(i) != 0) {
                /*for (int j = i+1; j != i; j = (j + 1)%vect_processToDo_elmRegion.size() ) {

                }*/
            }
        }
    }
}

void MainWindow::on_doubleSpinBox_mountain_valueChanged(double arg1)
{
    if (!sumSpinBox())
        setValSpinBoxMountain(arg1-1);
}

void MainWindow::on_doubleSpinBox_valley_valueChanged(double arg1)
{
    if (!sumSpinBox())
        setValSpinBoxValley(arg1-1);
}

void MainWindow::on_doubleSpinBox_plain_valueChanged(double arg1)
{
    if (!sumSpinBox())
        setValSpinBoxPlain(arg1-1);
}
